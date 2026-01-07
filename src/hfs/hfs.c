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
 *  hfs.c - File System Module for HFS and HFS+.
 *
 *  Copyright (c) 1999-2002 Apple Computer, Inc.
 *
 *  DRI: Josh de Cesare
 */

#include "sl.h"
#include "hfs_format.h"
#include "hfs.h"

#define kBlockSize (0x200)

#define kMDBBaseOffset (2 * kBlockSize)

#define kBTreeCatalog (0)
#define kBTreeExtents (1)

#define kHFSBinaryCompare 0xBC
#define kHFSXSigWord 0x482B

static CICell                  gCurrentIH;
static long long               gAllocationOffset;
static long                    gIsHFSPlus;
static long                    gBlockSize;
static long                    gCaseSensitive;
static long                    gCacheBlockSize;
static char                    gBTreeHeaderBuffer[512];
static BTHeaderRec             *gBTHeaders[2];
static char                    gHFSMdbVib[kBlockSize];
static HFSMasterDirectoryBlock *gHFSMDB =(HFSMasterDirectoryBlock*)gHFSMdbVib;
static char                    gHFSPlusHeader[kBlockSize];
static HFSPlusVolumeHeader     *gHFSPlus =(HFSPlusVolumeHeader*)gHFSPlusHeader;
static char                    gLinkTemp[64];
static long long               gVolID;
static char                    *gTempStr;

static long ReadFile(void *file, uint64_t *length, void *base, uint64_t offset);
static long GetCatalogEntryInfo(void *entry, long *flags, long *time,
                                FinderInfo *finderInfo, long *infoValid);
static long ResolvePathToCatalogEntry(char *filePath, long *flags,
                void *entry, long dirID, long *dirIndex);

static long GetCatalogEntry(long *dirIndex, char **name,
                            long *flags, long *time,
                            FinderInfo *finderInfo, long *infoValid);
static long ReadCatalogEntry(char *fileName, long dirID, void *entry,
                long *dirIndex);
static long ReadExtentsEntry(long fileID, long startBlock, void *entry);

static long ReadBTreeEntry(long btree, void *key, char *entry, long *dirIndex);
static void GetBTreeRecord(long index, char *nodeBuffer, long nodeSize,
                char **key, char **data);

static long ReadExtent(char *extent, uint64_t extentSize, long extentFile,
                uint64_t offset, uint64_t size, void *buffer, long cache);

static long GetExtentStart(void *extents, long index);
static long GetExtentSize(void *extents, long index);

static long CompareHFSCatalogKeys(void *key, void *testKey);
static long CompareHFSPlusCatalogKeys(void *key, void *testKey);
static long CompareHFSExtentsKeys(void *key, void *testKey);
static long CompareHFSPlusExtentsKeys(void *key, void *testKey);

extern long FastRelString(u_int8_t *str1, u_int8_t *str2);
extern long BinaryUnicodeCompare(u_int16_t *uniStr1, u_int32_t len1,
                                 u_int16_t *uniStr2, u_int32_t len2);


static void SwapFinderInfo(FndrFileInfo *dst, FndrFileInfo *src)
{
    dst->fdType = SWAP_BE32(src->fdType);
    dst->fdCreator = SWAP_BE32(src->fdCreator);
    dst->fdFlags = SWAP_BE16(src->fdFlags);
    // Don't bother with location
}

void HFSFree(CICell ih)
{
    if(gCurrentIH == ih)
        gCurrentIH = 0;
    free(ih);
}

bool HFSProbe (const void *buf)
{
	const HFSMasterDirectoryBlock *mdb;
	const HFSPlusVolumeHeader     *header;
	mdb=(const HFSMasterDirectoryBlock *)(((const char*)buf)+kMDBBaseOffset);
	header=(const HFSPlusVolumeHeader *)(((const char*)buf)+kMDBBaseOffset);
	
	if ( SWAP_BE16(mdb->drSigWord) == kHFSSigWord )
		return true;
	if (SWAP_BE16(header->signature) != kHFSPlusSigWord &&
        SWAP_BE16(header->signature) != kHFSXSigWord)
		return false;
	return true;
}

