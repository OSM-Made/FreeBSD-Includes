/*
	Credits to MIRA
*/

#ifndef __DYNLIB_H__
#define __DYNLIB_H__

//#ifdef _KERNEL
#include <sys/types.h>
#include <sys/lock.h>
#include <sys/sx.h>
#include <sys/elf64.h>

#if !defined(__cplusplus)
#include <sys/stddef.h>
#ifndef static_assert
#define static_asssert
#endif
#endif

typedef STAILQ_HEAD(Struct_Objlist, Struct_Objlist_Entry) Objlist;

struct dynlib_obj;
struct dynlib_obj_dyn;
struct dynlib;
struct dynlib_load_prx_args;
struct dynlib_dlsym_args;
struct dynlib_get_obj_member;
struct SceKdlPerFileInfo;


//variable size? (see pfi alloc)
struct SceKdlPerFileInfo
{
	char _unk0[0x10];
	int32_t _unk10;
	uint64_t _unk18;
	uint64_t _unk20;
	/* 0x10 | 28 */ caddr_t symtab;
	/* 0x18 | 30 */ size_t symtabsize;
	/* 0x20 | 38 */ caddr_t strtab;
	/* 0x28 | 40 */ size_t strsize;
	/* 0x30 | 48 */ caddr_t pltrela;
	/* 0x38 | 50 */ size_t pltrelasize;
	/* 0x40 | 58 */ caddr_t rela;
	/* 0x48 | 60 */ size_t relasize;
	char _unk50[0x50];
	/* 0xA0 | B8 */ caddr_t buckets;
	char _unkA8[0x8];
	/* 0xB0 | E8  */ int nbuckets;
	/* 0xB8 | D0 */ caddr_t chains;
	char _unkC0[0x8];
	/* 0xC8 | E0 */ int nchains;
	char _unkCC[0x1E];
	//not printed anymore by dump_obj on newer fws.
	/* 0xEA | 102 */ char file_format;
	/* 0xEB | 103 */ char is_prx;
};


struct dynlib_load_prx_args
{
  const char *prx_path;
  int flags;
  int *handle_out;
  uint64_t unk; //never used in (kernel 5.05) and always 0 (libkernel 7.00);
};

struct dynlib_dlsym_args {
    int32_t handle;
    const char* symbol;
    void** address_out;
};

struct dynlib_get_obj_member {
    uint32_t handle;
    uint32_t index;
    uint64_t value;
};

// Thank you flatz for 1.62
// Thank you ChendoChap for fixing newer fw's
struct dynlib
{
	SLIST_HEAD(, dynlib_obj) objs;
	struct dynlib* self;
	struct dynlib_obj* main_obj;
	struct dynlib_obj* libkernel_obj;
	struct dynlib_obj* asan_obj;
	uint32_t nmodules; // module count
	char unk2C[0x4];
	Objlist obj_list_0;
	Objlist obj_list_1;
	Objlist obj_list_2;
	Objlist obj_list_3;
	struct sx bind_lock;
	char unk90[0x18];
	uint8_t unkA8[0x8];
    uint8_t unkB0[0x8];
    uint8_t unkB8[0x8];
    uint8_t unkC0[0x4]; // set to 1
    uint8_t unkC4[0x4]; // set to 1, this and above optimized to one 0x100000001 (on creation)
	void* procparam_seg_addr;
	uint64_t procparam_seg_filesz;
	void* unpatched_call_addr;

	// [ includes
	// ( up to

	// [1.xx - 2.00)
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_100 && MIRA_PLATFORM < MIRA_PLATFORM_ORBIS_BSD_200
	char unkD0[0x4];
	char unkD4[0x8];
	char unkDC;
	char unkDD;
	char unkDE;
	char unkDF;
#endif

// [2.00 - 3.15]
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_200 && MIRA_PLATFORM <= MIRA_PLATFORM_ORBIS_BSD_315
	int rtld_dbg_msg_flag; 			//D0
	int is_sandboxed;				//D4
#endif

// [2.50 - 3.15]
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_250 && MIRA_PLATFORM <= MIRA_PLATFORM_ORBIS_BSD_315
	char unkD8[0x8];
#endif

// [3.50 - 3.70]
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_350 && MIRA_PLATFORM <= MIRA_PLATFORM_ORBIS_BSD_370
    uint32_t restrict_flags; //D8
    uint32_t no_dynamic_segment; //DC
    int is_sandboxed; //E0
    char unkE4[0x4];
#endif

// [4.00 - 6.20]
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_400 && MIRA_PLATFORM <= MIRA_PLATFORM_ORBIS_BSD_620
    void* sysc_s00_pointer;
    void* sysc_e00_pointer;
    uint32_t restrict_flags;
    uint32_t no_dynamic_segment;
    int is_sandboxed;
    char unkFC[0x4];
#endif

// [6.50 - *]
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_650
	void* __freeze_pointer;
    void* sysc_s00_pointer;
    void* sysc_e00_pointer;
    uint32_t restrict_flags; //flags of some kind, conditionally zeroes out some stuff in the dynlib  info_ex syscall and other places as well.
    uint32_t no_dynamic_segment; //also flags, used to conditionally load the asan? other bit used for sys_mmap_dmem?
    int is_sandboxed; //((proc->p_fd->fd_rdir != rootvnode) ? 1 : 0)   -> used to determine if it should use random path or system/ to load modules
	uint8_t unk104[0x4];
#endif

};

// Credits: flatz
struct dynlib_obj_dyn
{
	void* symtab_addr;
	uint64_t symtab_size;

	void* strtab_addr;
	uint64_t strtab_size;
};


// Credits: flatz
struct dynlib_obj
{
	SLIST_ENTRY(dynlib_obj) link; 	// 0x00
	char* path;						// 0x08
	char _unk10[0x10];				// 0x10
	int32_t ref_count;				// 0x20
	int32_t dl_ref_count;			// 0x24
	uint64_t handle;				// 0x28
	caddr_t map_base;				// 0x30
	size_t map_size;				// 0x38
	size_t text_size;				// 0x40
	caddr_t database;				// 0x48
	size_t data_size;				// 0x50
	char _unk58[0x10];				// 0x58
	size_t vaddr_base;				// 0x68
	caddr_t realloc_base;			// 0x70
	caddr_t entry;					// 0x78
	int32_t tls_index;  //94/80/90/80
	void* tls_init; //98/88/98/88
	size_t tls_init_size; //A0/90/A0/90
	size_t tls_size; //A8/98/A8/98
	size_t tls_offset; //B0/A0/B0/A0
	size_t tls_align; //B8/A8/B8/A8
	caddr_t plt_got; //C0/B0/C0/B0
	char _unkC8[0x38];				// 0xC8
	caddr_t init; 					//138/100/F0/100/F0
	caddr_t fini; 					//140/108/F8/108/F8
	uint64_t eh_frame_hdr; 			//148/110/100/110/100
	uint64_t eh_frame_hdr_size; 	//150/118/108/118/108
	uint64_t eh_frame; 				//158/120/110/120/110
	uint64_t eh_frame_size; 		//160/128/118/128/118
	int status; 					//168/130/120/130/120
	int flags; 						//bitfield/16C/134/124/134/124
	Objlist unkA; 					//170/138/128/138/128
	Objlist unkB; 					//180/148/138/148/138
	char _unk148[0x8];				// 0x148
	struct SceKdlPerFileInfo* pfi;	// 0x150
	char _unk158[0x18]; //1D0/198/158/168/158
};

//#endif // _KERNEL

#endif // __DYNLIB_H__