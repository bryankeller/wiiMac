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

//#include <wait.h>
//#include <stdlib.h>

#include "../../core/core.h"
#include "../../core/usb.h"
#include "../../usbspec/usb11spec.h"
#include "../../../malloc.h"

#include "storage.h"


#define MAX_DEVICES 2

void usb_storage_probe();
void usb_storage_check();

struct usb_device *massstorage[MAX_DEVICES];
u16 sectorsize[MAX_DEVICES];
u8 massstorage_in_use;

struct usb_driver storage = {
	.name	  = "storage",
	.probe  = usb_storage_probe,
	.check  = usb_storage_check,
	.data	  = NULL
};

void usb_storage_init()
{
	massstorage_in_use = 0;
	usb_register_driver(&storage);	
}


void usb_storage_probe()
{
	// schaue ob aktuell enumeriertes geraet ein storage ist
	#if DEBUG
	core.stdout("Probe: Storage\r\n");
	#endif
 
	/* read interface descriptor for class code */
	u8 buf[32];
	
	struct usb_device* dev;
	struct element * iterator = core.devices->head;
	
	while(iterator != NULL) {
		dev = (struct usb_device*)iterator->data;

		/* get interface descriptor */
		usb_control_msg(dev, 0x80, GET_DESCRIPTOR,2, 0, 32, buf, 0);

		if(buf[14]==MASS_STORAGE_CLASSCODE){
			massstorage[massstorage_in_use] = dev;
			massstorage_in_use++;
			#if DEBUG
			core.stdout("Storage: Found Storage Device\r\n");
			#endif 

			/* here is only my lib driver test */
			usb_storage_open(0);
			usb_storage_inquiry(0);
			usb_storage_read_capacity(0);

			//char * buf = (char*)malloc(512);
			//free(buf);
			//char buf[512];
			//usb_storage_read_sector(0,1,buf);

			/* end of driver test */
		}

		iterator=iterator->next;
	}
}


void usb_storage_check()
{
	// wird periodisch augerufen
	// da ein mass storage aber keinen interrupt oder isochronen endpunkt
	// hat passiert hier nichts
}



u8 usb_storage_inquiry(u8 device)
{
	/* send cwb "usbc" */
	
	usb_storage_cbw *cbw = (usb_storage_cbw*)malloc(sizeof(usb_storage_cbw));
	cbw->dCBWSignature= 0x43425355;
	cbw->dCBWTag=0x826A6008;
	cbw->dCBWDataTransferLength=0x00000024;
	cbw->bCWDFlags=0x80;
	cbw->bCBWLun=0x00;
	cbw->bCBWCBLength=0x01;

	u8 i;
	for(i=0;i<16;i++)
		cbw->CBWCB[i]=0x00;

	cbw->CBWCB[0]=0x12; // 0x12 = INQUIRY

	usb_bulk_write(massstorage[device], 2, (u8*)cbw, 31, 0); 
	usb_bulk_read(massstorage[device], 1, (u8*)cbw, 36, 0); 
	usb_bulk_read(massstorage[device], 1, (u8*)cbw, 13, 0); 

	free(cbw);

	return 0;
}


u8 usb_storage_read_capacity(u8 device)
{
	/* send cwb "usbc" */

	u8 tmp[8];
	u8 i;
	usb_storage_cbw  * cbw = (usb_storage_cbw*)malloc(sizeof(usb_storage_cbw));

	usb_control_msg(massstorage[device], 0x02,1,0, 0x8100, 0,tmp, 0);

	cbw->dCBWSignature= 0x43425355;
	cbw->dCBWTag=0x826A6008;
	cbw->dCBWDataTransferLength=0x00000008;
	cbw->bCWDFlags=0x80;
	cbw->bCBWLun=0x00;
	cbw->bCBWCBLength=0x0A;

	for(i=0;i<16;i++)
		cbw->CBWCB[i]=0x00;

	cbw->CBWCB[0]=0x25; // 0x12 = INQUIRY

	usb_bulk_write(massstorage[device], 2, (u8*)cbw, 31, 0); 
	usb_bulk_read(massstorage[device], 1, (u8*)cbw, 8, 0); 
	usb_bulk_read(massstorage[device], 1, (u8*)cbw, 13, 0); 

	free(cbw);

	return 0;
}


u8 usb_storage_read_sector(u8 device, u32 sector, char * buf)
{
	/* send cwb "usbc" */
	u8 tmpbuf[] = {0x55,0x53,0x42,0x43,0x08,
		0xE0,0x63,0x82,0x00,0x02,
		0x00,0x00,0x80,0x00,0x0A,
		0x28,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x01,0x00,
		0x00,0x00,0x00,0x00,0x00,
		0x00,
		0x00,0x00,0x00,0x00,0x00};

	usb_bulk_write(massstorage[device], 2, tmpbuf, 31, 0);
	//usb_bulk_read(massstorage[device], 1, buf,64,0);
	//usb_bulk_read(massstorage[device], 1, buf, 13, 0); 

	
	return 0;
}


u8 usb_storage_write_sector(u8 device, u32 sector, char * buf)
{

	return 0;
}
