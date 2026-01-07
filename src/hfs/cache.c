/*
 * Copyright (c) 2000-2003 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 2.0 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 * 
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 *  cache.c - A simple cache for file systems meta-data.
 *
 *  Copyright (c) 2000 Apple Computer, Inc.
 *
 *  DRI: Josh de Cesare
 */

#include "sl.h"

struct CacheEntry {
  CICell    ih;
  long      time;
  long long offset;
};
typedef struct CacheEntry CacheEntry;

#define kCacheSize            (0x100000)
#define kCacheMinBlockSize    (0x200)
#define kCacheMaxBlockSize    (0x4000)
#define kCacheMaxEntries      (kCacheSize / kCacheMinBlockSize)

static CICell     gCacheIH;
static long       gCacheBlockSize;
static long       gCacheNumEntries;
static long       gCacheTime;

static CacheEntry gCacheEntries[kCacheMaxEntries];
static char       gCacheBuffer[kCacheSize];

#if CACHE_STATS
unsigned long     gCacheHits;
unsigned long     gCacheMisses;
unsigned long     gCacheEvicts;
#endif

void CacheReset()
{
    gCacheIH = NULL;
}

void CacheInit( CICell ih, long blockSize )
{
    if ((blockSize  < kCacheMinBlockSize) ||
        (blockSize >= kCacheMaxBlockSize))
        return;

    gCacheBlockSize = blockSize;
    gCacheNumEntries = kCacheSize / gCacheBlockSize;
    gCacheTime = 0;
    
#if CACHE_STATS
    gCacheHits = 0;
    gCacheMisses = 0;
    gCacheEvicts = 0;
#endif

    gCacheIH = ih;

    bzero(gCacheEntries, kCacheMaxEntries * sizeof(CacheEntry));
}

long CacheRead( CICell ih, char * buffer, long long offset,
	            long length, long cache )
{
    long       cnt, oldestEntry = 0, oldestTime, loadCache = 0;
    CacheEntry *entry;

    // See if the data can be cached.
    if (cache && (gCacheIH == ih) && (length == gCacheBlockSize)) {
        // Look for the data in the cache.
        for (cnt = 0; cnt < gCacheNumEntries; cnt++) {
            entry = &gCacheEntries[cnt];
            if ((entry->ih == ih) && (entry->offset == offset)) {
                entry->time = ++gCacheTime;
                break;
            }
        }

        // If the data was found copy it to the caller.
        if (cnt != gCacheNumEntries) {
            bcopy(gCacheBuffer + cnt * gCacheBlockSize, buffer, gCacheBlockSize);
#if CACHE_STATS
            gCacheHits++;
#endif
            return gCacheBlockSize;
        }

        // Could not find the data in the cache.
        loadCache = 1;
    }

    // Read the data from the disk.
    Seek(ih, offset);
    Read(ih, (long)buffer, length);
#if CACHE_STATS
    if (cache) gCacheMisses++;
#endif

    // Put the data from the disk in the cache if needed.
    if (loadCache) {
        // Find a free entry.
        oldestTime = gCacheTime;
        for (cnt = 0; cnt < gCacheNumEntries; cnt++) {
            entry = &gCacheEntries[cnt];

            // Found a free entry.
            if (entry->ih == 0) break;
        
            if (entry->time < oldestTime) {
                oldestTime = entry->time;
                oldestEntry = cnt;
            }
        }

        // If no free entry was found, use the oldest.
        if (cnt == gCacheNumEntries) {
            cnt = oldestEntry;
#if CACHE_STATS
            gCacheEvicts++;
#endif
        }

        // Copy the data from disk to the new entry.
        entry = &gCacheEntries[cnt];
        entry->ih = ih;
        entry->time = ++gCacheTime;
        entry->offset = offset;
        bcopy(buffer, gCacheBuffer + cnt * gCacheBlockSize, gCacheBlockSize);
    }

    return length;
}
