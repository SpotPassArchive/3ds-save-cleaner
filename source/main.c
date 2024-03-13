#include <save.h>

#include <stdio.h>
#include <unistd.h>

//#define ERR_EXIT(msg) { perror(msg); goto bad_exit; }
//int fcpy(char *source, char *dest) {
//	FILE *src = NULL, *dst = NULL;
//	u8 *cpybuf = NULL;
//	int rv;
//
//	if (access(source, F_OK) != 0)
//		ERR_EXIT("cannot access source file");
//
//	struct stat st;
//	if (stat(source, &st) != 0)
//		ERR_EXIT("stat source failed");
//
//	printf("source size: %ld\n", st.st_size);
//
//	src = fopen(source, "r");
//	if (!src)
//		ERR_EXIT("failed opening source file for reading");
//
//	dst = fopen(dest, "w");
//	if (!dst)
//		ERR_EXIT("failed opening destination file for writing");
//	
//	if (ftruncate(fileno(dst), st.st_size) != 0)
//		ERR_EXIT("failed setting destination file size");
//
//	static const size_t cpybufsize = 1 * 1024 * 1024;
//
//	cpybuf = (u8 *)malloc(cpybufsize);
//
//	if (!cpybuf)
//		ERR_EXIT("failed allocating memory for copy buffer");
//
//	u64 remaining = st.st_size;
//	u64 to_write = MIN(cpybufsize, remaining);
//
//	while (remaining) {
//		if (fread(cpybuf, 1, to_write, src) != to_write)
//			ERR_EXIT("failed reading from source file");
//
//		if (fwrite(cpybuf, 1, to_write, dst) != to_write)
//			ERR_EXIT("failed writing to destination file");
//
//		remaining -= to_write;
//		to_write = MIN(cpybufsize, remaining);
//	}
//	rv = 1;
//exit:
//	if (src) fclose(src);
//	if (dst) fclose(dst);
//	if (cpybuf) free(cpybuf);
//	return rv;
//bad_exit:
//	rv = 0;
//	goto exit;
//}
//#undef ERR_EXIT

//char *alloc_extension(char *input, const char *extension) {
//	size_t len = snprintf(0, 0, "%s%s", input, extension) + 1;
//	char *buf = (char *)malloc(len);
//	if (!buf) return 0;
//	snprintf(buf, len, "%s%s", input, extension);
//	return buf;
//}

int main(int argc, char **argv) {
	save sav;
	int rv;

	if (argc != 2) {
		printf("usage: %s <path to partitionA.bin>\n", argv[0]);
		printf("note: the input file will be overwritten!\n");
		return 1;
	}

	if (!save_init(&sav, argv[1])) {
		fprintf(stderr, "failed initializing raw save\n");
		goto bad;
	}

	if (!save_perform_secure_erase(&sav)) {
		fprintf(stderr, "failed secure erasing file\n");
		goto bad;
	}

	rv = 0;
ret:
	save_close(&sav);
	return rv;
bad:
	rv = 1;
	goto ret;
}
