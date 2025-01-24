/* usb11spec.h
* Copyright (C) 2005  Benedikt Sauter
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef _USB11PSEC_H
#define _USB11SPEC_H

/*-------------------------------------------
 * device descriptor 
 * ------------------------------------------*/
struct usb_device_descriptor 
{
	unsigned char   bLength;
	unsigned char   bDescriptorType;
	unsigned short  bcdUSB;
	unsigned char   bDeviceClass;
	unsigned char   bDeviceSubClass;
	unsigned char   bDeviceProtocol;
	unsigned char   bMaxPacketSize0;
	unsigned short  idVendor;
	unsigned short  idProduct;
	unsigned short  bcdDevice;
	unsigned char   iManufacturer;
	unsigned char   iProduct;
	unsigned char   iSerialNumber;
	unsigned char   bNumConfigurations;
};

/*-------------------------------------------
 * configuration descriptor 
 * ------------------------------------------*/

struct usb_configuration_descriptor 
{
	unsigned char   bLength;
	unsigned char   bDescriptorType;
	unsigned short  wTotalLength;
	unsigned char   bNumInterfaces;
	unsigned char   bConfigurationValue;
	unsigned char   iConfiguration;
	unsigned char   bmAttributes;
	unsigned char   MaxPower;
};

/*-------------------------------------------
 * interface descriptor 
 * ------------------------------------------*/

struct usb_interface_descriptor 
{
	unsigned char   bLength;
	unsigned char   bDescriptorType;
	unsigned char   bInterfaceNumber;
	unsigned char   bAlternateSetting;
	unsigned char   bNumEndpoints;
	unsigned char   bInterfaceClass;
	unsigned char   bInterfaceSubClass;
	unsigned char   bInterfaceProtocol;
	unsigned char   iInterface;
};


/*-------------------------------------------
 * endpoint descriptor 
 * ------------------------------------------*/
struct usb_endpoint_descriptor 
{
	unsigned char   bLength;
	unsigned char   bDescriptorType;
	unsigned char   bEndpointAddress;
	unsigned char   bmAttributes;
	unsigned short  wMaxPacketSize;
	unsigned char   bIntervall;
};



/*-------------------------------------------
 * string descriptor 
 * ------------------------------------------*/
struct usb_string_descriptor 
{
	unsigned char   bLength;
	unsigned char   bDescriptorType;
	unsigned char*  String;
};




/*-------------------------------------------
 * USB Device Request
 * ------------------------------------------*/
typedef struct usb_device_request_t usb_device_request;
struct usb_device_request_t
{
	unsigned char bmRequestType;
	unsigned char bRequest;
	unsigned short wValue;
	unsigned short wIndex;	
	unsigned short wLength;
};

/*-------------------------------------------
 * Get Device Descriptor Request
 * ------------------------------------------*/
//char GetDeviceDescriptor[] {0x80,GET_DESCRIPTOR, DEVICE,0x00, 0x00,0x00,0x00,0x40};
//char GetDeviceDescriptor[] {0x80,GET_DESCRIPTOR, DEVICE,0x00, 0x00,0x00,0x00,0x40};
//char SetAddress[] {0x00,SET_ADDRESS, 0x00, address, 0x00,0x00,0x00,0x40};
//char SetAddress[] {0x00,SET_ADDRESS, 0x00, 0x00, 0x00,0x00,0x00,0x00};


/*------------------------------------------
 * transfer types 
 * ------------------------------------------*/

#define USB_CTRL		0x00
#define USB_ISOC		0x01
#define USB_BULK		0x02
#define USB_INTR		0x03

/*-------------------------------------------
 * device descriptor types 
 * ------------------------------------------*/
#define DO_STANDARD   0x00
#define DO_CLASS      0x20
#define DO_VENDOR     0x40

/*-------------------------------------------
 * standard requests for ep0 
 * ------------------------------------------*/

#define GET_STATUS	  	0x00 
#define CLR_FEATURE	  	0x01
#define SET_FEATURE	  	0x03
#define SET_ADDRESS	  	0x05
#define GET_DESCRIPTOR	  	0x06
#define SET_DESCRIPTOR	  	0x07
#define GET_CONFIGURATION 	0x08
#define SET_CONFIGURATION 	0x09
#define GET_INTERFACE	  	0x0A
#define SET_INTERFACE	  	0x0B

/*-------------------------------------------
 * descriptor types 
 * ------------------------------------------*/

#define DEVICE			0x01
#define CONFIGURATION		0x02
#define STRING			0x03
#define INTERFACE		0x04
#define ENDPOINT		0x05
#define DEVICEQUALIFIER		0x06 // only usb2.0
#define OTHERSPEEDCONFIGURATION 0x07 // only usb2.0


/*-------------------------------------------
 * pid fields 
 * ------------------------------------------*/

#define USB_PID_SOF     0x05
#define USB_PID_SETUP   0x0D
#define USB_PID_IN      0x09
#define USB_PID_OUT     0x01
#define USB_PID_DATA0   0x03
#define USB_PID_DATA1   0x0B
#define USB_PID_ACK     0x02
#define USB_PID_NACK    0x0A
#define USB_PID_STALL   0x0E
#define USB_PID_PRE     0x0C


#define HID_CLASSCODE 	0x03
#define MASS_STORAGE_CLASSCODE	0x08
#define HUB_CLASSCODE		0x09



#endif /* __USB11SPEC_H__ */