long HFSInitPartition(CICell ih)
{
    long extentSize, extentFile, nodeSize;
    void *extent;

    if (ih == gCurrentIH) {
        return 0;
    }

    gAllocationOffset = 0;
    gIsHFSPlus = 0;
    gCaseSensitive = 0;
    gBTHeaders[0] = 0;
    gBTHeaders[1] = 0;

    // Look for the HFS MDB
    Seek(ih, kMDBBaseOffset);
    Read(ih, (long)gHFSMdbVib, kBlockSize);

    if ( SWAP_BE16(gHFSMDB->drSigWord) == kHFSSigWord ) {
        gAllocationOffset = SWAP_BE16(gHFSMDB->drAlBlSt) * kBlockSize;

        // See if it is HFSPlus
        if (SWAP_BE16(gHFSMDB->drEmbedSigWord) != kHFSPlusSigWord) {
            // Normal HFS;
            gCacheBlockSize = gBlockSize = SWAP_BE32(gHFSMDB->drAlBlkSiz);
            CacheInit(ih, gCacheBlockSize);
            gCurrentIH = ih;

            // grab the 64 bit volume ID
            bcopy(&gHFSMDB->drFndrInfo[6], &gVolID, 8);

            // Get the Catalog BTree node size.
            extent     = (HFSExtentDescriptor *)&gHFSMDB->drCTExtRec;
            extentSize = SWAP_BE32(gHFSMDB->drCTFlSize);
            extentFile = kHFSCatalogFileID;
            ReadExtent(extent, extentSize, extentFile, 0, 256,
                       gBTreeHeaderBuffer + kBTreeCatalog * 256, 0);

            nodeSize = SWAP_BE16(((BTHeaderRec *)(gBTreeHeaderBuffer + kBTreeCatalog * 256 + 
                                 sizeof(BTNodeDescriptor)))->nodeSize);

            // If the BTree node size is larger than the block size, reset the cache.
            if (nodeSize > gBlockSize) {
                gCacheBlockSize = nodeSize;
                CacheInit(ih, gCacheBlockSize);
            }

            return 0;
        }

        // Calculate the offset to the embeded HFSPlus volume.
        gAllocationOffset += (long long)SWAP_BE16(gHFSMDB->drEmbedExtent.startBlock) *
                                        SWAP_BE32(gHFSMDB->drAlBlkSiz);
    }

    // Look for the HFSPlus Header
    Seek(ih, gAllocationOffset + kMDBBaseOffset);
    Read(ih, (long)gHFSPlusHeader, kBlockSize);

    // Not a HFS+ or HFSX volume.
    if (SWAP_BE16(gHFSPlus->signature) != kHFSPlusSigWord &&
        SWAP_BE16(gHFSPlus->signature) != kHFSXSigWord) {
	printf("HFS signature was not present.\n");
        gCurrentIH = 0;
	return -1;
    }

    gIsHFSPlus = 1;
    gCacheBlockSize = gBlockSize = SWAP_BE32(gHFSPlus->blockSize);
    CacheInit(ih, gCacheBlockSize);
    gCurrentIH = ih;

	ih->modTime = SWAP_BE32(gHFSPlus->modifyDate) - 2082844800;
	
    // grab the 64 bit volume ID
    bcopy(&gHFSPlus->finderInfo[24], &gVolID, 8);

    // Get the Catalog BTree node size.
    extent     = &gHFSPlus->catalogFile.extents;
    extentSize = SWAP_BE64(gHFSPlus->catalogFile.logicalSize);
    extentFile = kHFSCatalogFileID;

    ReadExtent(extent, extentSize, extentFile, 0, 256,
               gBTreeHeaderBuffer + kBTreeCatalog * 256, 0);

    nodeSize = SWAP_BE16(((BTHeaderRec *)(gBTreeHeaderBuffer + kBTreeCatalog * 256 +
                         sizeof(BTNodeDescriptor)))->nodeSize);

    // If the BTree node size is larger than the block size, reset the cache.
    if (nodeSize > gBlockSize) {
        gCacheBlockSize = nodeSize;
        CacheInit(ih, gCacheBlockSize);
    }

    return 0;
}

long HFSLoadFile(CICell ih, char * filePath)
{
    return HFSReadFile(ih, filePath, (void *)gFSLoadAddress, 0, 0);
}

long HFSReadFile(CICell ih, char * filePath, void *base, uint64_t offset,  uint64_t length)
{
    char entry[512];
    long dirID, result, flags;

    if (HFSInitPartition(ih) == -1) return -1;

    dirID = kHFSRootFolderID;
    // Skip a lead '\'.  Start in the system folder if there are two.
    if (filePath[0] == '/') {
        if (filePath[1] == '/') {
            if (gIsHFSPlus) dirID = SWAP_BE32(((long *)gHFSPlus->finderInfo)[5]);
            else dirID = SWAP_BE32(gHFSMDB->drFndrInfo[5]);
            if (dirID == 0) {
		return -1;
	    }
            filePath++;
        }
        filePath++;
    }

    result = ResolvePathToCatalogEntry(filePath, &flags, entry, dirID, 0);
    if ((result == -1) || ((flags & kFileTypeMask) != kFileTypeFlat)) {
	return -1;
    }

#if UNUSED
    // Not yet for Intel. System.config/Default.table will fail this check.
    // Check file owner and permissions.
    if (flags & (kOwnerNotRoot | kPermGroupWrite | kPermOtherWrite)) return -1;
#endif

    result = ReadFile(entry, &length, base, offset);
    if (result == -1) {
	return -1;
    }

  printf("Loaded HFS%s file: [%s] %d bytes from %x.\n",
            (gIsHFSPlus ? "+" : ""), filePath, (uint32_t)length, ih);
	
    return length;
}

