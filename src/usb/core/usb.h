/*
 * Copyright (c) 2006, Benedikt Sauter <sauter@ixbat.de>
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

#ifndef _USB_H_
#define _USB_H_

#include "../../types.h"
#include "core.h"


/******************* Device Operations **********************/

// use an own usb device
struct usb_device *usb_open(u32 vendor_id, u32 product_id);
struct usb_device *usb_open_class(u8 class);

s8 usb_close(struct usb_device *dev);


/**
 * usb_reset resets the specified device by sending a RESET down the port 
 * it is connected to. Returns 0 on success or < 0 on error.
 */

s8 usb_reset(struct usb_device *dev);


/******************* Control Transfer **********************/
s8 usb_control_msg(struct usb_device *dev, u8 requesttype, u8 request, u16 value, u16 index, u16 length, u8 *buf, u16 timeout);
s8 usb_get_descriptor(struct usb_device *dev, u8 type, u8 index, u8 *buf, u8 size);
s8 usb_get_desc_dev_simple(struct usb_device *dev);
s8 usb_get_desc_dev(struct usb_device *dev);
s8 usb_get_desc_configuration(struct usb_device *dev, u8 index, struct usb_conf *conf);
s8 usb_get_desc_config_ext(struct usb_device *dev, u8 index, struct usb_conf *conf);

char *usb_get_string_simple(struct usb_device *dev, u8 index);
s8 usb_get_string(struct usb_device *dev, u8 index, u8 langid);

s8 usb_set_address(struct usb_device *dev, u8 address);
u8 usb_get_configuration(struct usb_device *dev);
s8 usb_set_configuration(struct usb_device *dev, u8 configuration);
s8 usb_set_altinterface(struct usb_device *dev, u8 alternate);


/******************* Bulk Transfer **********************/
s8 usb_bulk_write(struct usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout);
s8 usb_bulk_read(struct usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout);


/******************* Interrupt Transfer **********************/
s8 usb_interrupt_write(struct usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout);
s8 usb_interrupt_read(struct usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout);


/******************* Isochron Transfer **********************/
s8 usb_isochron_write(struct usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout);
s8 usb_isochron_read(struct usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout);

#endif	//_USB_H_
