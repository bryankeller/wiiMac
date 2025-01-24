/*
 * Copyright (c) 2006, Benedikt Sauter <sauter@ixbat.de>
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

#include "core.h"
#include "../host/host.h"
#include "usb.h"
#include "../usbspec/usb11spec.h"
#include "../lib/list.h"
#include "../../malloc.h"
#include "../../bootmii_ppc.h" //printf
#include "../../string.h" //memset

/**
 * Initialize USB stack.
 */
void usb_init(u32 reg)
{
	core.drivers = list_create();
	core.devices = list_create();
	core.nextaddress = 1;
	hcdi_init(reg);
}

/**
 * Get next free usb device address.
 */
u8 usb_next_address()
{
	u8 addr = core.nextaddress;
	core.nextaddress++;
	return addr;
}


/**
 * Call this function periodically for 
 * control and transfer management.
 */
void usb_periodic()
{
	// call ever registered driver	
	struct usb_driver *drv;
	struct element *iterator = core.drivers->head;
	while (iterator != NULL) {
		drv = (struct usb_driver *) iterator->data;
		drv->check();
		iterator = iterator->next;
	}
}


/** 
 * Enumerate new device and create data structures 
 * for the core. usb_add_device expected that
 * the device answers to address zero.
 */
struct usb_device *usb_add_device(u8 lowspeed, u32 reg)
{
	struct usb_device *dev = (struct usb_device *) malloc(sizeof(struct usb_device));
	dev->conf = (struct usb_conf *) malloc(sizeof(struct usb_conf));
	dev->address = 0;
	dev->fullspeed = lowspeed ? 0 : 1;
	/* send at first time only 8 bytes for lowspeed devices
	 * 64 bytes for fullspeed
	 */
	dev->bMaxPacketSize0 = lowspeed ? 8 : 64;
	dev->ohci = reg;

	dev->epSize[0] = 8;
	dev->epSize[1] = 64;
	dev->epSize[2] = 64;

	dev->epTogl[0] = 0;
	dev->epTogl[1] = 0;
	dev->epTogl[2] = 0;

	s8 ret;
	ret = usb_get_desc_dev_simple(dev);
	if(ret < 0) {
		return (void*) -1;
	}
//
//#define WTF
#ifdef WTF
	volatile u8 wzf = 11;
	if(0 == wzf) {
		printf("WTF WTF WTF WTF padding??? WTFWTF WTF\n");
		printf("WTF WTF WTF WTF padding??? WTF WTF WTF\n");
		printf("WTF TF WTF WTF padding??? WTF WTF WTF\n");
		printf("WTF WTF TF WTF padding??? WTF WTWTF\n");
		printf("TF WTF WTF WTF padding??? WTF WTF WTF\n");
		printf("WTF WTF WTF WT padding??? WTF WF WTF\n");
		printf("WTF WTF WTF WTF padding??? WTF WTF WTF\n");
		printf("WTF WTF WTF WTF padding??? WT WTF WTF\n");
		printf("WTF WTF WTF WTF pdding??? WTF WTF WTF\n");
		printf("WTF WTF WTF WTF paddin??? WTF WTF WTF\n");
		printf("WTF WTF WTF WTF padding??? WTF WTF WTF\n");
		printf("WTF WTF WTF WTF padding?? WT WTF WTF\n");
		printf("WTF WTF WTF WTF padding??? WTF WTF WTF\n");
		printf("WTF WTF WTF WTF padding??? WTFWTF WTF\n");
	}
#endif
	u8 address = usb_next_address();
	ret = usb_set_address(dev, address);
	dev->address = address;
	printf("set address to %d\n", dev->address);

	/* get device descriptor&co */
	ret = usb_get_desc_dev(dev);
	if(ret < 0)
		return (void*) -1;

	/* print device info */
	lsusb(dev);

	/* add device to device list */
	struct element *tmp = (struct element *) malloc(sizeof(struct element));
	tmp->data = (void *) dev;
	list_add_tail(core.devices, tmp);

	usb_probe_driver();

	return dev;
}