long HFSGetDirEntry(CICell ih, char * dirPath, long * dirIndex, char ** name,
                    long * flags, long * time,
                    FinderInfo * finderInfo, long * infoValid)
{
    char entry[512];
    long dirID, dirFlags;

    if (HFSInitPartition(ih) == -1) return -1;

    if (*dirIndex == -1) return -1;

    dirID = kHFSRootFolderID;
    // Skip a lead '\'.  Start in the system folder if there are two.
    if (dirPath[0] == '/') {
        if (dirPath[1] == '/') {
            if (gIsHFSPlus) dirID = SWAP_BE32(((long *)gHFSPlus->finderInfo)[5]);
            else dirID = SWAP_BE32(gHFSMDB->drFndrInfo[5]);
            if (dirID == 0) return -1;
            dirPath++;
        }
        dirPath++;
    }

    if (*dirIndex == 0) {
        ResolvePathToCatalogEntry(dirPath, &dirFlags, entry, dirID, dirIndex);
        if (*dirIndex == 0) *dirIndex = -1;
        if ((dirFlags & kFileTypeMask) != kFileTypeUnknown) return -1;
    }

    GetCatalogEntry(dirIndex, name, flags, time, finderInfo, infoValid);
    if (*dirIndex == 0) *dirIndex = -1;
    if ((*flags & kFileTypeMask) == kFileTypeUnknown) return -1;

    return 0;
}

void
HFSGetDescription(CICell ih, char *str, long strMaxLen)
{

    UInt16 nodeSize;
    UInt32 firstLeafNode;
    long dirIndex;
    char *name;
    long flags, time;

    if (HFSInitPartition(ih) == -1)  { return; }

    /* Fill some crucial data structures by side effect. */
    dirIndex = 0;
    HFSGetDirEntry(ih, "/", &dirIndex, &name, &flags, &time, 0, 0);

    /* Now we can loook up the volume name node. */
    nodeSize = SWAP_BE16(gBTHeaders[kBTreeCatalog]->nodeSize);
    firstLeafNode = SWAP_BE32(gBTHeaders[kBTreeCatalog]->firstLeafNode);

    dirIndex = firstLeafNode * nodeSize;

    GetCatalogEntry(&dirIndex, &name, &flags, &time, 0, 0);

    strncpy(str, name, strMaxLen);
    str[strMaxLen] = '\0';
}


long
HFSGetFileBlock(CICell ih, char *filePath, unsigned long long *firstBlock)
{
    char entry[512];
    long dirID, result, flags;
    void               *extents;
    HFSCatalogFile     *hfsFile     = (void *)entry;
    HFSPlusCatalogFile *hfsPlusFile = (void *)entry;

    if (HFSInitPartition(ih) == -1) return -1;

    dirID = kHFSRootFolderID;
    // Skip a lead '\'.  Start in the system folder if there are two.
    if (filePath[0] == '/') {
        if (filePath[1] == '/') {
            if (gIsHFSPlus) dirID = SWAP_BE32(((long *)gHFSPlus->finderInfo)[5]);
            else dirID = SWAP_BE32(gHFSMDB->drFndrInfo[5]);
            if (dirID == 0) {
		return -1;
	    }
            filePath++;
        }
        filePath++;
    }

    result = ResolvePathToCatalogEntry(filePath, &flags, entry, dirID, 0);
    if ((result == -1) || ((flags & kFileTypeMask) != kFileTypeFlat)) {
        printf("HFS: Resolve path %s failed\n", filePath);
	return -1;
    }

    if (gIsHFSPlus) {
        extents    = &hfsPlusFile->dataFork.extents;
    } else {
        extents    = &hfsFile->dataExtents;
    }

#if DEBUG
    printf("extent start 0x%x\n", (unsigned long)GetExtentStart(extents, 0));
    printf("block size 0x%x\n", (unsigned long)gBlockSize);
    printf("Allocation offset 0x%x\n", (unsigned long)gAllocationOffset);
#endif
    *firstBlock = ((unsigned long long)GetExtentStart(extents, 0) * (unsigned long long) gBlockSize + gAllocationOffset) / 512ULL;
    return 0;
}

long HFSGetUUID(CICell ih, char *uuidStr)
{
    if (HFSInitPartition(ih) == -1) return -1;
    if (gVolID == 0LL)  return -1;

    return CreateUUIDString((uint8_t*)(&gVolID), sizeof(gVolID), uuidStr);
}

