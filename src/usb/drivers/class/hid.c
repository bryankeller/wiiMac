/*
	ppcskel - a Free Software replacement for the Nintendo/BroadOn bootloader.
	hid driver

Copyright (C) 2009     Bernhard Urban <lewurm@gmx.net>
Copyright (C) 2009     Sebastian Falbesoner <sebastian.falbesoner@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "../../core/core.h"
#include "../../core/usb.h"
#include "../../usbspec/usb11spec.h"
#include "../../../malloc.h"
#include "../../../string.h"

#include "hid.h"

struct usb_driver hidkb = {
	.name	  = "hidkb",
	.probe  = usb_hidkb_probe,
	.check  = usb_hidkb_check,
	.remove = usb_hidkb_remove,
	.data	  = NULL
};

/*
 * just using two very simple US layout code translation tables that
 * are sufficient for providing a getc() C standard library call;
 * the only non-printable character here in this table is ESCAPE which
 * has index 0x29 and is zero
 */
unsigned char code_translation_table[2][57] = {
	{ /* unshifted */
	 0,   0,   0,   0,  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
	'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
	'3', '4', '5', '6', '7', '8', '9', '0', '\n', 0,  '\r','\t',' ', '-', '=', '[',
	']', '\\','\\',';', '\'','`', ',', '.', '/'
	},
	{ /* shifted */
	 0,   0,   0,   0,  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',
	'#', '$', '%', '^', '&', '*', '(', ')', '\n', 0,  '\r','\t',' ', '_', '+', '{',
	'}', '|', '|', ':', '\"','~', '<', '>', '?'
	}
};


void usb_hidkb_init()
{
	usb_register_driver(&hidkb);
}

void usb_hidkb_probe()
{
	struct usb_device *dev;
	struct element *iterator = core.devices->head;
	
	while(iterator != NULL) {
		dev = (struct usb_device*)iterator->data;
		if(dev == NULL) {
			continue;
		}

		if(dev->conf->intf->bInterfaceClass == HID_CLASSCODE &&
				dev->conf->intf->bInterfaceSubClass == 1 && /* keyboard support boot protocol? */
				dev->conf->intf->bInterfaceProtocol == 1) { /* keyboard? */
			hidkb.data = (void*) dev;
			usb_hidkb_set_idle(dev, 1);
		}

		iterator=iterator->next;
	}
}

void usb_hidkb_set_idle(struct usb_device *dev, u8 duration) {
#define SET_IDLE 0x0A
	u8 buf[8];
	memset(buf, 0, 8);
	usb_control_msg(dev, 0x21, SET_IDLE, (duration << 8), 0, 0, buf, 0);
	hexdump((void*) buf, 8);
}

void usb_hidkb_check()
{
}

u8 usb_hidkb_inuse()
{
	return hidkb.data ? 1 : 0;
}

void usb_hidkb_remove() {
	hidkb.data = NULL;
}

struct kbrep *usb_hidkb_getChars() {
	struct usb_device *dev = (struct usb_device*) hidkb.data;
	struct kbrep *ret = (struct kbrep*) malloc(sizeof(struct kbrep));

	memset(ret, 0, 8);
	s8 epnum = dev->conf->intf->endp->bEndpointAddress & 0xf;
	(void) usb_interrupt_read(dev, epnum, (u8*) ret, 8, 0);
#if 0
	printf("============\nusb_interrupt_read:\n");
	hexdump((void*)ret, 8);
#endif
	return ret;
}

unsigned char usb_hidkb_get_char_from_keycode(u8 keycode, int shifted)
{
	unsigned char result = 0;
	if (keycode >= 0x3 && keycode < 57) {
		result = code_translation_table[!!shifted][keycode];
	}
	return result;
}
