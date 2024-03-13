#include <save.h>

void save_close(save *sav) {
	if (sav->file) { fclose(sav->file); sav->file = NULL; }
	if (sav->block_buffer) { free(sav->block_buffer); sav->block_buffer = NULL; }
//	if (sav->dir_hashtbl) { free(sav->dir_hashtbl); sav->dir_hashtbl = NULL; }
//	if (sav->file_hashtbl) { free(sav->file_hashtbl); sav->file_hashtbl = NULL; }
	if (sav->fat_entries) { free(sav->fat_entries); sav->fat_entries = NULL; }
	if (sav->dir_entries) { free(sav->dir_entries); sav->dir_entries = NULL; }
	if (sav->file_entries) { free(sav->file_entries); sav->file_entries = NULL; }
}

typedef int (* follow_cb)(save *save, u32 data_index, void *arg);

static int erase_block(save *save, u32 data_index, void *arg) {
	(void)arg;
	if (data_index) {
		u64 off = save->header.data_region_offset + (data_index * save->header.data_region_blocksize);
		if (fseek(save->file, off, SEEK_SET) != 0) {
			perror("failed seeking file");
			return 0;
		}
		if (fwrite(save->block_buffer, 1, save->header.data_region_blocksize, save->file) != save->header.data_region_blocksize) {
			perror("failed writing block");
			return 0;
		}
	}
	return 1;
}

typedef struct copy_data {
	u64 offset;
	u64 size;
	FILE *file;
} copy_data;

static int extract_file(save *save, u32 data_index, void *arg) {
	copy_data *cdata = (copy_data *)arg;
	u32 to_write = MIN(cdata->size - cdata->offset, save->header.data_region_blocksize);
	u64 off_read = save->header.data_region_offset + data_index * (save->header.data_region_blocksize);
	if (fseek(save->file, off_read, SEEK_SET) != 0) {
		perror("failed seeking save file");
		return 0;
	}
	if (fread(save->block_buffer, 1, to_write, save->file) != to_write) {
		perror("failed reading save file");
		return 0;
	}
	if (fwrite(save->block_buffer, 1, to_write, cdata->file) != to_write) {
		perror("failed writing to file");
		return 0;
	}
	cdata->offset += to_write;
	return 1;
}

static u32 follow_fat(save *save, u32 index, int is_ext, follow_cb cb, void *arg) {
	save_fat_entry *ent = &save->fat_entries[index];

	if (index && !cb(save, index - 1, arg))
		return UINT32_MAX;

	if (is_ext) return 0;
	/* process extended nodes first. */
	if (ent->V.flag) {
		/* get the end index of this node */
		u32 end_index = save->fat_entries[index + 1].V.index;
		for (u32 i = index + 1; i < end_index + 1; i++) {
			if (follow_fat(save, i, 1, cb, arg) == UINT32_MAX)
				return UINT32_MAX;
		}
	}

	if (ent->V.index) {
		/* go to next node, if we have one. */
		return ent->V.index;
	}

	return 0;
}

static int recurse_fat_chain(save *save, u32 initial_index, follow_cb cb, void *arg) {
	u32 idx = initial_index;
	do {
		idx = follow_fat(save, idx, 0, cb, arg);
	}
	while (idx != 0 && idx != UINT32_MAX);

	return idx == UINT32_MAX ? 0 : 1;
}

int read_block(void **output, FILE *save_file, u64 offset, u32 size) {
	*output = malloc(size);
	if (!*output) {
		fprintf(stderr, "out of memory\n");
		return 0;
	}

	if (fseek(save_file, offset, SEEK_SET) != 0) {
		perror("failed seeking to required offset");
		return 0;
	}
	
	if (fread(*output, 1, size, save_file) != size) {
		fprintf(stderr, "failed reading from file\n");
		return 0;
	}

	return 1;
}