// Private Functions

static long ReadFile(void * file, uint64_t * length, void * base, uint64_t offset)
{
    void               *extents;
    long               fileID;
    uint64_t               fileLength;
    HFSCatalogFile     *hfsFile     = file;
    HFSPlusCatalogFile *hfsPlusFile = file;

    if (gIsHFSPlus) {
        fileID  = SWAP_BE32(hfsPlusFile->fileID);
        fileLength = (uint64_t)SWAP_BE64(hfsPlusFile->dataFork.logicalSize);
        extents = &hfsPlusFile->dataFork.extents;
    } else {
        fileID  = SWAP_BE32(hfsFile->fileID);
        fileLength = SWAP_BE32(hfsFile->dataLogicalSize);
        extents = &hfsFile->dataExtents;
    }

    if (offset > fileLength) {
        printf("Offset is too large.\n");
        return -1;
    }

    if ((*length == 0) || ((offset + *length) > fileLength)) {
        *length = fileLength - offset;
    }

/*    if (*length > kLoadSize) {
        printf("File is too large.\n");
        return -1;
    }*/

    *length = ReadExtent((char *)extents, fileLength, fileID,
                         offset, *length, (char *)base, 0);

    return 0;
}

static long GetCatalogEntryInfo(void * entry, long * flags, long * time,
                                FinderInfo * finderInfo, long * infoValid)
{
    long tmpTime = 0;
    long valid = 0;

    // Get information about the file.
    
    switch ( SWAP_BE16(*(short *)entry) )
    {
        case kHFSFolderRecord           :
            *flags = kFileTypeDirectory;
            tmpTime = SWAP_BE32(((HFSCatalogFolder *)entry)->modifyDate);
            break;

        case kHFSPlusFolderRecord       :
            *flags = kFileTypeDirectory |
                     (SWAP_BE16(((HFSPlusCatalogFolder *)entry)->bsdInfo.fileMode) & kPermMask);
            if (SWAP_BE32(((HFSPlusCatalogFolder *)entry)->bsdInfo.ownerID) != 0)
                *flags |= kOwnerNotRoot;
            tmpTime = SWAP_BE32(((HFSPlusCatalogFolder *)entry)->contentModDate);
            break;

        case kHFSFileRecord             :
            *flags = kFileTypeFlat;
            tmpTime = SWAP_BE32(((HFSCatalogFile *)entry)->modifyDate);
            if (finderInfo) {
                SwapFinderInfo((FndrFileInfo *)finderInfo, &((HFSCatalogFile *)entry)->userInfo);
                valid = 1;
            }
            break;

        case kHFSPlusFileRecord         :
            *flags = kFileTypeFlat |
                     (SWAP_BE16(((HFSPlusCatalogFile *)entry)->bsdInfo.fileMode) & kPermMask);
            if (SWAP_BE32(((HFSPlusCatalogFile *)entry)->bsdInfo.ownerID) != 0)
                *flags |= kOwnerNotRoot;
            tmpTime = SWAP_BE32(((HFSPlusCatalogFile *)entry)->contentModDate);
            if (finderInfo) {
                SwapFinderInfo((FndrFileInfo *)finderInfo, &((HFSPlusCatalogFile *)entry)->userInfo);
                valid = 1;
            }
            break;

        case kHFSFileThreadRecord       :
        case kHFSPlusFileThreadRecord   :
        case kHFSFolderThreadRecord     :
        case kHFSPlusFolderThreadRecord :
            *flags = kFileTypeUnknown;
            tmpTime = 0;
            break;
    }

    if (time != 0) {
        // Convert base time from 1904 to 1970.
        *time = tmpTime - 2082844800;
    }
    if (infoValid) *infoValid = valid;

    return 0;
}

