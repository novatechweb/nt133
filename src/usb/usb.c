#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <strings.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/rcc.h>

#include <platform.h>
#include <nt_timer.h>
#include <io.h>
#include "usb.h"

#define BOARD_IDENT "NovaTech Input & Output ["BUILDDATE"]"
#define INTERFACE_STRING "Input/Output control"

#define SERIALNO_FLASH_LOCATION	0x8001ff0

#define NT133_USB_REQ_INPUT_STATUS 0x10
#define NT133_USB_REQ_OUTPUT_STATUS 0x11
#define NT133_USB_REQ_OUTPUT_CTRL 0x12
#define NT133_USB_REQ_OUTPUT_ENABLE 0x13

#define INTERRUPT_EP_NUM 0x01

#define EP_ADDR_INTERRUPT (EP_ADDR_IN | (0x0F & INTERRUPT_EP_NUM))

#define INTERRUPT_DATA_BUFF_LEN 2
#define INPUTS_DATA_BUFF_LEN 2
#define TIMER_REMAINING_DATA_BUFF_LEN 4

const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = USB_2_0_STANDARD,
	.bDeviceClass = USB_CLASS_VENDOR,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
 //	.bMaxPacketSize0 = 8, // 8, 16, 32, 64
	.idVendor = NOVATECH_VENDOR_ID,
	.idProduct = USB_PRODUCT_ID,
	.bcdDevice = USB_BCD_VERSION_NUM,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor interrupt_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = EP_ADDR_INTERRUPT,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = INTERRUPT_DATA_BUFF_LEN,
	.bInterval = 1,
}};

const struct usb_interface_descriptor iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_VENDOR,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.endpoint = interrupt_endp,
};

const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = &iface,
}};

const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"NovaTech LLC",
	BOARD_IDENT,
	(char *)SERIALNO_FLASH_LOCATION,
	INTERFACE_STRING,
};

// Global handle to USB device data structure
usbd_device * global_usb_dev_handle;

// Global flag for the main loop to state that USB needs to be handled
bool usb_interrupt_flag = false;

// ***  USB Message Buffers  ***
// Buffer to be used for control requests.
uint8_t usbd_control_buffer[128];
// buffer used to send the values of the input ports back to the host
uint8_t inputs_data_buffer[INPUTS_DATA_BUFF_LEN];
// buffer used to send the values of the input ports back to the host
uint8_t timer_remaining_data_buffer[TIMER_REMAINING_DATA_BUFF_LEN];


uint16_t usb_set_interrupt_data(uint16_t pin_state)
{
	// NOTE:        interrupt_pending_buffer[]
	// The first two bytes of interrupt_pending_buffer hold the state of input pins
	//   interrupt_pending_buffer[0] == input pins 16, 15, 14, 13, 12, 11, 10, 09
	//   interrupt_pending_buffer[1] == input pins 08, 07 ,06 ,05 ,04 ,03 ,02 ,01

	// buffer holding the data for interrupt endpoint
	uint8_t interrupt_pending_buffer[INTERRUPT_DATA_BUFF_LEN];
	// store the current state of the pins
	interrupt_pending_buffer[0] = pin_state >> 8;
	interrupt_pending_buffer[1] = pin_state & 0xFF;
	// set the data to be sent
	return usbd_ep_write_packet(global_usb_dev_handle, EP_ADDR_INTERRUPT, interrupt_pending_buffer, INTERRUPT_DATA_BUFF_LEN);
}

static int get_inputs_control_callback(usbd_device * UNUSED(usbd_dev),
	struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
	void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req) __attribute__((unused)))
{
	register uint16_t pin_state;
	if (req->bRequest != NT133_USB_REQ_INPUT_STATUS) {
		// not handling any other request other than NT133_USB_REQ_INPUT_STATUS
		return USBD_REQ_NEXT_CALLBACK;
	}
	pin_state = get_inputs();
	inputs_data_buffer[0] = pin_state >> 8;
	inputs_data_buffer[1] = pin_state & 0xFF;
	*buf = inputs_data_buffer;
	*len = INPUTS_DATA_BUFF_LEN;

	return USBD_REQ_HANDLED;
}

static int set_output_control_callback(usbd_device * UNUSED(usbd_dev),
	struct usb_setup_data *req, uint8_t **buf, uint16_t * UNUSED(len),
	void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req) __attribute__((unused)))
{
	uint32_t timeout;
	uint8_t *buff = *buf;

	if (req->bRequest != NT133_USB_REQ_OUTPUT_CTRL) {
		// not handling any other request other than NT133_USB_REQ_OUTPUT_CTRL
		return USBD_REQ_NEXT_CALLBACK;
	}
	if (!relays_enabled()) {
		// not handling setting an output port without first enabiling the outputs
		return USBD_REQ_NOTSUPP;
	}
	if ((req->wLength != 4) || (buf == NULL) || (*buf == NULL)) {
		// not handling setting an output port without a timeout value
		return USBD_REQ_NOTSUPP;
	}

	// calculate the timeout value from the data
	timeout = (buff[0] * 16777216) + (buff[1] * 65536) + (buff[2] * 256) + buff[3];

	// set the output and start the timer
	if (set_output(req->wIndex, ((req->wValue == 0) ? false : true), timeout)) {
		return USBD_REQ_HANDLED;
	}

	// error occured while trying to set the outputs
	return USBD_REQ_NOTSUPP;
}

