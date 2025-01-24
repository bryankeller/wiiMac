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
//#include <stdlib.h>
#include "list.h"
#include "../../malloc.h"
#include "../../string.h"
#include "../../bootmii_ppc.h"

struct list* list_create()
{
	struct list *l = (struct list*)malloc(sizeof(struct list));
	l->head = NULL;
	return l;
}

u8 list_add_tail(struct list *l, struct element *e)
{
	e->next = NULL;

	/* if head is empty put first element here */
	if(l->head==NULL){
		l->head = e;
		return 1;
	}

	/* find last element */
	struct element *iterator = l->head;

	while(iterator->next!=NULL) {
		iterator = iterator->next;
	} 
	iterator->next = e;

	return 1;
}



// FIXME: untested
u8 list_delete_element(struct list *l, struct element *e)
{
	struct element *iterator = l->head;
	struct element *delete = NULL;

	if(!l->head) {
		return 0;
	} else {
		if(l->head->data && !(memcmp(l->head->data, e->data, sizeof(struct element)))) {
			delete = l->head;
			l->head = NULL;
		}
	}

	while(iterator->next!=NULL) {
		if(iterator->next->data && !(memcmp(iterator->next->data, e->data, sizeof(struct element)))) {
			delete = iterator->next;
			iterator->next = iterator->next->next;
			break;
		}

		iterator = iterator->next;
	} 

	if(delete) {
		free(delete->data);
		free(delete);
	}
	
	return 1;
}

// FIXME: untested and unused!! 
u8 list_is_element_last(struct list *l, struct element *e)
{
	if(e->next==NULL)
		return 1;
	else
		return 0;
}



// FIXME: untested and unused!! 
struct element *list_find_next_element(struct list *l, struct element *e)
{
	struct element *iterator = l->head;

	while(iterator!=NULL){
		if(iterator == e)
			return iterator->next;
		iterator = iterator->next;
	}
	return NULL;
}

