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


void usb_hub_probe();
void usb_hub_check();


struct usb_driver hub = {
	.name	  = "hub",
	.probe  = usb_hub_probe,
	.check  = usb_hub_check,
	.data	  = NULL
};

void usb_hub_init()
{
	usb_register_driver(&hub);	
}


void usb_hub_probe()
{
	// schaue ob aktuell enumeriertes geraet ein hub ist
	#if DEBUG
	core.stdout("Probe: Hub\r\n");
	#endif
	wait_ms(1000);
	
	struct usb_device *dev = usb_open_class(HUB_CLASSCODE);
	if(dev != NULL){
		hub.data = (void*)dev;		/* save handle */
		#if DEBUG
		core.stdout("Hub: Found Hub Device\r\n");
		#endif 
	 
		/* install int in EP */

	}



}


void usb_hub_check()
{
	// usb_read_interrupt(handle,1,buf); 
	// ah neue geraet gefunden
	// mit request den port geziel reseten, damit device adresse 0 hat
	// enumeration start
	// usb_add_device()
	//
	//
	// ah geraet entfernt
	// usb_remove_device(h1);

}


u8 usb_hub_get_hub_descriptor(struct usb_device *dev, char * buf)
{
	return 0;  
}

u8 usb_hub_get_hub_status(struct usb_device *dev, char *buf)
{


	return 0;  
}


u8 usb_hub_get_port_status(struct usb_device *dev, char *buf)
{


	return 0;  
}

u8 usb_hub_clear_port_feature(struct usb_device *dev)
{

	return 0;  
}

u8 usb_hub_set_port_feature(struct usb_device *dev, u8 value)
{

	return 0;  
}

u8 usb_hub_clear_hub_feature(struct usb_device *dev)
{

	return 0;  
}

u8 usb_hub_set_hub_feature(struct usb_device *dev)
{

	return 0;  
}

u8 usb_hub_set_hub_descriptor(struct usb_device *dev)
{

	return 0;  
}

