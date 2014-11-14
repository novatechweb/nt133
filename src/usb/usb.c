/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2011  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "usb.h"

#include <io.h>
#include <platform.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <strings.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/rcc.h>

#define BOARD_IDENT "NovaTech Input & Output ["BUILDDATE"]"
#define INTERFACE_STRING "Input/Output control"

#define SERIALNO_FLASH_LOCATION	0x8001ff0

#define IO_REQUEST 0x5A


#define USB_1_0_STANDARD 0x0100
#define USB_1_1_STANDARD 0x0101
#define USB_2_0_STANDARD 0x0200

#define INTERRUPT_EP_NUM 0x01
#define EP_ADDR_IN  0x80
#define EP_ADDR_OUT 0x00

#define EP_ADDR_INTERRUPT (EP_ADDR_IN | (0x0F & INTERRUPT_EP_NUM))

#define EP_INTERRUPT_MAX_SIZE 2
#define IO_DATA_BUFF_LEN 2

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
	.wMaxPacketSize = EP_INTERRUPT_MAX_SIZE,
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

// Buffer to be used for control requests.
uint8_t usbd_control_buffer[128];

// buffer used to send the values of the input ports back to the host
uint8_t io_data_buffer[IO_DATA_BUFF_LEN];

uint16_t usb_set_interrupt_data(uint16_t pin_state)
{
	// NOTE:        interrupt_pending_buffer[]
	// The first two bytes of interrupt_pending_buffer hold the state of input pins
	//   interrupt_pending_buffer[0] == input pins 16, 15, 14, 13, 12, 11, 10, 09
	//   interrupt_pending_buffer[1] == input pins 08, 07 ,06 ,05 ,04 ,03 ,02 ,01

	// buffer holding the data for interrupt endpoint
	uint8_t interrupt_pending_buffer[EP_INTERRUPT_MAX_SIZE];
	// store the current state of the pins
	interrupt_pending_buffer[0] = pin_state >> 8;
	interrupt_pending_buffer[1] = pin_state & 0xFF;
	// set the data to be sent
	return usbd_ep_write_packet(global_usb_dev_handle, EP_ADDR_INTERRUPT, interrupt_pending_buffer, EP_INTERRUPT_MAX_SIZE);
}

static int get_io_control_callback(usbd_device *usbd_dev,
	struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
	void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	register uint16_t pin_state;
	if (req->bRequest != IO_REQUEST) {
		// not handling any other request other than IO_REQUEST
		return USBD_REQ_NEXT_CALLBACK;
	}
	pin_state = get_inputs();
	io_data_buffer[0] = pin_state >> 8;
	io_data_buffer[1] = pin_state & 0xFF;
	*buf = io_data_buffer;
	*len = IO_DATA_BUFF_LEN;

	return USBD_REQ_HANDLED;
	(void)usbd_dev;
	(void)complete;
}

static int set_io_control_callback(usbd_device *usbd_dev,
	struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
	void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	if (req->bRequest != IO_REQUEST) {
		// not handling any other request other than IO_REQUEST
		return USBD_REQ_NEXT_CALLBACK;
	}
	// set the output ports
	set_outputs((req->wValue & 0x10) ? true : false, (req->wValue & 0x0F));

	return USBD_REQ_HANDLED;
	(void)usbd_dev;
	(void)buf;
	(void)len;
	(void)complete;
}

static void set_config(usbd_device *usbd_dev, uint16_t wValue)
{
	usbd_ep_setup(usbd_dev,
		EP_ADDR_INTERRUPT,
		USB_ENDPOINT_ATTR_INTERRUPT,
		EP_INTERRUPT_MAX_SIZE,
		NULL);
	usbd_register_control_callback(usbd_dev,
		USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_IN,
		USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE,
		get_io_control_callback);
	usbd_register_control_callback(usbd_dev,
		USB_REQ_TYPE_VENDOR,
		USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE,
		set_io_control_callback);
	(void)wValue;
}

void usb_reenumerate(void)
{
	nvic_disable_irq(USB_IRQ);
	// reset usb hardware
	usb_reset_hardware();
	// init USB hardware
	usb_platform_init();
	// set ISR priority
	nvic_set_priority(USB_IRQ, IRQ_PRI_USB);
	// Initilize the USB dev handle
	global_usb_dev_handle = usbd_init(&USB_DRIVER, &dev,
		&config, usb_strings, sizeof(usb_strings)/sizeof(char *),
		usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(global_usb_dev_handle, set_config);
	// enable interrupt
	nvic_enable_irq(USB_IRQ);
	// perform the re-enumerate steps
	usb_platform_reenumerate();
}

void USB_ISR(void)
{
	usbd_poll(global_usb_dev_handle);
}
