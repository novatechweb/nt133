import usb.core
import usb.util
import array

NT133_USB_REQ_INPUT_STATUS = 0x10
NT133_USB_REQ_OUTPUT_STATUS  =0x11
NT133_USB_REQ_OUTPUT_CTRL = 0x12
NT133_USB_REQ_OUTPUT_ENABLE = 0x13

idVendor = 0x2AEB
idProduct = 133

timeout = 1000

def open_dev():
    dev = usb.core.find(idVendor=idVendor, idProduct=idProduct)
    if dev is None:
        raise ValueError('Device not found')
    return dev

dev = open_dev()

def set_config():
    dev.set_configuration(configuration=1)
    cfg = dev.get_active_configuration()
    print(repr(cfg))

def enable_relays(wValue=False):
    result = dev.ctrl_transfer(
        bmRequestType=usb.util.build_request_type(
            usb.util.CTRL_OUT,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_DEVICE),
        bRequest=NT133_USB_REQ_OUTPUT_ENABLE,
        wValue=wValue,
        wIndex=0,
        data_or_wLength=None,
        timeout=timeout)
    return result

def set_output(output_index, output_set=False, pulse_msec=0):
    data = array.array('B', [
        (pulse_msec & 0xFF000000) >> 24,
        (pulse_msec & 0x00FF0000) >> 16,
        (pulse_msec & 0x0000FF00) >> 8,
        (pulse_msec & 0x000000FF) >> 0,
         ])
    result = dev.ctrl_transfer(
        bmRequestType=usb.util.build_request_type(
            usb.util.CTRL_OUT,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_DEVICE),
        bRequest=NT133_USB_REQ_OUTPUT_CTRL,
        wValue=output_set,
        wIndex=output_index,
        data_or_wLength=data,
        timeout=timeout)
    return result

def get_inputs():
    result = dev.ctrl_transfer(
        bmRequestType=usb.util.build_request_type(
            usb.util.CTRL_IN,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_DEVICE),
        bRequest=NT133_USB_REQ_INPUT_STATUS,
        wValue=0,
        wIndex=0,
        data_or_wLength=2,
        timeout=timeout)
    print("0b{0:08b}, 0b{1:08b}".format(int(result[0]), int(result[1])))
    return result

def get_timer(output_index):
    result = dev.ctrl_transfer(
        bmRequestType=usb.util.build_request_type(
            usb.util.CTRL_IN,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_DEVICE),
        bRequest=NT133_USB_REQ_OUTPUT_STATUS,
        wValue=0,
        wIndex=output_index,
        data_or_wLength=4,
        timeout=timeout)
    print(result)
    result = (result[0] * 16777216) + (result[1] * 65536) + (result[2] * 256) + result[3];
    print("milliseconds remaining on timer for output {0}: {1}".format(output_index, result))
    return result

def get_interrupt(length=2):
    endpoint = usb.core.Endpoint(
        device=dev,
        endpoint=0,
        interface=0,
        alternate_setting=0,
        configuration=0)
    result = endpoint.read(length)
    print("0b{0:08b}, 0b{1:08b}".format(int(result[0]), int(result[1])))
    return result



dev.ctrl_transfer(bmRequestType=usb.util.build_request_type(usb.util.CTRL_IN, usb.util.CTRL_TYPE_STANDARD, usb.util.CTRL_RECIPIENT_DEVICE), bRequest=0x06, wIndex=0, data_or_wLength=20, wValue=(0x00|(0x01<<8)))


'''
while true;do dd if=/dev/ntserusb0 bs=2 count=1 2>/dev/null | hexdump -C | grep -e '^00000000  ' -e '^*';done
printf "\x1F" > /dev/ntserusb0
'''

'''
prev = [0,0]
while (True):
    recieve = rcve_IO_vendor_control(data=2);
    if recieve[0] is not prev[0] or recieve[1] is not prev[1]:
        prev = recieve
        print("0b{0:08b}, 0b{1:08b}".format(int(recieve[0]), int(recieve[1])))

'''