void lsusb(struct usb_device *dev)
{
	printf("=== Device Descriptor === \n");
	printf("bLength 0x%02X\n", dev->bLength);
	printf("bDescriptorType 0x%02X\n", dev->bDeviceClass);
	printf("bcdUSB 0x%04X\n", dev->bcdUSB);
	printf("bDeviceClass 0x%02X\n", dev->bDeviceClass);
	printf("bDeviceSubClass 0x%02X\n", dev->bDeviceSubClass);
	printf("bDeviceProtocoll 0x%02X\n", dev->bDeviceProtocoll);
	printf("bMaxPacketSize 0x%02X\n", dev->bMaxPacketSize0);
	printf("idVendor 0x%04X\n", dev->idVendor);
	printf("idProduct 0x%04X\n", dev->idProduct);
	printf("bcdDevice 0x%04X\n", dev->bcdDevice);
	printf("iManufacturer(0x%02X): \"%s\"\n", dev->iManufacturer, dev->iManufacturer ? usb_get_string_simple(dev, dev->iManufacturer) : "no String");
	printf("iProduct(0x%02X): \"%s\"\n", dev->iProduct, dev->iProduct ? usb_get_string_simple(dev, dev->iProduct) : "no String");
	printf("iSerialNumber(0x%02X): \"%s\"\n", dev->iSerialNumber, dev->iSerialNumber ? usb_get_string_simple(dev, dev->iSerialNumber) : "no String");
	printf("bNumConfigurations 0x%02X\n", dev->bNumConfigurations);

	u8 c, i, e;
	struct usb_conf *conf = dev->conf;
	for(c=0; c <= dev->bNumConfigurations; c++) {
		printf("  === Configuration Descriptor %d ===\n", c+1);
		printf("  bLength 0x%02X\n", conf->bLength);
		printf("  bDescriptorType 0x%02X\n", conf->bDescriptorType);
		printf("  wTotalLength 0x%04X\n", conf->wTotalLength);
		printf("  bNumInterfaces 0x%02X\n", conf->bNumInterfaces);
		printf("  bConfigurationValue 0x%02X\n", conf->bConfigurationValue);
		printf("  iConfiguration (0x%02X): \"%s\"\n", conf->iConfiguration, conf->iConfiguration ? usb_get_string_simple(dev, conf->iConfiguration) : "no String");
		printf("  bmAttributes 0x%02X\n", conf->bmAttributes);
		printf("  bMaxPower 0x%02X\n", conf->bMaxPower);

		struct usb_intf *ifs = conf->intf;
		for(i=1; i <= conf->bNumInterfaces; i++) {
			printf("    === Interface Descriptor %d ===\n", i);
			printf("    bLength 0x%02X\n", ifs->bLength);
			printf("    bDescriptorType 0x%02X\n", ifs->bDescriptorType);
			printf("    bInterfaceNumber 0x%02X\n", ifs->bInterfaceNumber);
			printf("    bAlternateSetting 0x%02X\n", ifs->bAlternateSetting);
			printf("    bNumEndpoints 0x%02X\n", ifs->bNumEndpoints);
			printf("    bInterfaceClass 0x%02X\n", ifs->bInterfaceClass);
			printf("    bInterfaceSubClass 0x%02X\n", ifs->bInterfaceSubClass);
			printf("    bInterfaceProtocol 0x%02X\n", ifs->bInterfaceProtocol);
			printf("    iInterface (0x%02X): \"%s\"\n", ifs->iInterface, ifs->iInterface ? usb_get_string_simple(dev, ifs->iInterface) : "no String");

			struct usb_endp *ed = ifs->endp;
			for(e=1; e <= ifs->bNumEndpoints; e++) {
				printf("      === Endpoint Descriptor %d ===\n", e);
				printf("      bLength 0x%02X\n", ed->bLength);
				printf("      bDescriptorType 0x%02X\n", ed->bDescriptorType);
				printf("      bEndpointAddress 0x%02X\n", ed->bEndpointAddress);
				printf("      bmAttributes 0x%02X\n", ed->bmAttributes);
				printf("      wMaxPacketSize 0x%02X\n", ed->wMaxPacketSize);
				printf("      bInterval 0x%02X\n", ed->bInterval);

				ed = ed->next;
			} //endpoint

			ifs = ifs->next;
		} //interface

		conf = conf->next;
	} //configuration
}

/**
 * Find currently detached device and remove
 * data structures
 */
u8 usb_remove_device(struct usb_device *dev)
{
	/* trigger driver for this device */
	struct usb_driver *drv;
	struct element *iterator = core.drivers->head;
	while (iterator != NULL) {
		drv = (struct usb_driver *) iterator->data;
		if(drv->data && !memcmp(drv->data, dev, sizeof(struct usb_device))) {
			drv->remove();
			break;
		}
		iterator = iterator->next;
	}

	/* remove from device list */
	struct element *tmp = (struct element *) malloc(sizeof(struct element));
	tmp->data = (void *) dev;
	list_delete_element(core.devices, tmp);

	printf("REMOVED\n");

	return 1;
}