static long ResolvePathToCatalogEntry(char * filePath, long * flags,
                                      void * entry, long dirID, long * dirIndex)
{
    char                 *restPath;
    long                 result, cnt, subFolderID = 0, tmpDirIndex;
    HFSPlusCatalogFile   *hfsPlusFile;

    // Copy the file name to gTempStr
    cnt = 0;
    while ((filePath[cnt] != '/') && (filePath[cnt] != '\0')) cnt++;
    strlcpy(gTempStr, filePath, cnt+1);

    // Move restPath to the right place.
    if (filePath[cnt] != '\0') cnt++;
    restPath = filePath + cnt;

    // gTempStr is a name in the current Dir.
    // restPath is the rest of the path if any.

    result = ReadCatalogEntry(gTempStr, dirID, entry, dirIndex);
    if (result == -1) {
	return -1;
    }

    GetCatalogEntryInfo(entry, flags, 0, 0, 0);

    if ((*flags & kFileTypeMask) == kFileTypeDirectory) {
        if (gIsHFSPlus)
            subFolderID = SWAP_BE32(((HFSPlusCatalogFolder *)entry)->folderID);
        else
            subFolderID = SWAP_BE32(((HFSCatalogFolder *)entry)->folderID);
    }

    if ((*flags & kFileTypeMask) == kFileTypeDirectory)
        result = ResolvePathToCatalogEntry(restPath, flags, entry,
                                           subFolderID, dirIndex);

    if (gIsHFSPlus && ((*flags & kFileTypeMask) == kFileTypeFlat)) {
        hfsPlusFile = (HFSPlusCatalogFile *)entry;
        if ((SWAP_BE32(hfsPlusFile->userInfo.fdType) == kHardLinkFileType) &&
            (SWAP_BE32(hfsPlusFile->userInfo.fdCreator) == kHFSPlusCreator)) {
                sprintf(gLinkTemp, "%s/%s%ld", HFSPLUSMETADATAFOLDER,
                        HFS_INODE_PREFIX, SWAP_BE32(hfsPlusFile->bsdInfo.special.iNodeNum));
                result = ResolvePathToCatalogEntry(gLinkTemp, flags, entry,
                                                   kHFSRootFolderID, &tmpDirIndex);
        }
    }

    return result;
}

static long GetCatalogEntry(long * dirIndex, char ** name,
                            long * flags, long * time,
                            FinderInfo * finderInfo, long * infoValid)
{
    long              extentSize, nodeSize, curNode, index;
    void              *extent;
    char              *nodeBuf, *testKey, *entry;
    BTNodeDescriptor  *node;

    if (gIsHFSPlus) {
        extent     = &gHFSPlus->catalogFile.extents;
        extentSize = SWAP_BE64(gHFSPlus->catalogFile.logicalSize);
    } else {
        extent     = (HFSExtentDescriptor *)&gHFSMDB->drCTExtRec;
        extentSize = SWAP_BE32(gHFSMDB->drCTFlSize);
    }

    nodeSize = SWAP_BE16(gBTHeaders[kBTreeCatalog]->nodeSize);
    nodeBuf  = (char *)MALLOC(nodeSize);
    node     = (BTNodeDescriptor *)nodeBuf;

    index   = *dirIndex % nodeSize;
    curNode = *dirIndex / nodeSize;

    // Read the BTree node and get the record for index.
    ReadExtent(extent, extentSize, kHFSCatalogFileID,
               curNode * nodeSize, nodeSize, nodeBuf, 1);
    GetBTreeRecord(index, nodeBuf, nodeSize, &testKey, &entry);

    GetCatalogEntryInfo(entry, flags, time, finderInfo, infoValid);

    // Get the file name.
    if (gIsHFSPlus) {
        utf_encodestr(((HFSPlusCatalogKey *)testKey)->nodeName.unicode,
                      SWAP_BE16(((HFSPlusCatalogKey *)testKey)->nodeName.length),
                      (u_int8_t *)gTempStr, 256);
    } else {
        strncpy(gTempStr,
                (const char *)&((HFSCatalogKey *)testKey)->nodeName[1],
                 ((HFSCatalogKey *)testKey)->nodeName[0]);
        gTempStr[((HFSCatalogKey *)testKey)->nodeName[0]] = '\0';
    }
    *name = gTempStr;

    // Update dirIndex.
    index++;
    if (index == SWAP_BE16(node->numRecords)) {
        index = 0;
        curNode = SWAP_BE32(node->fLink);
    }
    *dirIndex = curNode * nodeSize + index;

    free(nodeBuf);

    return 0;
}

static long ReadCatalogEntry(char * fileName, long dirID,
                             void * entry,    long * dirIndex)
{
    long              length;
    char              key[sizeof(HFSPlusCatalogKey)];
    HFSCatalogKey     *hfsKey     = (HFSCatalogKey *)key;
    HFSPlusCatalogKey *hfsPlusKey = (HFSPlusCatalogKey *)key;
  
    // Make the catalog key.
    if ( gIsHFSPlus )
    {
        hfsPlusKey->parentID = SWAP_BE32(dirID);
        length = strlen(fileName);
        if (length > 255) length = 255;
        utf_decodestr((u_int8_t *)fileName, hfsPlusKey->nodeName.unicode,
                      &(hfsPlusKey->nodeName.length), 512);
    } else {
        hfsKey->parentID = SWAP_BE32(dirID);
        length = strlen(fileName);
        if (length > 31) length = 31;
        hfsKey->nodeName[0] = length;
        strncpy((char *)(hfsKey->nodeName + 1), fileName, length);
    }

    return ReadBTreeEntry(kBTreeCatalog, &key, entry, dirIndex);
}

