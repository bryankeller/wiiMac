/*
 * libhfs - library for reading and writing Macintosh HFS volumes
 * The fucntions are used to handle the various forms of btrees
 * found on HFS+ volumes.
 *
 * The fucntions are used to handle the various forms of btrees
 * found on HFS+ volumes.
 *
 * Copyright (C) 2000 Klaus Halfmann <khalfmann@libra.de>
 * Original 1996-1998 Robert Leslie <rob@mars.org>
 * Additional work by  Brad Boyer (flar@pants.nu)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * $Id: btree.c,v 1.14 2000/10/25 05:43:04 hasi Exp $
 */

#include "libhfsp.h"
#include "volume.h"
#include "btree.h"
#include "record.h"
#include "swab.h"
#include "../string.h"
#include "../malloc.h"

/* Read the node from the given buffer and swap the bytes.
 *
 * return pointer after reading the structure
 */
static void* btree_readnode(btree_node_desc* node, void *p)
{
    node->next	    = bswabU32_inc(p);
    node->prev	    = bswabU32_inc(p);
    node->kind	    = bswabU8_inc(p);
    node->height    = bswabU8_inc(p);
    node->num_rec   = bswabU16_inc(p);
    node->reserved  = bswabU16_inc(p);
    return p;
}

/* read a btree header from the given buffer and swap the bytes.
 *
 * return pointer after reading the structure
 */
static void* btree_readhead(btree_head* head, void *p)
{
	UInt32 *q;
        head->depth	    = bswabU16_inc(p);
        head->root	    = bswabU32_inc(p);
        head->leaf_count    = bswabU32_inc(p);
        head->leaf_head	    = bswabU32_inc(p);
        head->leaf_tail	    = bswabU32_inc(p);
        head->node_size	    = bswabU16_inc(p);
        head->max_key_len   = bswabU16_inc(p);
        head->node_count    = bswabU32_inc(p);
        head->free_nodes    = bswabU32_inc(p);
        head->reserved1	    = bswabU16_inc(p);
        head->clump_size    = bswabU32_inc(p);
        head->btree_type    = bswabU8_inc(p);
        head->reserved2	    = bswabU8_inc(p);
        head->attributes    = bswabU32_inc(p);
	    // skip reserved bytes
	q=((UInt32*) p);
	// ((UInt32*) p) += 16;
	q+=16;
	return q;
}

/* Priority of the depth of the node compared to LRU value.
 * Should be the average number of keys per node but these vary. */
#define DEPTH_FACTOR	1000

/* Cache size is height of tree + this value
 * Really big numbers wont help in case of ls -R
 */
#define EXTRA_CACHESIZE	3

/* Not in use by now ... */
#define CACHE_DIRTY 0x0001

/* Intialize cache with default cache Size,
 * must call node_cache_close to deallocate memory */
static int node_cache_init(node_cache* cache, btree* tree, int size)
{
    int nodebufsize;
    char * buf;

    cache->size		= size;
    cache->currindex	= 0;
    nodebufsize = tree->head.node_size + sizeof(node_buf);
    buf = malloc(size *(sizeof(node_entry) + nodebufsize));
    if (!buf)
	return -1;
    cache -> nodebufsize = nodebufsize;
    cache -> entries = (node_entry*) buf;
    cache -> buffers = (char*) &cache->entries[size];
    bzero(cache->entries, size*sizeof(node_entry));
    return 0;
}

/* Like cache->buffers[i], since size of node_buf is variable */
static inline node_buf* node_buf_get(node_cache* cache, int i)
{
    return (node_buf*) (cache->buffers + (i * cache->nodebufsize));
}

/* flush the node at index */
static void node_cache_flush_node(node_cache* cache, int index)
{
    // NYI
    cache -> entries[index].index = 0;	// invalidate entry
}

static void node_cache_close(node_cache* cache)
{
    if (!cache->entries) // not (fully) intialized ?
	return;
    free(cache->entries);
}

/* Load the cach node indentified by index with
 * the node identified by node_index */

static node_buf* node_cache_load_buf
    (btree* bt, node_cache* cache, int index, UInt16 node_index)
{
    node_buf	*result	    = node_buf_get(cache ,index);
    UInt32	blkpernode  = bt->blkpernode;
    UInt32	block	    = node_index * blkpernode;
    void*	p	    = volume_readfromfork(bt->vol, result->node, bt->fork,
			     block, blkpernode, HFSP_EXTENT_DATA, bt->cnid);
    node_entry	*e	    = &cache->entries[index];

    if (!p)
	return NULL;	// evil ...

    result->index   = node_index;
    btree_readnode(&result->desc, p);

    e -> priority   = result->desc.height * DEPTH_FACTOR;
    e -> index	    = node_index;
    return result;
}

