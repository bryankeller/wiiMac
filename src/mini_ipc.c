/*
	mini_ipc.c -- public PowerPC-side interface to mini.  Part of the
	BootMii project.

Copyright (C) 2009			Andre Heider "dhewg" <dhewg@wiibrew.org>
Copyright (C) 2009			Haxx Enterprises <bushing@gmail.com>
Copyright (C) 2009			John Kelley <wiidev@kelley.ca>
Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "bootmii_ppc.h"
#include "ipc.h"
#include "mini_ipc.h"
#include "string.h"

int ipc_powerpc_boot(const void *addr, u32 len)
{
	ipc_request *req;

	sync_after_write(addr, len);
	req =  ipc_exchange(IPC_PPC_BOOT, 3, 0, virt_to_phys(addr), len);
	return req->args[0];
}

u32 boot2_run(u32 hi, u32 lo)
{
	ipc_request * req;
	req =  ipc_exchange(IPC_BOOT2_RUN, 2, hi, lo);
	return req->args[0];
}

tmd *boot2_tmd(void)
{
	tmd *ret = phys_to_virt(ipc_exchange(IPC_BOOT2_TMD, 0)->args[0]);
	sync_before_read(ret, sizeof(tmd));
	return ret;
}

void getotp(otp_t *otp)
{
	sync_before_read(otp, sizeof(*otp));
	ipc_exchange(IPC_KEYS_GETOTP, 1, virt_to_phys(otp));
}

void getMiniGitVer(char *buf, u16 len)
{
	if (len < 32)
	{
		memset((void *)buf, 0, 32);
		return;
	}
	sync_before_read(buf, len);
	ipc_exchange(IPC_SYS_GETGITS, 1, virt_to_phys(buf));
}

void getseeprom(seeprom_t *seeprom)
{
	sync_before_read(seeprom, sizeof(*seeprom));
	ipc_exchange(IPC_KEYS_GETEEP, 1, virt_to_phys(seeprom));
}

void aes_reset(void)
{
	ipc_exchange(IPC_AES_RESET, 0);
}

void aes_set_key(u8 *key)
{
	u32 *keyptr = (u32 *)key;
	ipc_exchange(IPC_AES_SETKEY, 4, keyptr[0], keyptr[1], keyptr[2], keyptr[3]);
}

void aes_set_iv(u8 *iv)
{
	u32 *ivptr = (u32 *)iv;
	ipc_exchange(IPC_AES_SETIV, 4, ivptr[0], ivptr[1], ivptr[2], ivptr[3]);
}

void aes_decrypt(u8 *src, u8 *dst, u32 blocks, u8 keep_iv)
{
	sync_after_write(src, (blocks+1)*16);
	ipc_exchange(IPC_AES_DECRYPT, 4, virt_to_phys(src), virt_to_phys(dst), blocks, keep_iv);
	sync_before_read(dst, (blocks+1)*16);
}

void nand_reset(void)
{
	ipc_exchange(IPC_NAND_RESET, 0);
}

u32 nand_getid(void)
{
	static u8 idbuf[64] __attribute__((aligned(64)));

	ipc_exchange(IPC_NAND_GETID, 1, virt_to_phys(&idbuf));
	sync_before_read(idbuf, 0x40);

	return idbuf[0] << 24 | idbuf[1] << 16 | idbuf[2] << 8 | idbuf[3];
}

u8 nand_status(void)
{
	static u8 buf[64] __attribute__((aligned(64)));

	ipc_exchange(IPC_NAND_STATUS, 1, virt_to_phys(&buf));
	sync_before_read(buf, 0x40);

	return buf[0];
}

int nand_read(u32 pageno, void *data, void *ecc)
{
	if (data)
		sync_before_read(data, 0x800);
	if (ecc)
		sync_before_read(ecc, 0x40);
	return ipc_exchange(IPC_NAND_READ, 3, pageno,
		(!data ? (u32)-1 : virt_to_phys(data)),
		(!ecc ? (u32)-1 : virt_to_phys(ecc)))->args[0];
}

void nand_write(u32 pageno, void *data, void *ecc)
{
	if (data)
		sync_after_write(data, 0x800);
	if (ecc)
		sync_after_write(ecc, 0x40);
	ipc_exchange(IPC_NAND_WRITE, 3, pageno,
		(!data ? (u32)-1 : virt_to_phys(data)),
		(!ecc ? (u32)-1 : virt_to_phys(ecc)));
}

void nand_erase(u32 pageno)
{
	ipc_exchange(IPC_NAND_ERASE, 1, pageno);
}

int sd_mount(void)
{
	return ipc_exchange(IPC_SDMMC_ACK, 0)->args[0];
}

int sd_get_state(void)
{
	return ipc_exchange(IPC_SDMMC_STATE, 0)->args[0];
}

int sd_protected(void)
{
//	return (ipc_exchange(IPC_SD_GETSTATE, 0)->args[0] & SDHC_WRITE_PROTECT) == SDHC_WRITE_PROTECT;
	return 0;
}

int sd_select(void)
{
	return 1;
//	return ipc_exchange(IPC_SD_SELECT, 0)->args[0];
}

int sd_read(u32 start_block, u32 blk_cnt, void *buffer)
{
	int retval;
	sync_before_read(buffer, blk_cnt * 512);
	retval = ipc_exchange(IPC_SDMMC_READ, 3, start_block, blk_cnt, virt_to_phys(buffer))->args[0];
	return retval;
}

int sd_write(u32 start_block, u32 blk_cnt, const void *buffer)
{
	int retval;
	sync_after_write(buffer, blk_cnt * 512);
	retval = ipc_exchange(IPC_SDMMC_WRITE, 3, start_block, blk_cnt, virt_to_phys(buffer))->args[0];

	return retval;
}

u32 sd_getsize(void)
{
	return ipc_exchange(IPC_SDMMC_SIZE, 0)->args[0];
}