int save_init(save *sav, char *path) {
	memset(sav, 0, sizeof(struct save));
	sav->file = fopen(path, "r+");
	if (!sav->file) {
		perror("failed opening file");
		return 0;
	}

	if (fread(&sav->header, 1, sizeof(save_fs_header), sav->file) != sizeof(save_fs_header)) {
		fprintf(stderr, "failed reading save header\n");
		return 0;
	}

	if (strncmp(sav->header.magic, "SAVE", 4) != 0) {
		fprintf(stderr, "invalid file magic\n");
		return 0;
	}

//	if (!read_block((void **)&sav->dir_hashtbl, sav->file, sav->header.dir_hashtbl_offset, sav->header.dir_hashtbl_buckcount * 4) ||
//		!read_block((void **)&sav->file_hashtbl, sav->file, sav->header.file_hashtbl_offset, sav->header.file_hashtbl_buckcount * 4)) {
//		fprintf(stderr, "failed reading hashtables\n");
//		return 0;
//	}

	if (!read_block((void **)&sav->fat_entries, sav->file, sav->header.fat_offset, (sav->header.fat_entry_count + 1) * sizeof(save_fat_entry))) {
		fprintf(stderr, "failed reading fat entries\n");
		return 0;
	}

#define TBL_OFF(x) (sav->header.data_region_offset + sav->header. x## table_info.starting_block_index * sav->header.data_region_blocksize)
#define TBL_SIZE(x) (sav->header.data_region_blocksize * sav->header. x## table_info.block_count)

	if (!read_block((void **)&sav->dir_entries, sav->file, TBL_OFF(dir), TBL_SIZE(dir)) ||
		!read_block((void **)&sav->file_entries, sav->file, TBL_OFF(file), TBL_SIZE(file))) {
		fprintf(stderr, "failed reading file/dir entries\n");
		return 0;
	}

#undef TBL_SIZE
#undef TBL_OFF

	sav->block_buffer = (u8 *)malloc(sav->header.data_region_blocksize);
	if (!sav->block_buffer) {
		fprintf(stderr, "failed allocating memory for block buffer\n");
		return 0;
	}
	memset(sav->block_buffer, 0, sav->header.data_region_blocksize);

	return 1;
}

int save_perform_secure_erase(save *save) {
	memset(save->block_buffer, 0, save->header.data_region_blocksize);
	return recurse_fat_chain(save, 0, erase_block, NULL);
}

char *alloc_sub_path(char *prevpath, char (* sub_name)[16]) {
	size_t prevlen = strlen(prevpath);
	size_t elen = strnlen((char *)sub_name, 16);
	char *curbuf = (char *)malloc(prevlen + elen + 1 + 1);
	if (!curbuf)
		return NULL;
	strncpy(curbuf, prevpath, prevlen);
	curbuf[prevlen] = '/';
	strncpy(&curbuf[prevlen + 1], (char *)sub_name, elen);
	curbuf[prevlen + 1 + elen] = '\0';
	return curbuf;
}

static inline save_dir_entry *save_root_dir(save *sav) {
	return &sav->dir_entries[1];
}

static int save_extract_recurse(save *save, save_dir_entry *root, char *path, int recurse) {
	mkdir(path, 0777);

	if (root->ent.next_sibling_dir_index) {
		char *slash = strrchr(path, '/');
		size_t base_size = (size_t)((slash + 1) - path); // should give us base/ so we can append {sibling} to get base/{sibling}
	
		save_dir_entry *sibling = root;

		while (sibling->ent.next_sibling_dir_index) {
			size_t elen = strnlen(sibling->ent.name, 16);
			char *npath = malloc(base_size + elen + 1);
			if (!npath) {
				fprintf(stderr, "failed allocating memory for file path\n");
				return 0;
			}
			strncpy(npath, path, base_size);
			strncpy(&npath[base_size], sibling->ent.name, elen);
			npath[base_size + elen] = '\0';
			sibling = &save->dir_entries[sibling->ent.next_sibling_dir_index];
			if (!save_extract_recurse(save, sibling, npath, recurse)) {
				free(npath);
				return 0;
			}
			free(npath);
		}
	}

	if (root->ent.first_subdir_index) {
		save_dir_entry *subdir = &save->dir_entries[root->ent.first_subdir_index];
		char *npath = alloc_sub_path(path, &subdir->ent.name);
		if (!npath) {
			fprintf(stderr, "failed allocating memory for file path\n");
			return 0;
		}
		if (!save_extract_recurse(save, subdir, npath, recurse + 1))
			return 0;
		free(npath);
	}

	if (!root->ent.first_file_index) return 1;
	save_file_entry *file = &save->file_entries[root->ent.first_file_index];
	while (1) {
		char *npath = alloc_sub_path(path, &file->ent.name);
		copy_data cdata;
		cdata.file = fopen(npath, "w");
		cdata.offset = 0;
		cdata.size = file->ent.file_size;
		if (!cdata.file) {
			free(npath);
			perror("failed creating file");
			return 1;
		}
		if (file->ent.first_block_index != 0x80000000) {
			if (!recurse_fat_chain(save, file->ent.first_block_index + 1, extract_file, &cdata)) {
				fprintf(stderr, "failed extracting file\n");
				fclose(cdata.file);
				free(npath);
				return 0;
			}
		}
		fclose(cdata.file);
		free(npath);
		if (!file->ent.next_sibling_index)
			break;
		file = &save->file_entries[file->ent.next_sibling_index];
	}
	return 1;
}

int save_extract(save *save, char *path) {
	return save_extract_recurse(save, save_root_dir(save), path, 0);
}
