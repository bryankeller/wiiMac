/*
	ppcskel - a Free Software replacement for the Nintendo/BroadOn bootloader.
	hid driver

Copyright (C) 2009     Bernhard Urban <lewurm@gmx.net>
Copyright (C) 2009     Sebastian Falbesoner <sebastian.falbesoner@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/


#ifndef __HID_H
#define __HID_H

#define MOD_lctrl (1<<0)
#define MOD_lshift (1<<1)
#define MOD_lalt (1<<2)
#define MOD_lwin (1<<3)
#define MOD_rctrl (1<<4)
#define MOD_rshift (1<<5)
#define MOD_ralt (1<<6)
#define MOD_rwin (1<<7)

struct kbrep {
	u8 mod;
	u8 reserved;
	u8 keys[6];
};

void usb_hidkb_probe();
void usb_hidkb_check();
void usb_hidkb_init();
u8 usb_hidkb_inuse();

struct kbrep *usb_hidkb_getChars();
unsigned char usb_hidkb_get_char_from_keycode(u8 keycode, int shifted);
void usb_hidkb_set_idle(struct usb_device *dev, u8 duration);
void usb_hidkb_remove();

#endif /* __HID_H */