static long ReadExtentsEntry(long fileID, long startBlock, void * entry)
{
    char             key[sizeof(HFSPlusExtentKey)];
    HFSExtentKey     *hfsKey     = (HFSExtentKey *)key;
    HFSPlusExtentKey *hfsPlusKey = (HFSPlusExtentKey *)key;

    // Make the extents key.
    if (gIsHFSPlus) {
        hfsPlusKey->forkType   = 0;
        hfsPlusKey->fileID     = SWAP_BE32(fileID);
        hfsPlusKey->startBlock = SWAP_BE32(startBlock);
    } else {
        hfsKey->forkType   = 0;
        hfsKey->fileID     = SWAP_BE32(fileID);
        hfsKey->startBlock = SWAP_BE16(startBlock);
    }

    return ReadBTreeEntry(kBTreeExtents, &key, entry, 0);
}

static long ReadBTreeEntry(long btree, void * key, char * entry, long * dirIndex)
{
    long             extentSize;
    void             *extent;
    short            extentFile;
    char             *nodeBuf;
    BTNodeDescriptor *node;
    long             nodeSize, result = 0, entrySize = 0;
    long             curNode, index = 0, lowerBound, upperBound;
    char             *testKey, *recordData;

    // Figure out which tree is being looked at.
    if (btree == kBTreeCatalog) {
        if (gIsHFSPlus) {
            extent     = &gHFSPlus->catalogFile.extents;
            extentSize = SWAP_BE64(gHFSPlus->catalogFile.logicalSize);
        } else {
            extent     = (HFSExtentDescriptor *)&gHFSMDB->drCTExtRec;
            extentSize = SWAP_BE32(gHFSMDB->drCTFlSize);
        }
        extentFile = kHFSCatalogFileID;
    } else {
        if (gIsHFSPlus) {
            extent     = &gHFSPlus->extentsFile.extents;
            extentSize = SWAP_BE64(gHFSPlus->extentsFile.logicalSize);
        } else {
            extent     = (HFSExtentDescriptor *)&gHFSMDB->drXTExtRec;
            extentSize = SWAP_BE32(gHFSMDB->drXTFlSize);
        }
        extentFile = kHFSExtentsFileID;
    }

    // Read the BTree Header if needed.
    if (gBTHeaders[btree] == 0) {
        ReadExtent(extent, extentSize, extentFile, 0, 256,
                   gBTreeHeaderBuffer + btree * 256, 0);
        gBTHeaders[btree] = (BTHeaderRec *)(gBTreeHeaderBuffer + btree * 256 +
                                            sizeof(BTNodeDescriptor));
        if ((gIsHFSPlus && btree == kBTreeCatalog) &&
            (gBTHeaders[btree]->keyCompareType == kHFSBinaryCompare)) {
          gCaseSensitive = 1;
        }
    }

    curNode  = SWAP_BE32(gBTHeaders[btree]->rootNode);
    nodeSize = SWAP_BE16(gBTHeaders[btree]->nodeSize);
    nodeBuf  = (char *)MALLOC(nodeSize);
    node     = (BTNodeDescriptor *)nodeBuf;

    while (1) {
        // Read the current node.
        ReadExtent(extent, extentSize, extentFile,
                   curNode * nodeSize, nodeSize, nodeBuf, 1);
    
        // Find the matching key.
        lowerBound = 0;
        upperBound = SWAP_BE16(node->numRecords) - 1;
        while (lowerBound <= upperBound) {
            index = (lowerBound + upperBound) / 2;

            GetBTreeRecord(index, nodeBuf, nodeSize, &testKey, &recordData);

            if (gIsHFSPlus) {
                if (btree == kBTreeCatalog) {
                    result = CompareHFSPlusCatalogKeys(key, testKey);
                } else {
                    result = CompareHFSPlusExtentsKeys(key, testKey);
                }
            } else {
                if (btree == kBTreeCatalog) {
                    result = CompareHFSCatalogKeys(key, testKey);
                } else {
                    result = CompareHFSExtentsKeys(key, testKey);
                }
            }
      
            if (result < 0) upperBound = index - 1;        // search < trial
            else if (result > 0) lowerBound = index + 1;   // search > trial
            else break;                                    // search = trial
        }
    
        if (result < 0) {
            index = upperBound;
            GetBTreeRecord(index, nodeBuf, nodeSize, &testKey, &recordData);
        }
    
        // Found the closest key... Recurse on it if this is an index node.
        if (node->kind == kBTIndexNode) {
            curNode = SWAP_BE32( *((long *)recordData) );
        } else break;
    }
  
    // Return error if the file was not found.
    if (result != 0) { free(nodeBuf); return -1; }

    if (btree == kBTreeCatalog) {
        switch (SWAP_BE16(*(short *)recordData)) {
            case kHFSFolderRecord           : entrySize = 70;  break;
            case kHFSFileRecord             : entrySize = 102; break;
            case kHFSFolderThreadRecord     : entrySize = 46;  break;
            case kHFSFileThreadRecord       : entrySize = 46;  break;
            case kHFSPlusFolderRecord       : entrySize = 88;  break;
            case kHFSPlusFileRecord         : entrySize = 248; break;
            case kHFSPlusFolderThreadRecord : entrySize = 264; break;
            case kHFSPlusFileThreadRecord   : entrySize = 264; break;
        }
    } else {
        if (gIsHFSPlus) entrySize = sizeof(HFSPlusExtentRecord);
        else entrySize = sizeof(HFSExtentRecord);
    }
  
    bcopy(recordData, entry, entrySize);

    // Update dirIndex.
    if (dirIndex != 0) {
        index++;
        if (index == SWAP_BE16(node->numRecords)) {
            index = 0;
            curNode = SWAP_BE32(node->fLink);
        }
        *dirIndex = curNode * nodeSize + index;
    }
  
    free(nodeBuf);
  
    return 0;
}

