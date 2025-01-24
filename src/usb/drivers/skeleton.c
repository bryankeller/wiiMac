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
#include <core/core.h>
#include <core/usb.h>
#include <usbspec/usb11spec.h>

#define MAX_DEVICES;

usb_device * devices[MAX_DEVICES];
u8 devices_in_use;

usb_driver skeleton = {
	.name    = "skeleton",
	.probe   = usb_skeleton_probe,
	.unprobe = usb_skeleton_unprobe,
	.check   = usb_skeleton_check,
	.data    = NULL
};

/* Zum Treiber initialisieren und anmelden am Stack */
void usb_skeleton_init()
{
	devices_in_use = 0;
	usb_register_driver(&skeleton);
}

/* Prüfen ob neues Gerät vom Treiber aus angesteuert werden kann */
void usb_skeleton_probe()
{
	usb_device * tmp;
	tmp = usb_open(0x1234,0x9876);
	if(tmp!=NULL) {
		/* neues Gerät gefunden */

		/* wenn Gerät noch nicht in interner Datenstruktur */
		if(devices_in_use<MAX_DEVICES)
			devices[devices_in_use++] = tmp;
	}

}

/* Entferntes Gerät aus Treiberstrukturen löschen */
void usb_skeleton_unprobe(usb_device * dev)
{
	u8 i;
	for(i=0;i<MAX_DEVICES;i++) {
		if(devices[i]==dev)
			usb_close(dev);
	}
}

/* Für periodische Endpunkte, Verwaltungs- und Steuerungsaufgaben */
void usb_skeleton_check(){