static int get_output_timer_control_callback(usbd_device * UNUSED(usbd_dev),
	struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
	void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req) __attribute__((unused)))
{
	usec_time_t timer_remaining;
	if (req->bRequest != NT133_USB_REQ_OUTPUT_STATUS) {
		// not handling any other request other than NT133_USB_REQ_OUTPUT_STATUS
		return USBD_REQ_NEXT_CALLBACK;
	}
	timer_remaining = get_timer_remaining(req->wIndex);
	if (timer_remaining > 0x1FFFFFFF) {
		// an invalid time was returned
		return USBD_REQ_NOTSUPP;
	}

	timer_remaining_data_buffer[0] = (uint8_t)(timer_remaining >> 24);
	timer_remaining_data_buffer[1] = (uint8_t)(timer_remaining >> 16);
	timer_remaining_data_buffer[2] = (uint8_t)(timer_remaining >>  8);
	timer_remaining_data_buffer[3] = (uint8_t)(timer_remaining >>  0);
	*buf = timer_remaining_data_buffer;
	*len = TIMER_REMAINING_DATA_BUFF_LEN;

	return USBD_REQ_HANDLED;
}

static int set_output_enable_control_callback(usbd_device * UNUSED(usbd_dev),
	struct usb_setup_data *req, uint8_t ** UNUSED(buf), uint16_t * UNUSED(len),
	void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req) __attribute__((unused)))
{
	if (req->bRequest != NT133_USB_REQ_OUTPUT_ENABLE) {
		// not handling any other request other than NT133_USB_REQ_OUTPUT_ENABLE
		return USBD_REQ_NEXT_CALLBACK;
	}

	if (req->wValue == 0) {
		disable_relays();
	} else {
		enable_relays();
	}

	return USBD_REQ_HANDLED;
}

static void set_config(usbd_device *usbd_dev, uint16_t UNUSED(wValue))
{
	usbd_ep_setup(usbd_dev,
		EP_ADDR_INTERRUPT,
		USB_ENDPOINT_ATTR_INTERRUPT,
		INTERRUPT_DATA_BUFF_LEN,
		NULL);
	// IN controll message (sends data back to host)
	usbd_register_control_callback(usbd_dev,
		USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_IN,
		USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE,
		get_inputs_control_callback);
	// OUT control message (recieves data from host)
	usbd_register_control_callback(usbd_dev,
		USB_REQ_TYPE_VENDOR,
		USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE,
		set_output_control_callback);
	// IN controll message (sends data back to host)
	usbd_register_control_callback(usbd_dev,
		USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_IN,
		USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE,
		get_output_timer_control_callback);
	// OUT control message (recieves data from host)
	usbd_register_control_callback(usbd_dev,
		USB_REQ_TYPE_VENDOR,
		USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE,
		set_output_enable_control_callback);
}

void usb_reenumerate(void)
{
	nvic_disable_irq(USB_IRQ);
	// reset usb hardware
	usb_reset_hardware();
	// init USB hardware
	usb_platform_init();
	// Initilize the USB dev handle
	global_usb_dev_handle = usbd_init(&USB_DRIVER, &dev,
		&config, usb_strings, sizeof(usb_strings)/sizeof(char *),
		usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(global_usb_dev_handle, set_config);
	// specify there isn't any usb interrupts that need handaling
	usb_interrupt_flag = false;
	// set ISR priority
	nvic_set_priority(USB_IRQ, IRQ_PRI_USB);
	// enable interrupt
	nvic_enable_irq(USB_IRQ);
	// perform the re-enumerate steps
	usb_platform_reenumerate();
}

void USB_ISR(void)
{
	DBG_USB_ISR();
	// disable USB until the interrupt request has been handled
	nvic_disable_irq(USB_IRQ);
	// flag that there is at least one USB interrupt that needs handled
	usb_interrupt_flag = true;
	DBG_USB_ISR();
}

inline void poll_usb(void)
{
	while (usb_interrupt_flag) {
		// USB interrupts are disabled
		usbd_poll(global_usb_dev_handle);
		// clear the flag
		usb_interrupt_flag = false;
		// enable USB interrupt again
		nvic_enable_irq(USB_IRQ);
		// the usb interrupt handler may be called here.
		// This will set the flag and disable the USB interrupt once again
	}
}
