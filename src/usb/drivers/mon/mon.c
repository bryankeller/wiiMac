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
#if USBMON

#include <string.h>

#include "mon.h"
#include <core/core.h>
#include <uart.h>

void usb_mon_stdout(void *output)
{
  core.stdout = output;
  usbmon_index=0;
  usbmon_loop=0;
}

void usb_mon_stdin(unsigned char input)
{
  char send[3]={input,0,0};
  usbmon_buffer[usbmon_index++]=input;
  // if enter is pressed
  if(input==0xD){
    usbmon_buffer[--usbmon_index]=0;
    usb_mon_command();
    usbmon_index=0;
  } else {
    core.stdout(send);
  }
 
  if((usbmon_loop==0 && input==0xD)){ 
    send[0]='\n';
    send[1]='\r';
    core.stdout(send);
    core.stdout("usbmon> ");
  } 

  if((usbmon_loop==1 && input==0xD)){
    usbmon_loop=0;
  }
}

void usb_mon_command()
{
  if(strcmp("list",(char*)usbmon_buffer)==0){
    core.stdout("\r\nDevice list:\r\n");
  } else if (strcmp("mon",(char*)usbmon_buffer)==0) {
    core.stdout("\r\n0:02:Bo 00 93 04");
    core.stdout("\r\n0:02:Bo 00 93 04");
    usbmon_loop=1;
  } else if (strcmp("help",(char*)usbmon_buffer)==0) {
    usb_mon_usage();
  } else if (strcmp(".",(char*)usbmon_buffer)==0) {
    usbmon_loop=0;
  }
  else {
    if(usbmon_index>0)
      core.stdout("\r\nunkown command (type help)");
  }

}

void usb_mon_usage()
{
  core.stdout("\r\nUsage:\r\n");
  core.stdout("\tlist\t Print device list\r\n");
  core.stdout("\tmon\t View online traffic\r\n");
}



#endif
