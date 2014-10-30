import usb.core
import usb.util

IO_REQUEST = 0x5A

def vendor_control(wValue, wIndex, idVendor=0x2AEB, idProduct=133, direction=usb.util.CTRL_OUT, recipient=usb.util.CTRL_RECIPIENT_OTHER, bRequest=IO_REQUEST, data=None):
    dev = usb.core.find(idVendor=idVendor, idProduct=idProduct)
    if dev is None:
        raise ValueError('Device not found')
    dev.set_configuration(configuration=1)
    # cfg = dev.get_active_configuration()
    # print(repr(cfg))
    result = dev.ctrl_transfer(
        bmRequestType=usb.util.build_request_type(
            direction,
            usb.util.CTRL_TYPE_VENDOR,
            recipient),
        bRequest=bRequest,
        wValue=wValue,
        wIndex=wIndex,
        data_or_wLength=data,
        timeout=1000)
    return result

def send_IO_vendor_control(wValue, wIndex, idVendor=0x2AEB, idProduct=133):
    return vendor_control(
        wValue,
        wIndex,
        idVendor=idVendor,
        idProduct=idProduct,
        direction=usb.util.CTRL_OUT,
        recipient=usb.util.CTRL_RECIPIENT_DEVICE,
        bRequest=IO_REQUEST,
        data=None)

def rcve_IO_vendor_control(wValue=0x00, wIndex=0x00, idVendor=0x2AEB, idProduct=133, data=None):
    return vendor_control(
        wValue,
        wIndex,
        idVendor=idVendor,
        idProduct=idProduct,
        direction=usb.util.CTRL_IN,
        recipient=usb.util.CTRL_RECIPIENT_DEVICE,
        bRequest=IO_REQUEST,
        data=data)

while (True):recieve = rcve_IO_vendor_control(data=2);print("0b{0:08b}, 0b{1:08b}".format(int(recieve[0]), int(recieve[1])))
send_IO_vendor_control(wValue=0x1F, wIndex=0x00)


dev = usb.core.find(idVendor=0x2AEB, idProduct=133)
dev.ctrl_transfer(bmRequestType=usb.util.build_request_type(usb.util.CTRL_IN, usb.util.CTRL_TYPE_STANDARD, usb.util.CTRL_RECIPIENT_DEVICE), bRequest=0x06, wIndex=0, data_or_wLength=20, wValue=(0x00|(0x01<<8)))
dev.ctrl_transfer(bmRequestType=usb.util.build_request_type(usb.util.CTRL_IN, usb.util.CTRL_TYPE_VENDOR, usb.util.CTRL_RECIPIENT_DEVICE), bRequest=IO_REQUEST, wIndex=0, data_or_wLength=2, wValue=0x00)

usb.core.Endpoint(device=dev, endpoint=0, interface=0, alternate_setting=0, configuration=0)

usb.core.Endpoint(device=usb.core.find(idVendor=0x2AEB, idProduct=133), endpoint=0, interface=0, alternate_setting=0, configuration=0).read(size_or_buffer=2, timeout=None)

'''
usb.util.build_request_type(direction, type, recipient)
        Build a bmRequestType field for control requests.
        
        These is a conventional function to build a bmRequestType
        for a control request.
        
        The direction parameter can be CTRL_OUT or CTRL_IN.
        The type parameter can be CTRL_TYPE_STANDARD, CTRL_TYPE_CLASS,
        CTRL_TYPE_VENDOR or CTRL_TYPE_RESERVED values.
        The recipient can be CTRL_RECIPIENT_DEVICE, CTRL_RECIPIENT_INTERFACE,
        CTRL_RECIPIENT_ENDPOINT or CTRL_RECIPIENT_OTHER.
        
        Return the bmRequestType value.
'''
'''
ctrl_transfer(bmRequestType, bRequest, wValue=0, wIndex=0, data_or_wLength=None, timeout=None) method of usb.core.Device instance
    Do a control transfer on the endpoint 0.
    
    This method is used to issue a control transfer over the endpoint 0
    (endpoint 0 is required to always be a control endpoint).
    
    The parameters bmRequestType, bRequest, wValue and wIndex are the same
    of the USB Standard Control Request format.
    
    Control requests may or may not have a data payload to write/read.
    In cases which it has, the direction bit of the bmRequestType
    field is used to infere the desired request direction. For
    host to device requests (OUT), data_or_wLength parameter is
    the data payload to send, and it must be a sequence type convertible
    to an array object. In this case, the return value is the number
    of bytes written in the data payload. For device to host requests
    (IN), data_or_wLength is either the wLength parameter of the control
    request specifying the number of bytes to read in data payload, and
    the return value is an array object with data read, or an array
    object which the data will be read to, and the return value is the
    number of bytes read.
'''

'''
set_configuration(configuration=None) method of usb.core.Device instance
    Set the active configuration.
    
    The configuration parameter is the bConfigurationValue field of the
    configuration you want to set as active. If you call this method
    without parameter, it will use the first configuration found.  As a
    device hardly ever has more than one configuration, calling the method
    without arguments is enough to get the device ready.
'''
'''
usb.legacy
    interruptRead(self, endpoint, size, timeout=100)
        Performs a interrupt read request to the endpoint specified.
        
        Arguments:
            endpoint: endpoint number.
            size: number of bytes to read.
            timeout: operation timeout in miliseconds. (default: 100)
        Return a tuple with the data read.
    
    interruptWrite(self, endpoint, buffer, timeout=100)
        Perform a interrupt write request to the endpoint specified.
        
        Arguments:
            endpoint: endpoint number.
            buffer: sequence data buffer to write.
                    This parameter can be any sequence type.
            timeout: operation timeout in miliseconds. (default: 100)
                     Returns the number of bytes written.
'''

'''
while true;do dd if=/dev/ntserusb0 bs=2 count=1 2>/dev/null | hexdump -C | grep -e '^00000000  ' -e '^*';done
printf "\x1F" > /dev/ntserusb0
'''