/*
 * Copyright (c) 2007, Benedikt Sauter <sauter@ixbat.de>
 * All rights reserved.
 *
 * Short descripton of file:
 * I take the function names and parameters mainly from
 * libusb.sf.net.
 *
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright 
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above 
 *     copyright notice, this list of conditions and the following 
 *     disclaimer in the documentation and/or other materials provided 
 *     with the distribution.
 *   * Neither the name of the FH Augsburg nor the names of its 
 *     contributors may be used to endorse or promote products derived 
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
	ppcskel - a Free Software replacement for the Nintendo/BroadOn bootloader.
	libusb like interface

Copyright (C) 2009     Bernhard Urban <lewurm@gmx.net>
Copyright (C) 2009     Sebastian Falbesoner <sebastian.falbesoner@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "usb.h"
#include "core.h"
#include "../host/host.h"
#include "../usbspec/usb11spec.h"
#include "../../malloc.h"
#include "../../string.h"

#define cleargbuf() memset(gbuf, 0, 0xffff)
/* internal global buffer */
static u8 gbuf[0xffff];

/******************* Device Operations **********************/
/**
 * Open a device with verndor- and product-id for a communication.
 */
struct usb_device *usb_open(u32 vendor_id, u32 product_id)
{
	struct usb_device* dev;
	struct element * iterator = core.devices->head;
	while(iterator != NULL) {
		dev = (struct usb_device*)iterator->data;
		
		if(dev->idVendor==vendor_id&&dev->idProduct==product_id)
			return dev;

		iterator=iterator->next;
	}

	return NULL;
}


/**
 * Open a device with an class code for a communication.
 */
struct usb_device *usb_open_class(u8 class)
{
	struct usb_device* dev;
	struct element * iterator = core.devices->head;
	while(iterator != NULL) {
		dev = (struct usb_device*)iterator->data;
		
		if(dev->bDeviceClass==class)
			return dev;

		iterator=iterator->next;
	}
	return NULL;
}

/* Close device after a communication.
 */
s8 usb_close(struct usb_device *dev)
{

	return 0;
}

s8 usb_reset(struct usb_device *dev)
{


	return 0;
}


/******************* Control Transfer **********************/
/**
 * Create a control transfer.
 */
s8 usb_control_msg(struct usb_device *dev, u8 requesttype, u8 request,
		u16 value, u16 index, u16 length, u8 *buf, u16 timeout)
{
	struct usb_irp *irp = (struct usb_irp*)malloc(sizeof(struct usb_irp));
	irp->dev = dev;
	irp->endpoint = 0;

	irp->epsize = dev->bMaxPacketSize0;
	irp->type = USB_CTRL;

	buf[0]=(u8)requesttype;
	buf[1]=(u8)request;
	buf[2]=(u8)(value);
	buf[3]=(u8)(value >> 8);
	buf[4]=(u8)(index);
	buf[5]=(u8)(index >> 8);
	buf[6]=(u8)(length);
	buf[7]=(u8)(length >> 8);

	irp->buffer = buf;
	irp->len = length;
	irp->timeout = timeout;

	usb_submit_irp(irp);
	free(irp);

	return 0;
}

s8 usb_get_descriptor(struct usb_device *dev, u8 type, u8 index, u8 *buf, u8 size)
{
	usb_control_msg(dev, 0x80, GET_DESCRIPTOR, (type << 8) | index, 0, size, buf, 0);
	return 0;
}

s8 usb_get_string(struct usb_device *dev, u8 index, u8 langid)
{

	return 0;
}

char *usb_get_string_simple(struct usb_device *dev, u8 index)
{
	cleargbuf();
	usb_get_descriptor(dev, STRING, index, gbuf, (u8) 8);
	usb_get_descriptor(dev, STRING, index, gbuf, gbuf[0]);

	char *str = (char*)malloc((gbuf[0]/2));
	memset(str, '\0', (gbuf[0]/2));

	u16 i;
	for(i=0; i<(gbuf[0]/2)-1; i++) {
		str[i] = gbuf[2+(i*2)];
	}

	return str;
}

/* ask first 8 bytes of device descriptor with this special 
 * GET Descriptor Request, when device address = 0
 */
