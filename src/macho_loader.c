//
// Created by Bryan Keller on 2/20/25.
//

#include "bootmii_ppc.h"
#include "ff.h"
#include "macho_loader.h"
#include "malloc.h"
#include "string.h"
#include "types.h"

#define LC_SEGMENT 1
#define LC_SYMTAB 2
#define LC_UNIXTHREAD 5

#define kMacOSXSignature 0x4D4F5358

typedef u32 cpu_type_t;
typedef u32 cpu_subtype_t;
typedef u32 vm_prot_t;

typedef struct mach_header {
    u32 magic;		            /* mach magic number identifier */
    cpu_type_t cputype;	        /* cpu specifier */
    cpu_subtype_t cpusubtype;	/* machine specifier */
    u32 filetype;	            /* type of file */
    u32 ncmds;		            /* number of load commands */
    u32 sizeofcmds;	            /* the size of all the load commands */
    u32 flags;		            /* flags */
} mach_header_t;

typedef struct load_command {
    u32 cmd;		            /* type of load command */
    u32 cmdsize;		        /* total size of command in bytes */
} load_command_t;

typedef struct segment_command {
    u32 cmd;		            /* LC_SEGMENT */
    u32 cmdsize;	            /* includes sizeof section structs */
    char segname[16];	        /* segment name */
    u32 vmaddr;		            /* memory address of this segment */
    u32 vmsize;		            /* memory size of this segment */
    u32 fileoff;	            /* file offset of this segment */
    u32 filesize;	            /* amount to map from the file */
    vm_prot_t maxprot;	        /* maximum VM protection */
    vm_prot_t initprot;	        /* initial VM protection */
    u32 nsects;		            /* number of sections in segment */
    u32 flags;		            /* flags */
} segment_command_t;

typedef struct symtab_command {
    u32 cmd;		            /* LC_SYMTAB */
    u32 cmdsize;	            /* sizeof(struct symtab_command) */
    u32 symoff;		            /* symbol table offset */
    u32 nsyms;		            /* number of symbol table entries */
    u32 stroff;		            /* string table offset */
    u32 strsize;	            /* string table size in bytes */
} symtab_command_t;

typedef struct thread_command {
    u32 cmd;		            /* LC_THREAD or  LC_UNIXTHREAD */
    u32 cmdsize;	            /* total size of this command */
    u32 flavor;
    u32 count;
} thread_command_t;

typedef struct ppc_thread_state {
    u32 srr0;                   /* Instruction address register (PC) */
    u32 srr1;	                /* Machine state register (supervisor) */
    u32 r0;
    u32 r1;
    u32 r2;
    u32 r3;
    u32 r4;
    u32 r5;
    u32 r6;
    u32 r7;
    u32 r8;
    u32 r9;
    u32 r10;
    u32 r11;
    u32 r12;
    u32 r13;
    u32 r14;
    u32 r15;
    u32 r16;
    u32 r17;
    u32 r18;
    u32 r19;
    u32 r20;
    u32 r21;
    u32 r22;
    u32 r23;
    u32 r24;
    u32 r25;
    u32 r26;
    u32 r27;
    u32 r28;
    u32 r29;
    u32 r30;
    u32 r31;

    u32 cr;                     /* Condition register */
    u32 xer;	                /* User's integer exception register */
    u32 lr;	                    /* Link register */
    u32 ctr;	                /* Count register */
    u32 mq;	                    /* MQ register (601 only) */

    u32 vrsave;	                /* Vector Save Register */
} ppc_thread_state_t;

u32 kernelEntryPoint;

int decode_mach_kernel(u8 *fbuf);
int handle_load_cmd(load_command_t *load_cmd, u8 *fbuf);
int handle_lc_segment(load_command_t *load_cmd, u8 *fbuf);
int load_segment(u8 *fbuf, u32 foff, u32 fsize, u32 vmaddr, u32 vmsize);
int handle_lc_symtab(load_command_t *load_cmd, u8 *fbuf);
int handle_lc_unixthread(load_command_t *load_cmd);

int load_mach_kernel(const char *kernel_path) {
    FATFS fs;
    FRESULT res;
    FIL fp;

    printf("Loading Mach Kernel from SD:%s...\n", kernel_path);

    // Mount the SD card
    res = f_mount(0, &fs);
    if (res != FR_OK) {
        printf("Failed to mount SD card! Error code: %d\n", res);
        return -1;
    }

    res = f_open(&fp, kernel_path, FA_READ);
    if (res != FR_OK) {
        printf("Failed to open kernel at SD:%s\n", kernel_path);
        return -1;
    }

    size_t fsize = fp.fsize;
    u8 *fbuf = (u8*)kernelFileLoadAddress;

    u32 bytesRead;
    res = f_read(&fp, fbuf, fsize, &bytesRead);
    if (res != FR_OK && bytesRead == fsize) {
        printf("Failed to read kernel at SD:%s\n", kernel_path);
        return -1;
    }

    return decode_mach_kernel(fbuf);
}

