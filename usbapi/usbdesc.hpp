/*
 * usbdesc.hpp
 *
 *  Created on: Dec 3, 2023
 *      Author: kosa
 */

#ifndef USBAPI_USBDESC_HPP_
#define USBAPI_USBDESC_HPP_

namespace USBDESC {
	

enum class tusb_dir : uint8_t
{
  DIR_OUT = 0,
  DIR_IN  = 1,
};

enum class request_type : uint8_t
{
  REQ_TYPE_STANDARD = 0,
  REQ_TYPE_CLASS,
  REQ_TYPE_VENDOR,
  REQ_TYPE_INVALID
};

enum class request_recipient : uint8_t
{
  REQ_RCPT_DEVICE =0,
  REQ_RCPT_INTERFACE,
  REQ_RCPT_ENDPOINT,
  REQ_RCPT_OTHER
};

struct [[gnu::packed]] control_request_t {
  union {
    struct [[gnu::packed]] {
      request_recipient recipient :  5;
      request_type type      :  2;
      tusb_dir direction :  1;
    } bmRequestType_bit;

    uint8_t bmRequestType;
  };

  uint8_t  bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
};

enum class USB_DESC_TYPE {
	DEVICE = 1,
	CONFIGURATION = 2,
	INTERFACE = 4,
	ENDPOINT = 5,
};


struct [[gnu::packed]] usb_desc_header {
    uint8_t bLength;
    uint8_t bDescriptorType;
};

struct [[gnu::packed]] usb_config {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationvalue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t MaxPower;
};

struct [[gnu::packed]] usb_interface {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
};

struct [[gnu::packed]] usb_endpoint {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
};

}




#endif /* USBAPI_USBDESC_HPP_ */