s8 usb_get_desc_dev_simple(struct usb_device *dev)
{
	cleargbuf();
	usb_get_descriptor(dev, DEVICE, 0, gbuf, 8);

	if(!gbuf[7]) {
		printf("FU: %d\n", gbuf[7]);
		return -2;
	}
	dev->bMaxPacketSize0 = gbuf[7];
	return 0;
}

s8 usb_get_desc_dev(struct usb_device *dev)
{
	cleargbuf();
	if (usb_get_desc_dev_simple(dev) < 0) {
		return -1;
	}
	usb_get_descriptor(dev, DEVICE, 0, gbuf, gbuf[0]);

	dev->bLength = gbuf[0];
	dev->bDescriptorType = gbuf[1];
	dev->bcdUSB = (u16) (gbuf[3] << 8 | gbuf[2]);
	dev->bDeviceClass = gbuf[4];
	dev->bDeviceSubClass = gbuf[5];
	dev->bDeviceProtocoll = gbuf[6];
	dev->idVendor = (u16) (gbuf[9] << 8) | (gbuf[8]);
	dev->idProduct = (u16) (gbuf[11] << 8) | (gbuf[10]);
	dev->bcdDevice = (u16) (gbuf[13] << 8) | (gbuf[12]);
	dev->iManufacturer = gbuf[14];
	dev->iProduct = gbuf[15];
	dev->iSerialNumber = gbuf[16];
	dev->bNumConfigurations = gbuf[17];

	u8 i;
	struct usb_conf *conf = dev->conf = (struct usb_conf*) malloc(sizeof(struct usb_conf));
	for(i=0; i <= dev->bNumConfigurations; i++) {
		if(i!=0) {
			conf = (struct usb_conf*) malloc(sizeof(struct usb_conf));
		}
		usb_get_desc_config_ext(dev, i, conf);
	}

	return 0;
}

s8 usb_get_desc_configuration(struct usb_device *dev, u8 index, struct usb_conf *conf)
{
	cleargbuf();
	usb_get_descriptor(dev, CONFIGURATION, index, gbuf, 8);
	usb_get_descriptor(dev, CONFIGURATION, index, gbuf, gbuf[0]);

	conf->bLength = gbuf[0];
	conf->bDescriptorType = gbuf[1];
	conf->wTotalLength = (u16) (gbuf[3] << 8 | gbuf[2]);
	conf->bNumInterfaces = gbuf[4];
	conf->bConfigurationValue = gbuf[5];
	conf->iConfiguration = gbuf[6];
	conf->bmAttributes = gbuf[7];
	conf->bMaxPower = gbuf[8];
	conf->intf = NULL;

	return 0;
}

/* returns more information about CONFIGURATION, including
 * INTERFACE(s) and ENDPOINT(s)
 * usb_get_desc_configuration() must be called for this device before
 */
s8 usb_get_desc_config_ext(struct usb_device *dev, u8 index, struct usb_conf *conf)
{
	cleargbuf();

	usb_get_desc_configuration(dev, index, conf);
	usb_get_descriptor(dev, CONFIGURATION, index, gbuf, dev->conf->wTotalLength);

	u8 i,j,off=9;
	struct usb_intf *ifs = dev->conf->intf = (struct usb_intf*) malloc(sizeof(struct usb_intf));
	for(i=1; i <= dev->conf->bNumInterfaces; i++) {
		if(i!=1) {
			ifs->next = (struct usb_intf*) malloc(sizeof(struct usb_intf));
			ifs = ifs->next;
		}
		ifs->bLength = gbuf[off+0];
		ifs->bDescriptorType = gbuf[off+1];
		ifs->bInterfaceNumber = gbuf[off+2];
		ifs->bAlternateSetting = gbuf[off+3];
		ifs->bNumEndpoints = gbuf[off+4];
		ifs->bInterfaceClass = gbuf[off+5];
		ifs->bInterfaceSubClass = gbuf[off+6];
		ifs->bInterfaceProtocol = gbuf[off+7];
		ifs->iInterface = gbuf[off+8];

		off += 9;

		struct usb_endp *ep = ifs->endp = (struct usb_endp*) malloc(sizeof(struct usb_endp));
		for(j=1; j <= ifs->bNumEndpoints; j++) {
			/* skip HID Device Descriptor (see lsusb) */
			if(gbuf[off+1] == 33) {
				j--;
				off += 9;
				continue;
			}

			if(j!=1) {
				ep->next = (struct usb_endp*) malloc(sizeof(struct usb_endp));
				ep = ep->next;
			}

			ep->bLength = gbuf[off+0];
			ep->bDescriptorType = gbuf[off+1];
			ep->bEndpointAddress = gbuf[off+2];
			ep->bmAttributes = gbuf[off+3];
			ep->wMaxPacketSize = (u16) ((gbuf[off+5] << 8) | (gbuf[off+4]));
			ep->bInterval = gbuf[off+6];

			off += 7;
		}
	}
	return 0;
}