/* Read node at given index, using cache.
 */
node_buf* btree_node_by_index(btree* bt, UInt16 index)
{
    node_cache*	cache = &bt->cache;
    int		oldindex, lruindex;
    int		currindex = cache->currindex;
    UInt32	prio;
    node_entry	*e;

    // Shortcut acces to current node, will not change priorities
    if (cache->entries[currindex].index == index)
	return node_buf_get(cache ,currindex);
    oldindex = currindex;
    if (currindex == 0)
	currindex = cache->size;
    currindex--;
    lruindex = oldindex;	    // entry to be flushed when needed
    prio     = 0;		    // current priority
    while (currindex != oldindex)   // round robin
    {
	e = &cache->entries[currindex];
	if (e->index == index)	    // got it
	{
	    if (e->priority != 0)   // already top, uuh
		e->priority--;
	    cache->currindex = currindex;
	    return node_buf_get(cache ,currindex);
	}
	else
	{
	    if (!e->index)
	    {
		lruindex = currindex;
		break;	// empty entry, load it
	    }
	    if (e->priority != UINT_MAX) // already least, uuh
		e->priority++;
	}
	if (prio < e->priority)
	{
	    lruindex = currindex;
	    prio = e->priority;
	}
	if (currindex == 0)
	    currindex = cache->size;
	currindex--;
    }
    e = &cache->entries[lruindex];
    cache->currindex = lruindex;
    if (e->flags & CACHE_DIRTY)
           node_cache_flush_node(    cache, lruindex);
    return node_cache_load_buf  (bt, cache, lruindex, index);
}

/** intialize the btree with the first entry in the fork */
static int btree_init(btree* bt, volume* vol, hfsp_fork_raw* fork)
{
    void	    *p;
    char	    buf[vol->blksize];
    UInt16	    node_size;
    btree_node_desc node;

    bt->vol	= vol;
    bt->fork	= fork;
    p	= volume_readfromfork(vol, buf, fork, 0, 1,
		 HFSP_EXTENT_DATA, bt->cnid);
    if (!p)
	return -1;
    p = btree_readnode(&node, p);
    if (node.kind != HFSP_NODE_HEAD)
	return -1;   // should not happen ?
    btree_readhead(&bt->head, p);

    node_size = bt->head.node_size;
    bt->blkpernode = node_size / vol->blksize;

    if (bt->blkpernode == 0 || vol->blksize *
	    bt->blkpernode != node_size)
	return -1;  // should never happen ...

    node_cache_init(&bt->cache, bt, bt->head.depth + EXTRA_CACHESIZE);

    // Allocate buffer
    // bt->buf = malloc(node_size);
    // if (!bt->buf)
    //	return ENOMEM;

    return 0;
}

/** Intialize catalog btree, so that btree_close can safely be called. */
void btree_reset(btree* bt)
{
    bt->cache.entries = NULL;
}

/** Intialize catalog btree */
int btree_init_cat(btree* bt, volume* vol, hfsp_fork_raw* fork)
{
    int result = btree_init(bt,vol,fork);	// super (...)
    bt->cnid  = HFSP_CAT_CNID;
    bt->kcomp = record_key_compare;
    bt->kread = record_readkey;
    return result;
}

/** Intialize catalog btree */
int btree_init_extent(btree* bt, volume* vol, hfsp_fork_raw* fork)
{
    int result = btree_init(bt,vol,fork);	// super (...)
    bt->cnid  = HFSP_EXT_CNID;
    bt->kcomp = record_extent_key_compare;
    bt->kread = record_extent_readkey;
    return result;
}

/** close the btree and free any resources */
void btree_close(btree* bt)
{
    node_cache_close(&bt->cache);
    // free(bt->buf);
}

/* returns pointer to key given by index in current node.
 *
 * Assumes that current node is not NODE_HEAD ...
 */
void* btree_key_by_index(btree* bt, node_buf* buf, UInt16 index)
{
    UInt16  node_size	    = bt->head.node_size;
	// The offsets are found at the end of the node ...
    UInt16  off_pos	    = node_size - (index +1) * sizeof(btree_record_offset);
	// position of offset at end of node
    btree_record_offset* offset =
	(btree_record_offset*) (buf->node + off_pos);

    // now we have the offset and can read the key ...
#ifdef CONFIG_LITTLE_ENDIAN
    return buf->node + bswabU16(*offset);
#else
    return buf->node + *offset;
#endif
}