/**
 * Register new driver at usb stack.
 */
u8 usb_register_driver(struct usb_driver *dev)
{
	/* add driver to driver list */
	struct element *tmp = (struct element *) malloc(sizeof(struct element));
	tmp->data = (void *) dev;
	tmp->next = NULL;
	list_add_tail(core.drivers, tmp);

	/** 
	 * first check to find a suitable device 
	 * (root hub drivers need this call here)
	 */
	dev->probe();

	return 1;
}


/**
 * Call every probe function from every registered
 * driver, to check if there is a valid driver
 * for the new device.	
 */
void usb_probe_driver()
{
	// call ever registered driver	
	struct usb_driver *drv;
	struct element *iterator = core.drivers->head;
	while (iterator != NULL) {
		drv = (struct usb_driver *) iterator->data;
		drv->probe();
		iterator = iterator->next;
	}
}

/**
 * Not implemented.
 */
struct usb_irp *usb_get_irp()
{
	return 0;
}

/**
 * Not implemented.
 */
u8 usb_remove_irp(struct usb_irp *irp)
{

	return 1;
}

/**
 * Takes usb_irp and split it into
 * several usb packeges (SETUP,IN,OUT)
 * In the usbstack they are transported with the
 * usb_transfer_descriptor data structure.
 */
