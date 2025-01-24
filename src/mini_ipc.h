/*
	mini_ipc.h -- public PowerPC-side interface to mini.  Part of the
	BootMii project.

Copyright (C) 2009			Andre Heider "dhewg" <dhewg@wiibrew.org>
Copyright (C) 2009			Haxx Enterprises <bushing@gmail.com>
Copyright (C) 2009			John Kelley <wiidev@kelley.ca>
Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __MINI_IPC_H__
#define __MINI_IPC_H__

#define SDHC_ENOCARD    -0x1001
#define SDHC_ESTRANGE   -0x1002
#define SDHC_EOVERFLOW  -0x1003
#define SDHC_ETIMEDOUT  -0x1004
#define SDHC_EINVAL     -0x1005
#define SDHC_EIO        -0x1006

#define SDMMC_NO_CARD   1
#define SDMMC_NEW_CARD  2
#define SDMMC_INSERTED  3

#define NAND_ECC_OK 0
#define NAND_ECC_CORRECTED 1
#define NAND_ECC_UNCORRECTABLE -1

int sd_get_state(void);
int sd_protected(void);
int sd_mount(void);
int sd_select(void);
int sd_read(u32 start_block, u32 blk_cnt, void *buffer);
int sd_write(u32 start_block, u32 blk_cnt, const void *buffer);
u32 sd_getsize(void);

int ipc_powerpc_boot(const void *addr, u32 len);

#define TMD_BM_MARK(x) ((u16*)&(x->reserved[4]))
// 'BM'
#define TMD_BM_MAGIC 0x424d

typedef struct {
	u32 type;
	u8 sig[256];
	u8 fill[60];
} __attribute__((packed)) sig_rsa2048;

typedef struct {
	u32 cid;
	u16 index;
	u16 type;
	u64 size;
	u8 hash[20];
} __attribute__((packed)) tmd_content;

typedef struct {
	sig_rsa2048 signature;
	char issuer[0x40];
	u8 version;
	u8 ca_crl_version;
	u8 signer_crl_version;
	u8 fill2;
	u64 sys_version;
	u64 title_id;
	u32 title_type;
	u16 group_id;
	u16 zero;
	u16 region;
	u8 ratings[16];
	u8 reserved[42];
	u32 access_rights;
	u16 title_version;
	u16 num_contents;
	u16 boot_index;
	u16 fill3;
	tmd_content boot_content;
} __attribute__((packed)) tmd;

u32 boot2_run(u32 hi, u32 lo);
tmd *boot2_tmd(void);

typedef struct
{
	u8 boot1_hash[20];
	u8 common_key[16];
	u32 ng_id;
	union { // first two bytes of nand_hmac overlap last two bytes of ng_priv. no clue why
		struct {
			u8 ng_priv[30];
			u8 _wtf1[18];
		};
		struct {
			u8 _wtf2[28];
			u8 nand_hmac[20];
		};
	};
	u8 nand_key[16];
	u8 rng_key[16];
	u32 unk1;
	u32 unk2; // 0x00000007
} __attribute__((packed)) otp_t;

typedef struct
{
	u8 boot2version;
	u8 unknown1;
	u8 unknown2;
	u8 pad;
	u32 update_tag;
	u16 checksum;
} __attribute__((packed)) eep_ctr_t;

typedef struct
{
	union {
		struct {
			u32 ms_key_id;
			u32 ca_key_id;
			u32 ng_key_id;
			u8 ng_sig[60];
			eep_ctr_t counters[2];
			u8 fill[0x18];
			u8 korean_key[16];
		};
		u8 data[256];
	};
} __attribute__((packed)) seeprom_t;

void getotp(otp_t *otp);
void getseeprom(seeprom_t *seeprom);
void getMiniGitVer(char *buf, u16 len);
	
void aes_reset(void);
void aes_set_key(u8 *key);
void aes_set_iv(u8 *iv);
void aes_decrypt(u8 *src, u8 *dst, u32 blocks, u8 keep_iv);

void nand_reset(void);
u32 nand_getid(void);
u8 nand_status(void);
int nand_read(u32 pageno, void *data, void *ecc);
void nand_write(u32 pageno, void *data, void *ecc);
void nand_erase(u32 pageno);

#endif
