#ifndef _savetool_save_h
#define _savetool_save_h

#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

#define MIN(x,y) ((x) < (y) ? (x) : (y))

typedef struct __attribute__((packed)) table_info {
	u32 starting_block_index;
	u32 block_count;
	u32 max_entry_count;
} table_info;

/* starting block index -> rel to data region */
/* everything else -> rel to header start */
typedef struct __attribute__((packed)) save_fs_header {
	char magic[4]; /* 0x0-0x4 */
	u32 version; /* 0x4-0x8 */
	u64 fs_info_offset; /* 0x8-0x10 */
	u64 fs_image_size_blocks; /* 0x10-0x18 */
	u32 fs_image_blocksize; /* 0x18-0x1C */
	u32 __pad; /* 0x1C-0x20 */
	u32 _unk; /* 0x20-0x24 */
	u32 data_region_blocksize; /* 0x24-0x28 */
	u64 dir_hashtbl_offset; /* 0x28-0x30 */
	u32 dir_hashtbl_buckcount; /* 0x30-0x34 */
	u32 __pad1; /* 0x34-0x38 */
	u64 file_hashtbl_offset; /* 0x38-0x40 */
	u32 file_hashtbl_buckcount; /* 0x40-0x44 */
	u32 __pad2; /* 0x44-0x48 */
	u64 fat_offset; /* 0x48-0x50 */
	u32 fat_entry_count; /* 0x50-0x54 */
	u32 __pad3; /* 0x54-0x58 */
	u64 data_region_offset; /* 0x58-0x60 */
	u32 data_region_block_count; /* 0x60-0x64 */
	u32 __pad4; /* 0x64-0x68 */
	table_info dirtable_info; /* 0x68-0x74 */
	u32 __pad5; /* 0x74-0x78 */
	table_info filetable_info; /* 0x78-0x84 */
	u32 __pad6; /* 0x84-0x88 */
} save_fs_header;

typedef struct __attribute__((packed)) save_fat_entry_half {
	u32 index: 31;
	u32 flag: 1;
} save_fat_entry_half;
/*
 * for node head,
 * - U.index --> index of previous node head.
 * - U.flag --> set if this is the first node head.
 * - V.index --> index of next node head.
 * - V.flag --> whether or not this node has extended entries (multiple entries)
 */
/*
 * for extended node,
 * - U.index --> index of first entry in this node.
 * - U.flag --> always set.
 * - V.index --> index of last entry in this node.
 * - V.flag -->  never set.
 */
typedef struct __attribute__((packed)) save_fat_entry {
	save_fat_entry_half U;
	save_fat_entry_half V;
} save_fat_entry;

typedef struct _save_dir_entry {
	u32 parent_dir_index;
	char name[16];
	u32 next_sibling_dir_index;
	u32 first_subdir_index;
	u32 first_file_index; /* in file entry table */
	u32 __pad;
	u32 next_dir_in_bucket_index;
} _save_dir_entry;

typedef struct _save_dir_entry_dummy {
	u32 current_total_entry_count;
	u32 max_entry_count; /* max dir count + 2 */
	u8 __pad[0x1C];
	u32 next_dummy_index;
} _save_dir_entry_dummy;

typedef union __attribute__((packed)) save_dir_entry {
	_save_dir_entry_dummy dmy;
	_save_dir_entry ent;
} save_dir_entry;

typedef struct __attribute__((packed)) _save_file_entry {
	u32 parent_dir_index; /* in dir entry table */
	char name[16];
	u32 next_sibling_index;
	u32 __pad;
	u32 first_block_index; /* in data region; 0x80000000 if no data */
	u64 file_size;
	u32 _unk_pad;
	u32 next_file_in_bucket_index;
} _save_file_entry;

typedef struct _save_file_entry_dummy {
	u32 current_total_entry_count;
	u32 max_entry_count; /* max file count + 1 */
	u8 __pad[0x24];
	u32 next_dummy_index;
} _save_file_entry_dummy;

typedef union save_file_entry {
	_save_file_entry_dummy dmy;
	_save_file_entry ent;
} save_file_entry;

typedef struct save {
	save_fs_header header;
	u32 *dir_hashtbl;
	u32 *file_hashtbl;
	save_fat_entry *fat_entries;
	save_dir_entry *dir_entries;
	save_file_entry *file_entries;
	u32 unused_blocks_count;
	u8 *block_buffer;
	FILE *file;
} save;

int save_init(save *sav, char *path);
void save_close(save *sav);

int save_extract(save *save, char *path);
int save_perform_secure_erase(save *save);

#endif