static void GetBTreeRecord(long index, char * nodeBuffer, long nodeSize,
                           char ** key, char ** data)
{
    long keySize;
    long recordOffset;
  
    recordOffset = SWAP_BE16(*((short *)(nodeBuffer + (nodeSize - 2 * index - 2))));
    *key = nodeBuffer + recordOffset;
    if (gIsHFSPlus) {
        keySize = SWAP_BE16(*(short *)*key);
        *data = *key + 2 + keySize;
    } else {
        keySize = **key;
        *data = *key + 2 + keySize - (keySize & 1);
    }
}

static long ReadExtent(char * extent, uint64_t extentSize,
                       long extentFile, uint64_t offset, uint64_t size,
                       void * buffer, long cache)
{
    uint64_t  lastOffset;
	long long blockNumber, countedBlocks = 0;
    long long nextExtent = 0, sizeRead = 0, readSize;
    long long nextExtentBlock, currentExtentBlock = 0;
    long long readOffset;
    long long extentDensity, sizeofExtent, currentExtentSize;
    char      *currentExtent, *extentBuffer = 0, *bufferPos = buffer;

    if (offset >= extentSize) return 0;

    if (gIsHFSPlus) {
        extentDensity = kHFSPlusExtentDensity;
        sizeofExtent  = sizeof(HFSPlusExtentDescriptor);
    } else {
        extentDensity = kHFSExtentDensity;
        sizeofExtent  = sizeof(HFSExtentDescriptor);
    }

    lastOffset = offset + size;
    while (offset < lastOffset) {
        blockNumber = offset / gBlockSize;

        // Find the extent for the offset.
        for (; ; nextExtent++) {
            if (nextExtent < extentDensity) {
                if ((countedBlocks+GetExtentSize(extent, nextExtent)-1)<blockNumber) {
                    countedBlocks += GetExtentSize(extent, nextExtent);
                    continue;
                }

                currentExtent = extent + nextExtent * sizeofExtent;
                break;
            }

            if (extentBuffer == 0) {
                extentBuffer = MALLOC(sizeofExtent * extentDensity);
                if (extentBuffer == 0) return -1;
            }

            nextExtentBlock = nextExtent / extentDensity;
            if (currentExtentBlock != nextExtentBlock) {
                ReadExtentsEntry(extentFile, countedBlocks, extentBuffer);
                currentExtentBlock = nextExtentBlock;
            }

            currentExtentSize = GetExtentSize(extentBuffer, nextExtent % extentDensity);

            if ((countedBlocks + currentExtentSize - 1) >= blockNumber) {
                currentExtent = extentBuffer + sizeofExtent * (nextExtent % extentDensity);
                break;
            }
      
            countedBlocks += currentExtentSize;
        }

        readOffset = ((blockNumber - countedBlocks) * gBlockSize) +
                     (offset % gBlockSize);
    
        readSize = GetExtentSize(currentExtent, 0) * gBlockSize - readOffset;
        if (readSize > (size - sizeRead)) readSize = size - sizeRead;

        readOffset += (long long)GetExtentStart(currentExtent, 0) * gBlockSize;
    
        CacheRead(gCurrentIH, bufferPos, gAllocationOffset + readOffset,
                  readSize, cache);

        sizeRead += readSize;
        offset += readSize;
        bufferPos += readSize;
    }

    if (extentBuffer) free(extentBuffer);

    return sizeRead;
}

static long GetExtentStart(void * extents, long index)
{
    long                    start;
    HFSExtentDescriptor     *hfsExtents     = extents;
    HFSPlusExtentDescriptor *hfsPlusExtents = extents;

    if (gIsHFSPlus) start = SWAP_BE32(hfsPlusExtents[index].startBlock);
    else start = SWAP_BE16(hfsExtents[index].startBlock);

    return start;
}

