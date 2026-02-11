//
// Created by Bryan Keller on 2/20/25.
//

#include "bootmii_ppc.h"
#include "console.h"
#include "macho_decoder.h"
#include "string.h"
#include "types.h"
#include "kernel_allocator.h"

#define LC_SEGMENT 1
#define LC_SYMTAB 2
#define LC_UNIXTHREAD 5

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

static int handle_load_cmd(load_command_t *load_cmd);
static int handle_lc_segment(load_command_t *load_cmd);
static int load_segment(u32 foff, u32 fsize, u32 vmaddr, u32 vmsize);
static int handle_lc_symtab(load_command_t *load_cmd);
static int handle_lc_unixthread(load_command_t *load_cmd);

int decode_mach_kernel() {
  mach_header_t *header = (mach_header_t *)KERNEL_FILE_LOAD_ADDRESS;
  
  printf("\n");
  printf("Found kernel header:\n");
  printf("magic: %X\n", header->magic);
  printf("cpu_type: %d\n", header->cputype);
  printf("cpu_subtype: %d\n", header->cpusubtype);
  printf("file_type: %d\n", header->filetype);
  printf("num_cmds: %d\n", header->ncmds);
  printf("size_of_cmds: %d\n", header->sizeofcmds);
  printf("flags: %X\n", header->flags);
  
  printf("\n");
  printf("Decoding Mach Kernel...\n");
  
  void *cmds_offset = ((void *)KERNEL_FILE_LOAD_ADDRESS) + sizeof(mach_header_t);
  u32 num_cmds = header->ncmds;
  
  for (u32 i = 0; i < num_cmds; i++) {
    load_command_t *cmd = (load_command_t *)cmds_offset;
    int ret = handle_load_cmd(cmd);
    if (ret != 0) {
      return -1;
    }
    cmds_offset += cmd->cmdsize;
    printf("\n");
  }
  
  u32 header_size = (u32)cmds_offset - (u32)header;
  u32 kernel_header_start = alloc_kernel_memory(header_size);
  memcpy((void*)kernel_header_start, header, header_size);
  
  strlcpy(macho_memory_map_entries[macho_memory_map_entries_count].name, "Kernel-__HEADER", sizeof(macho_memory_map_entries[macho_memory_map_entries_count].name));
  macho_memory_map_entries[macho_memory_map_entries_count].start = kernel_header_start;
  macho_memory_map_entries[macho_memory_map_entries_count].size = header_size;
  macho_memory_map_entries_count += 1;
  
  return 0;
}

static int handle_load_cmd(load_command_t *load_cmd) {
  int ret = 0;
  switch (load_cmd->cmd) {
    case LC_SEGMENT:
      ret = handle_lc_segment(load_cmd);
      break;
    case LC_SYMTAB:
      ret = handle_lc_symtab(load_cmd);
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

static int handle_lc_segment(load_command_t *load_cmd) {
  printf("LC_SEGMENT %d\n", load_cmd->cmdsize);
  
  segment_command_t *segment = (segment_command_t *)load_cmd;
  printf("Handle %s\n", segment->segname);
  
  sprintf(macho_memory_map_entries[macho_memory_map_entries_count].name, "Kernel-%s", segment->segname);
  macho_memory_map_entries[macho_memory_map_entries_count].start = segment->vmaddr;
  macho_memory_map_entries[macho_memory_map_entries_count].size = segment->vmsize;
  macho_memory_map_entries_count += 1;
  
  int ret = load_segment(segment->fileoff, segment->filesize, segment->vmaddr, segment->vmsize);
  if (ret != 0) {
    printf("Failed to load segment %s into memory\n", segment->segname);
    return -1;
  }
  
  return 0;
}

static int load_segment(u32 foff, u32 fsize, u32 vmaddr, u32 vmsize) {
  void *src = ((void *)KERNEL_FILE_LOAD_ADDRESS) + foff;
  void *dst = (void *)vmaddr;
  
  printf("memcpy 0x%08x-0x%08x to 0x%08x-0x%08x\n", src, src + fsize, dst, dst + fsize);
  
  if (fsize == 0) {
    return -1;
  }
  
  // Zeroing memory is handled earlier
  memcpy(dst, src, fsize);
  
  // Reserve space in kernel allocator
  if (vmaddr + vmsize > top_of_kernel_data()) {
    alloc_kernel_memory(vmaddr + vmsize - top_of_kernel_data());
  }
  
  return 0;
}

static int handle_lc_symtab(load_command_t *load_cmd) {
  printf("LC_SYMTAB %d\n", load_cmd->cmdsize);
  
  symtab_command_t *symtab = (symtab_command_t *)load_cmd;
  
  u32 sym_size = symtab->stroff - symtab->symoff;
  u32 total_size = sym_size + symtab->strsize;
  u32 symtab_size = total_size + sizeof(symtab_command_t);
  u32 kernel_symtab_start = alloc_kernel_memory(symtab_size);
  printf("0x%08x-0x%08x\n", kernel_symtab_start, kernel_symtab_start + symtab_size);
  
  symtab_command_t *symtabSave = (symtab_command_t *)kernel_symtab_start;
  u32 sym_off = (u32)kernel_symtab_start + sizeof(struct symtab_command);
  symtabSave->symoff = sym_off;
  symtabSave->nsyms = symtab->nsyms;
  symtabSave->stroff = sym_off + sym_size;
  symtabSave->strsize = symtab->strsize;
  
  memcpy((void *)sym_off, ((void *)KERNEL_FILE_LOAD_ADDRESS) + symtab->symoff, total_size);
  
  strlcpy(macho_memory_map_entries[macho_memory_map_entries_count].name, "Kernel-__SYMTAB", sizeof(macho_memory_map_entries[macho_memory_map_entries_count].name));
  macho_memory_map_entries[macho_memory_map_entries_count].start = kernel_symtab_start;
  macho_memory_map_entries[macho_memory_map_entries_count].size = symtab_size;
  macho_memory_map_entries_count += 1;
  
  return 0;
}

static int handle_lc_unixthread(load_command_t *load_cmd) {
  printf("LC_UNIXTHREAD %d\n", load_cmd->cmdsize);
  
  ppc_thread_state_t *thread_state = (ppc_thread_state_t *)((u8*)load_cmd + sizeof(thread_command_t));
  printf("SRR0: 0x%08x\n", thread_state->srr0);
  
  kernel_entry_point = thread_state->srr0;
  
  return 0;
}
