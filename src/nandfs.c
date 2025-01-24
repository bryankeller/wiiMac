/*
	BootMii - a Free Software replacement for the Nintendo/BroadOn IOS.
	Requires mini.

	NAND filesystem support

Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "bootmii_ppc.h"
#include "ipc.h"
#include "mini_ipc.h"
#include "nandfs.h"
#include "string.h"

#define	PAGE_SIZE	2048
#define NANDFS_FREE 	0xFFFE

static otp_t otp;

struct _nandfs_file_node {
	char name[NANDFS_NAME_LEN];
	u8 attr;
	u8 wtf;
	union {
		u16 first_child;
		u16 first_cluster;
	};
	u16 sibling;
	u32 size;
	u32 uid;
	u16 gid;
	u32 dummy;
} __attribute__((packed));

struct _nandfs_sffs {
	u8 magic[4];
	u32 version;
	u32 dummy;

	u16 cluster_table[32768];
	struct _nandfs_file_node files[6143];
} __attribute__((packed));


union _sffs_t {
	u8 buffer[16*8*2048];
	struct _nandfs_sffs sffs;
};

static union _sffs_t sffs __attribute__((aligned(32)));

/*static u8 _sffs_buffer[16*8*2048] __attribute__((aligned(32)));
static struct _nandfs_sffs *sffs = (struct _nandfs_sffs *)&_sffs_buffer;*/
static u8 buffer[8*2048] __attribute__((aligned(32)));
static s32 initialized = 0;

void nand_read_cluster(u32 pageno, u8 *buffer)
{
	int i;
	for (i = 0; i < 8; i++)
		nand_read(pageno + i, buffer + (i * PAGE_SIZE), NULL);
}

void nand_read_decrypted_cluster(u32 pageno, u8 *buffer)
{
	u8 iv[16] = {0,};
	nand_read_cluster(pageno, buffer);

	aes_reset();
	aes_set_iv(iv);
	aes_set_key(otp.nand_key);
	aes_decrypt(buffer, buffer, 0x400, 0);
}

s32 nandfs_initialize(void)
{
	u32 i;
	u32 supercluster = 0;
	u32 supercluster_version = 0;

	getotp(&otp);

	nand_reset();

	for(i = 0x7F00; i < 0x7fff; i++) {
		nand_read(i*8, sffs.buffer, NULL);
		if(memcmp(sffs.sffs.magic, "SFFS", 4) != 0)
			continue;
		if(supercluster == 0 ||
		   sffs.sffs.version > supercluster_version) {
			supercluster = i*8;
			supercluster_version = sffs.sffs.version;
		}
	}

	if(supercluster == 0) {
		printf("no supercluster found. "
			     " your nand filesystem is seriously broken...\n");
		return -1;
	}

	for(i = 0; i < 16; i++) {
		printf("reading...\n");
		nand_read_cluster(supercluster + i*8,
				(sffs.buffer) + (i * PAGE_SIZE * 8));
	}

	initialized = 1;
	return 0;
}

u32 nandfs_get_usage(void) {
	u32 i;
	int used_clusters = 0;
	for (i=0; i < sizeof(sffs.sffs.cluster_table) / sizeof(u16); i++)
		if(sffs.sffs.cluster_table[i] != NANDFS_FREE) used_clusters++;
		
	printf("Used clusters: %d\n", used_clusters);
	return 1000 * used_clusters / (sizeof(sffs.sffs.cluster_table)/sizeof(u16));
}

s32 nandfs_open(struct nandfs_fp *fp, const char *path)
{
	char *ptr, *ptr2;
	u32 len;
	struct _nandfs_file_node *cur = sffs.sffs.files;

	if (initialized != 1)
		return -1;

	memset(fp, 0, sizeof(*fp));

	if(strcmp(cur->name, "/") != 0) {
		printf("your nandfs is corrupted. fixit!\n");
		return -1;
	}

	cur = &sffs.sffs.files[cur->first_child];

	ptr = (char *)path;
	do {
		ptr++;
		ptr2 = strchr(ptr, '/');
		if (ptr2 == NULL)
			len = strlen(ptr);
		else {
			ptr2++;
			len = ptr2 - ptr - 1;
		}
		if (len > 12)
		{
			printf("invalid length: %s %s %s [%d]\n",
					ptr, ptr2, path, len);
			return -1;
		}

		for (;;) {
			if(ptr2 != NULL && strncmp(cur->name, ptr, len) == 0
			     && strnlen(cur->name, 12) == len
			     && (cur->attr&3) == 2
			     && (s16)(cur->first_child&0xffff) != (s16)0xffff) {
				cur = &sffs.sffs.files[cur->first_child];
				ptr = ptr2-1;
				break;
			} else if(ptr2 == NULL &&
				   strncmp(cur->name, ptr, len) == 0 &&
				   strnlen(cur->name, 12) == len &&
				   (cur->attr&3) == 1) {
				break;
			} else if((cur->sibling&0xffff) != 0xffff) {
				cur = &sffs.sffs.files[cur->sibling];
			} else {
				return -1;
			}
		}
		
	} while(ptr2 != NULL);

	fp->first_cluster = cur->first_cluster;
	fp->cur_cluster = fp->first_cluster;
	fp->offset = 0;
	fp->size = cur->size;
	return 0;
}

s32 nandfs_read(void *ptr, u32 size, u32 nmemb, struct nandfs_fp *fp)
{
	u32 total = size*nmemb;
	u32 copy_offset, copy_len;

	if (initialized != 1)
		return -1;

	if (fp->offset + total > fp->size)
		total = fp->size - fp->offset;

	if (total == 0)
		return 0;

	while(total > 0) {
		nand_read_decrypted_cluster(fp->cur_cluster*8, buffer);
		copy_offset = fp->offset % (PAGE_SIZE * 8);
		copy_len = (PAGE_SIZE * 8) - copy_offset;
		if(copy_len > total)
			copy_len = total;
		memcpy(ptr, buffer + copy_offset, copy_len);
		total -= copy_len;
		fp->offset += copy_len;

		if ((copy_offset + copy_len) >= (PAGE_SIZE * 8))
			fp->cur_cluster = sffs.sffs.cluster_table[fp->cur_cluster];
	}

	return size*nmemb;
}

s32 nandfs_seek(struct nandfs_fp *fp, s32 offset, u32 whence)
{
	if (initialized != 1)
		return -1;

	switch (whence) {
	case NANDFS_SEEK_SET:
		if (offset < 0)
			return -1;
		if ((u32)offset > fp->size)
			return -1;

		fp->offset = offset;
		break;

	case NANDFS_SEEK_CUR:
		if ((fp->offset + offset) > fp->size ||
		    (s32)(fp->offset + offset) < 0)
			return -1;
		fp->offset += offset;
		break;

	case NANDFS_SEEK_END:
	default:
		if ((fp->size + offset) > fp->size ||
		    (s32)(fp->size + offset) < 0)
			return -1;
		fp->offset = fp->size + offset;
		break;
	}

	int skip = fp->offset;
	fp->cur_cluster = fp->first_cluster;
	while (skip > (2048*8)) {
		fp->cur_cluster = sffs.sffs.cluster_table[fp->cur_cluster];
		skip -= 2048*8;
	}

	return 0;
}