int decode_mach_kernel(u8 *fbuf) {
    mach_header_t *header = (mach_header_t *)fbuf;
    printf("magic: %X\n", header->magic);
    printf("cpu_type: %d\n", header->cputype);
    printf("cpu_subtype: %d\n", header->cpusubtype);
    printf("file_type: %d\n", header->filetype);
    printf("num_cmds: %d\n", header->ncmds);
    printf("size_of_cmds: %d\n", header->sizeofcmds);
    printf("flags: %X\n", header->flags);

    printf("\n");

    u8 *cmds_offset = fbuf + sizeof(mach_header_t);
    u32 num_cmds = header->ncmds;

    for (u32 i = 0; i < num_cmds; i++) {
        load_command_t *cmd = (load_command_t *)cmds_offset;
        int ret = handle_load_cmd(cmd, fbuf);
        if (ret != 0) {
            return -1;
        }
        cmds_offset += cmd->cmdsize;
        printf("\n");
    }

    return 0;
}

int handle_load_cmd(load_command_t *load_cmd, u8 *fbuf) {
    int ret = 0;
    switch (load_cmd->cmd) {
        case LC_SEGMENT:
            ret = handle_lc_segment(load_cmd, fbuf);
        break;
        case LC_SYMTAB:
            ret = handle_lc_symtab(load_cmd, fbuf);
        break;
        case LC_UNIXTHREAD:
            ret = handle_lc_unixthread(load_cmd);
        break;
        default:
            printf("Unknown load command %d\n", load_cmd->cmd);
        ret = -1;
        break;
    }

    return ret;
}

int handle_lc_segment(load_command_t *load_cmd, u8 *fbuf) {
    printf("LC_SEGMENT %d\n", load_cmd->cmdsize);

    segment_command_t *segment = (segment_command_t *)load_cmd;
    printf("Handle %s\n", segment->segname);

    int ret = load_segment(fbuf, segment->fileoff, segment->filesize, segment->vmaddr, segment->vmsize);
    if (ret != 0) {
        printf("Failed to load segment %s into memory\n", segment->segname);
        return -1;
    }

    return 0;
}

int load_segment(u8 *fbuf, u32 foff, u32 fsize, u32 vmaddr, u32 vmsize) {
    u8 *src = fbuf + foff;
    u8 *dst = (u8*)vmaddr;
    printf("0x%08x -> 0x%08x, %ul\n", src, dst, fsize);

    if (fsize == 0) {
        return -1;
    }

    if (fsize < vmsize) {
        memset(dst, 0, vmsize);
    }

    memcpy(dst, src, fsize);

    return 0;
}

int handle_lc_symtab(load_command_t *load_cmd, u8 *fbuf) {
    printf("LC_SYMTAB %d\n", load_cmd->cmdsize);

    symtab_command_t *symtab = (symtab_command_t *)load_cmd;
    u32 symSize = symtab->stroff - symtab->symoff;
    u32 totalSize = symSize + symtab->strsize;
    u32 symtabSize = totalSize + sizeof(symtab_command_t);
    u8 *symtabAddr = malloc(symtabSize);
    printf("Address: 0x%08x, size: %u\n", symtabAddr, symtabSize);

    symtab_command_t *symtabSave = (symtab_command_t *)symtabAddr;
    u32 symOff = (u32)symtabAddr + sizeof(struct symtab_command);
    symtabSave->symoff = symOff;
    symtabSave->nsyms = symtab->nsyms;
    symtabSave->stroff = symOff + symSize;
    symtabSave->strsize = symtab->strsize;

    memcpy(symtabAddr, fbuf + symtab->symoff, totalSize);

    return 0;
}

int handle_lc_unixthread(load_command_t *load_cmd) {
    printf("LC_UNIXTHREAD %d\n", load_cmd->cmdsize);

    ppc_thread_state_t *thread_state = (ppc_thread_state_t *)((u8*)load_cmd + sizeof(thread_command_t));
    printf("SRR0: 0x%08x\n", thread_state->srr0);

    kernelEntryPoint = thread_state->srr0;

    return 0;
}

int start_mach_kernel() {
    long msr, cnt;

    printf("\nCall Kernel @ 0x%08x; boot args: 0x00400000, signature: %08x\n", kernelEntryPoint, kMacOSXSignature);

    msr = 0x00001000;
    __asm__ volatile("mtmsr %0" : : "r" (msr));
    __asm__ volatile("isync");

    // Make sure everything get sync'd up.
    __asm__ volatile("isync");
    __asm__ volatile("sync");
    __asm__ volatile("eieio");

    (*(void (*)())kernelEntryPoint)(0x00400000, kMacOSXSignature);

    return -1;
}