u16 usb_submit_irp(struct usb_irp *irp)
{
	struct usb_transfer_descriptor *td;
	u8 runloop = 1;
	u16 restlength = irp->len;
	u8 *td_buf_ptr = irp->buffer;
	u8 mybuf[64];

	u8 togl = irp->dev->epTogl[(irp->endpoint & 0x7F)];

	switch (irp->type) {
	case USB_CTRL:
		/* alle requests mit dem gleichen algorithmus zerteilen
		 * das einzige ist der spezielle get_Device_descriptor request
		 * bei dem eine laenge von 64 angegeben ist.
		 * wenn man an adresse 0 einen get_device_desciptor schickt
		 * dann reichen die ersten 8 byte.
		 */

		/***************** Setup Stage ***********************/
		td = usb_create_transfer_descriptor(irp);
		td->pid = USB_PID_SETUP;
		td->buffer = irp->buffer;

		/* control message are always 8 bytes */
		td->actlen = 8;

		togl = 0;
		/* start with data0 */
		td->togl = togl;
		togl = togl ? 0 : 1;

		/**** send token ****/
		hcdi_enqueue(td, irp->dev->ohci);

		/***************** Data Stage ***********************/
		/**
		 * You can see at bit 7 of bmRequestType if this stage is used,
		 * default requests are always 8 byte greate, from
		 * host to device. Stage 3 is only neccessary if the request
		 * expected datas from the device.
		 * bit7 - 1 = from device to host -> yes we need data stage
		 * bit7 - 0 = from host to device -> no send zero packet
		 *
		 * nach einem setup token kann nur ein IN token in stage 3 folgen
		 * nie aber ein OUT. Ein Zero OUT wird nur als Bestaetigung benoetigt.
		 *
		 *
		 * bit7 = 1
		 *	Device to Host
		 *	- es kommen noch Daten mit PID_IN an
		 *	- host beendet mit PID_OUT DATA1 Zero
		 * bit7 - 0
		 *	Host zu Device (wie set address)
		 *	- device sendet ein PID_IN DATA1 Zero Packet als bestaetigung
		 */
		memcpy(mybuf, irp->buffer, td->actlen);
		usb_device_request *setup = (usb_device_request *) mybuf;
		u8 bmRequestType = setup->bmRequestType;
		free(td);

		/* check bit 7 of bmRequestType */
		if (bmRequestType & 0x80) { 
			/* schleife die die tds generiert */
			while (runloop && (restlength > 0)) {
				td = usb_create_transfer_descriptor(irp);
				td->actlen = irp->epsize;
				/* stop loop if all bytes are send */
				if (restlength < irp->epsize) {
					runloop = 0;
					td->actlen = restlength;
				}

				td->buffer = td_buf_ptr;
				/* move pointer for next packet */
				td_buf_ptr += irp->epsize;

				td->pid = USB_PID_IN;
				td->togl = togl;
				togl = togl ? 0 : 1;

				/* wenn device descriptor von adresse 0 angefragt wird werden nur
				 * die ersten 8 byte abgefragt
				 */
				if (setup->bRequest == GET_DESCRIPTOR && (setup->wValue & 0xff) == 1
						&& td->devaddress == 0) {
					/* stop loop */
					runloop = 0;
				}

				/**** send token ****/
				hcdi_enqueue(td, irp->dev->ohci);

				/* pruefe ob noch weitere Pakete vom Device abgeholt werden muessen */
				restlength = restlength - irp->epsize;
				free(td);
			}
		}


		/***************** Status Stage ***********************/
		/* Zero packet for end */
		td = usb_create_transfer_descriptor(irp);
		td->togl = 1;								/* zero data packet = always DATA1 packet */
		td->actlen = 0;
		td->buffer = NULL;

		/**
		 * bit7 = 1, host beendet mit PID_OUT DATA1 Zero
		 * bit7 = 0, device sendet ein PID_IN DATA1 Zero Packet als bestaetigung
		 */
		/* check bit 7 of bmRequestType */
		if (bmRequestType & 0x80) {
			td->pid = USB_PID_OUT;
		} else {
			td->pid = USB_PID_IN;
		}
		/**** send token ****/
		hcdi_enqueue(td, irp->dev->ohci);
		free(td);
		break;

	case USB_BULK:
		//u8 runloop=1;
		//u16 restlength = irp->len;
		//char * td_buf_ptr=irp->buffer;

		/* schleife die die tds generiert */
		while (runloop) {
			td = usb_create_transfer_descriptor(irp);
			td->endpoint = td->endpoint & 0x7F;				/* clear direction bit */

			/* max packet size for given endpoint */
			td->actlen = irp->epsize;

			/* Generate In Packet  */
			if (irp->endpoint & 0x80)
				td->pid = USB_PID_IN;
			else
				/* Generate Out Packet */
				td->pid = USB_PID_OUT;

			/* stop loop if all bytes are send */
			if (restlength <= irp->epsize) {
				runloop = 0;
				td->actlen = restlength;
			}

			td->buffer = td_buf_ptr;
			/* move pointer for next packet */
			td_buf_ptr = td_buf_ptr + irp->epsize;

			td->togl = togl;
			togl = togl ? 0 : 1;
				/**** send token ****/
			hcdi_enqueue(td, irp->dev->ohci);
			free(td);
		}
		/* next togl */
		//if(td->pid == USB_PID_OUT) {
		//if(togl==0) togl=1; else togl=0;
		//}
		irp->dev->epTogl[(irp->endpoint & 0x7F)] = togl;

		break;
	
	case USB_INTR:
		//u8 runloop=1;
		//u16 restlength = irp->len;
		//char * td_buf_ptr=irp->buffer;

		/* schleife die die tds generiert */
		while (runloop && (restlength > 0)) {
			td = usb_create_transfer_descriptor(irp);
			/* max packet size for given endpoint */
			td->actlen = irp->epsize;

			td->pid = USB_PID_IN;
			/* TODO: USB_PID_OUT */

			/* stop loop if all bytes are send */
			if (restlength < irp->epsize) {
				runloop = 0;
				td->actlen = restlength;
			}

			td->buffer = td_buf_ptr;
			/* move pointer for next packet */
			td_buf_ptr += irp->epsize;

			td->togl = togl;
			togl = togl ? 0 : 1;
				
			/**** send token ****/
			hcdi_enqueue(td, irp->dev->ohci);
			restlength = restlength - irp->epsize;
			free(td);
		}
		break;
		irp->dev->epTogl[(irp->endpoint & 0x7F)] = togl;
	}
	hcdi_fire(irp->dev->ohci);

	return 1;
}



/** 
 * Create a transfer descriptor with an parent irp.
 */
struct usb_transfer_descriptor *usb_create_transfer_descriptor(struct usb_irp * irp)
{
	struct usb_transfer_descriptor *td =
			(struct usb_transfer_descriptor *) malloc(sizeof(struct usb_transfer_descriptor));

	td->devaddress = irp->dev->address;
	td->endpoint = irp->endpoint;
	td->iso = 0;
	td->state = USB_TRANSFER_DESCR_NONE;
	td->maxp = irp->epsize;
	td->fullspeed = irp->dev->fullspeed;
	td->type = irp->type;

	return td;
}