static long GetExtentSize(void * extents, long index)
{
    long                    size;
    HFSExtentDescriptor     *hfsExtents     = extents;
    HFSPlusExtentDescriptor *hfsPlusExtents = extents;
  
    if (gIsHFSPlus) size = SWAP_BE32(hfsPlusExtents[index].blockCount);
    else size = SWAP_BE16(hfsExtents[index].blockCount);

    return size;
}

static long CompareHFSCatalogKeys(void * key, void * testKey)
{
    HFSCatalogKey *searchKey, *trialKey;
    long          result, searchParentID, trialParentID;

    searchKey = key;
    trialKey  = testKey;

    searchParentID = SWAP_BE32(searchKey->parentID);
    trialParentID  = SWAP_BE32(trialKey->parentID);

    // parent dirID is unsigned
    if (searchParentID > trialParentID)      result = 1;
    else if (searchParentID < trialParentID) result = -1;
    else {
        // parent dirID's are equal, compare names
        result = FastRelString(searchKey->nodeName, trialKey->nodeName);
    }

    return result;
}

static long CompareHFSPlusCatalogKeys(void * key, void * testKey)
{
    HFSPlusCatalogKey *searchKey, *trialKey;
    long              result, searchParentID, trialParentID;
  
    searchKey = key;
    trialKey  = testKey;
  
    searchParentID = SWAP_BE32(searchKey->parentID);
    trialParentID  = SWAP_BE32(trialKey->parentID);

    // parent dirID is unsigned
    if (searchParentID > trialParentID)      result = 1;
    else if (searchParentID < trialParentID) result = -1;
    else {
        // parent dirID's are equal, compare names
        if ((searchKey->nodeName.length == 0) || (trialKey->nodeName.length == 0))
            result = searchKey->nodeName.length - trialKey->nodeName.length;
        else
          if (gCaseSensitive) {
            result = BinaryUnicodeCompare(&searchKey->nodeName.unicode[0],
                                          SWAP_BE16(searchKey->nodeName.length),
                                          &trialKey->nodeName.unicode[0],
                                          SWAP_BE16(trialKey->nodeName.length));
          } else {
            result = FastUnicodeCompare(&searchKey->nodeName.unicode[0],
                                        SWAP_BE16(searchKey->nodeName.length),
                                        &trialKey->nodeName.unicode[0],
                                        SWAP_BE16(trialKey->nodeName.length));
          }
    }

    return result;
}

static long CompareHFSExtentsKeys(void * key, void * testKey)
{
    HFSExtentKey *searchKey, *trialKey;
    long         result;
  
    searchKey = key;
    trialKey  = testKey;
  
    // assume searchKey < trialKey
    result = -1;            

    if (searchKey->fileID == trialKey->fileID) {
        // FileNum's are equal; compare fork types
        if (searchKey->forkType == trialKey->forkType) {
            // Fork types are equal; compare allocation block number
            if (searchKey->startBlock == trialKey->startBlock) {
                // Everything is equal
                result = 0;
            } else {
                // Allocation block numbers differ; determine sign
                if (SWAP_BE16(searchKey->startBlock) > SWAP_BE16(trialKey->startBlock))
                    result = 1;
            }
        } else {
            // Fork types differ; determine sign
            if (searchKey->forkType > trialKey->forkType) result = 1;
        }
    } else {
        // FileNums differ; determine sign
        if (SWAP_BE32(searchKey->fileID) > SWAP_BE32(trialKey->fileID))
            result = 1;
    }

    return result;
}

static long CompareHFSPlusExtentsKeys(void * key, void * testKey)
{
    HFSPlusExtentKey *searchKey, *trialKey;
    long             result;
  
    searchKey = key;
    trialKey  = testKey;
  
    // assume searchKey < trialKey
    result = -1;            
  
    if (searchKey->fileID == trialKey->fileID) {
        // FileNum's are equal; compare fork types
        if (searchKey->forkType == trialKey->forkType) {
            // Fork types are equal; compare allocation block number
            if (searchKey->startBlock == trialKey->startBlock) {
                // Everything is equal
                result = 0;
            } else {
                // Allocation block numbers differ; determine sign
                if (SWAP_BE32(searchKey->startBlock) > SWAP_BE32(trialKey->startBlock))
                    result = 1;
            }
        } else {
            // Fork types differ; determine sign
            if (searchKey->forkType > trialKey->forkType) result = 1;
        }
    } else {
        // FileNums differ; determine sign
        if (SWAP_BE32(searchKey->fileID) > SWAP_BE32(trialKey->fileID))
            result = 1;
    }

    return result;
}