s8 usb_set_address(struct usb_device *dev, u8 address)
{
	cleargbuf();
	usb_control_msg(dev, 0x00, SET_ADDRESS, address, 0, 0, gbuf, 0);
	wait_ms(210);
	return 0;
}


u8 usb_get_configuration(struct usb_device *dev)
{
	cleargbuf();
	usb_control_msg(dev, 0x80, GET_CONFIGURATION, 0, 0, 4, gbuf, 0);
	printf("=============\nafter usb_get_configuration:\n");
	hexdump((void*) gbuf, 8);
	return gbuf[0];
}

s8 usb_set_configuration(struct usb_device *dev, u8 configuration)
{
	cleargbuf();
	usb_control_msg(dev, 0x00, SET_CONFIGURATION, configuration, 0, 0, gbuf, 0);
	printf("=============\nafter usb_set_configuration:\n");
	hexdump((void*) gbuf, 8);
	wait_ms(20);
	return 0;
}

s8 usb_set_altinterface(struct usb_device *dev, u8 alternate)
{

	return 0;
}



/******************* Bulk Transfer **********************/
/**
 * Write to an a bulk endpoint.
 */
s8 usb_bulk_write(struct usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout)
{
	struct usb_irp * irp = (struct usb_irp*)malloc(sizeof(struct usb_irp));
	irp->dev = dev;
	//irp->devaddress = dev->address;
	
	irp->endpoint = ep;
	irp->epsize = dev->epSize[ep]; // ermitteln
	irp->type = USB_BULK;

	irp->buffer = buf;
	irp->len = size;
	irp->timeout = timeout;

	usb_submit_irp(irp);
	free(irp);

	return 0;
}

/**
 * Read from an bulk endpoint.
 */
s8 usb_bulk_read(struct usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout)
{
	struct usb_irp * irp = (struct usb_irp*)malloc(sizeof(struct usb_irp));
	//irp->devaddress = dev->address;
	irp->dev = dev;
	
	irp->endpoint = ep | 0x80;	// from device to host
	irp->epsize = dev->epSize[ep]; // ermitteln
	irp->type = USB_BULK;

	irp->buffer = buf;
	irp->len = size;
	irp->timeout = timeout;

	usb_submit_irp(irp);
	free(irp);

	return 0;
}


/******************* Interrupt Transfer **********************/
/**
 * Write to an interrupt endpoint.
 */
s8 usb_interrupt_write(struct usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout)
{
	return 0;
}

/**
 * Read from an interrupt endpoint.
 */
s8 usb_interrupt_read(struct usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout)
{
	struct usb_irp *irp = (struct usb_irp*)malloc(sizeof(struct usb_irp));
	irp->dev = dev;
	irp->endpoint = ep; //wtf? |80; //from device to host
	irp->epsize = dev->epSize[ep]; // ermitteln
	irp->type = USB_INTR;

	irp->buffer = buf;
	irp->len = size;
	irp->timeout = timeout;

	usb_submit_irp(irp);
	free(irp);

	return 0;
}


/******************* Isochron Transfer **********************/

/**
 * Write to an isochron endpoint.
 */
s8 usb_isochron_write(struct usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout)
{

	return 0;
}

/**
 * Read from an isochron endpoint.
 */
s8 usb_isochron_read(struct usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout)
{


	return 0;
}


//#endif	//_USB_H_
