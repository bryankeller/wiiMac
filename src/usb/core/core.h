/*
 * Copyright (c) 2007, Benedikt Sauter <sauter@ixbat.de>
 * All rights reserved.
 *
 * Short descripton of file:
 *
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 *
 *	 * Redistributions of source code must retain the above copyright 
 *		 notice, this list of conditions and the following disclaimer.
 *	 * Redistributions in binary form must reproduce the above 
 *		 copyright notice, this list of conditions and the following 
 *		 disclaimer in the documentation and/or other materials provided 
 *		 with the distribution.
 *	 * Neither the name of the FH Augsburg nor the names of its 
 *		 contributors may be used to endorse or promote products derived 
 *		 from this software without specific prior written permission.
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
	plugmii core

Copyright (C) 2009     Bernhard Urban <lewurm@gmx.net>
Copyright (C) 2009     Sebastian Falbesoner <sebastian.falbesoner@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef _CORE_H_
#define _CORE_H_

#include "../../types.h"
#include "../lib/list.h"


#include "../../bootmii_ppc.h"
inline static void wait_ms(int ms)
{
	while(ms--)
		udelay(1000);
}

struct usb_device {
	u8 address;
	u8 fullspeed;
	u32 ohci;

	/* device descriptor */
	u8 bLength;
	u8 bDescriptorType;
	u16 bcdUSB;
	u8 bDeviceClass;
	u8 bDeviceSubClass;
	u8 bDeviceProtocoll;
	u8 bMaxPacketSize0;
	u16 idVendor;
	u16 idProduct;
	u16 bcdDevice;
	u8 iManufacturer;
	u8 iProduct;
	u8 iSerialNumber;
	u8 bNumConfigurations;

	u8 epSize[16];
	u8 epTogl[16];

	struct usb_conf *conf;
	struct usb_device *next;
};

struct usb_conf {
	u8 bLength;
	u8 bDescriptorType;
	u16 wTotalLength;
	u8 bNumInterfaces;
	u8 bConfigurationValue;
	u8 iConfiguration;
	u8 bmAttributes;
	u8 bMaxPower;

	struct usb_conf *next;
	struct usb_intf *intf;
};

struct usb_intf {
	u8 bLength;
	u8 bDescriptorType;
	u8 bInterfaceNumber;
	u8 bAlternateSetting;
	u8 bNumEndpoints;
	u8 bInterfaceClass;
	u8 bInterfaceSubClass;
	u8 bInterfaceProtocol;
	u8 iInterface;

	struct usb_intf *next;
	struct usb_endp *endp;
};

struct usb_endp {
	u8 bLength;
	u8 bDescriptorType;
	u8 bEndpointAddress;
	u8 bmAttributes;
	u16 wMaxPacketSize;
	u8 bInterval;

	struct usb_endp *next;
};

struct usb_endpoint {
	u8 type;
	u8 size;
	u8 togl;
	struct usb_endpoint *next;
};

struct usb_transfer_descriptor_ep {
	struct usb_transfer_descriptor_ep *next;
	u8 device_address;
	u8 endpoint;
	struct usb_transfer_descriptor *start;
};

/**
 * USB Driver data structure
 */
struct usb_driver {
	char* name;
	void (*probe)(void);
	void (*check)(void);
	void (*remove)(void);
	void *data;
	struct usb_driver *next;
};


/**
 * I/O Request Block
 */

struct usb_irp {
	struct usb_device *dev;
	/* ep -> bit 7 is for direction 1=from	dev to host */
	u8 endpoint;
	u8 epsize;
	/* control, interrupt, bulk or isochron */
	u8 type;

	u8 *buffer;
	u16 len;

	//list * td_list;
	u16 timeout;
};


/**
 * usb transfer descriptor
 */
struct usb_transfer_descriptor {
	u8 devaddress;
	u8 endpoint;
	u8 fullspeed;
	u8 type;
	
	// TODO: zusammenfassen!
	u8 pid;
	u8 iso;
	u8 togl;	
	
	u8 *buffer;
	u16 actlen;
	
	u8 state;
	struct usb_transfer_descriptor *next;
	u8 maxp;
};

struct usb_core {
	u8 nextaddress;
	void (*stdout)(char * arg); 
	// driver list
	struct list *drivers;
	struct list *devices;
} core;

void usb_init(u32 reg);
void usb_periodic();
u8 usb_next_address();


struct usb_device *usb_add_device(u8 lowspeed, u32 reg);
u8 usb_remove_device(struct usb_device *dev);
u8 usb_register_driver(struct usb_driver *driver);
void usb_probe_driver();

void lsusb(struct usb_device *dev);

struct usb_irp *usb_get_irp();
u8 usb_remove_irp(struct usb_irp *irp);
u16 usb_submit_irp(struct usb_irp *irp);


struct usb_transfer_descriptor *usb_create_transfer_descriptor(struct usb_irp *irp);


#define USB_IRP_WAITING		1


#define USB_TRANSFER_DESCR_NONE 1
#define USB_TRANSFER_DESCR_SEND 2

#endif	//_CORE_H_
