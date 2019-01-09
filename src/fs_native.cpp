/* fs_native.cpp
 * Native file system support
 *
 * (c) Eelco Huininga 2017-2019
 */

#include <cstdio>	// FILE*, fopen(), fclose(), fgetpos(), fsetpos(), fgetc(), fputc(), fread(), fwrite()
#include <cstring>	// basename()
#include <unistd.h>	// access()

#include "config.h"	// uint8_t, uint32_t
#include "main.h"	// ECONET_MAX_FILENAME_LEN
#include "netfs.h"	// FILESTORE_MAX_FILEHANDLES, FILESTORE_EOF, FILESTORE_HANDLE, newhandle(), freehandle(), struct Attributes

typedef struct {
	FILE *fp;
	char *filename;
	uint32_t loadaddr;
	uint32_t execaddr;
	uint32_t length;
	uint32_t pos;
	Attributes attrib;
} FILESTORE_NATIVE_FILEHANDLES;



namespace fs_native {
	FILESTORE_NATIVE_FILEHANDLES filehandles[FILESTORE_MAX_FILEHANDLES];

	FILESTORE_HANDLE open(const char *filename, const char *mode) {
		FILESTORE_HANDLE handle;
		FILE *fp_inffile;
		char inffilename[ECONET_MAX_FILENAME_LEN];
		char inf_filename[ECONET_MAX_FILENAME_LEN];
		char access[FILESTORE_MAX_ATTRIBS];
		long int filesize;

		if ((handle = netfs::newhandle()) != -1) {
			filehandles[handle].fp = fopen(filename, mode);
			if (filehandles[handle].fp == NULL) {
				return FILESTORE_EOF;
			} else {
				/* Assemble filename for .INF file */
				strlcpy(inffilename, basename(filename), sizeof(inffilename));
				strcat(inffilename, ".INF");

				/* Get filesize */
				fseek(filehandles[handle].fp, 0, SEEK_END);
				filesize = ftell(filehandles[handle].fp);
				fseek(filehandles[handle].fp, 0, SEEK_SET);

				/* Read contents of .INF file */
				fp_inffile = fopen(inffilename, "r");
				if (fp_inffile == NULL) {
					fscanf(fp_inffile, "%s %x %x %x %s", inf_filename, &filehandles[handle].loadaddr, &filehandles[handle].execaddr, &filehandles[handle].length, access);
					if (filehandles[handle].length != filesize)
						fprintf(stderr, "fopen Actual filesize and .INF filesize for %s differ!\n", filename);
					netfs::strtoattrib(access, &filehandles[handle].attrib);
				} else {
					filehandles[handle].loadaddr = 0;
					filehandles[handle].execaddr = 0;
					filehandles[handle].length = filesize;
				}

				/* Set internal variables */
				filehandles[handle].filename = (char *)filename;
				filehandles[handle].pos = 0;

				/* Return FileStore's file handle */
				return handle;
			}
		}

		fprintf(stderr, "fopen Unable to open file: Maximum number of open files reached");
		return FILESTORE_EOF;
	}

	int close(FILESTORE_HANDLE handle) {
		if (filehandles[handle].fp == NULL) {
			/* Invalid handle */
			return FILESTORE_EOF;
		} else {
			if (fclose(filehandles[handle].fp) == 0) {
				filehandles[handle].fp = NULL;
				filehandles[handle].filename = NULL;
				filehandles[handle].pos = 0;
				netfs::freehandle(handle);
				return 0;
			} else {
				/* Unable to close handle */
				return FILESTORE_EOF;
			}
		}
	}

	int getpos(FILESTORE_HANDLE handle, uint32_t &pos) {
		if (filehandles[handle].fp == NULL) {
			/* Invalid handle */
			return FILESTORE_EOF;
		} else {
			pos = filehandles[handle].pos;
			return 0;
		}
	}

	int setpos(FILESTORE_HANDLE handle, const uint32_t &pos) {
		if (filehandles[handle].fp == NULL) {
			/* Invalid handle */
			return FILESTORE_EOF;
		} else {
			filehandles[handle].pos = pos;
			return 0;
		}
	}

	int bget(FILESTORE_HANDLE handle) {
		if (filehandles[handle].fp == NULL) {
			/* Invalid handle */
			return FILESTORE_EOF;
		} else {
			if (filehandles[handle].pos <= filehandles[handle].length) {
				return fgetc(filehandles[handle].fp);
			} else {
				return FILESTORE_EOF;
			}
		}
	}

	size_t fread(void *ptr, uint32_t count, FILESTORE_HANDLE handle) {
		if (filehandles[handle].fp == NULL) {
			/* Invalid handle */
			return FILESTORE_EOF;
		} else {
			if (filehandles[handle].pos <= filehandles[handle].length) {
				return fread(ptr, sizeof(uint8_t), count, filehandles[handle].fp);
			} else {
				return FILESTORE_EOF;
			}
		}
	}

	int bput(int character, FILESTORE_HANDLE handle) {
		if (filehandles[handle].fp == NULL) {
			/* Invalid handle */
			return FILESTORE_EOF;
		} else {
			return fputc(character, filehandles[handle].fp);
		}
	}

	size_t fwrite(const void *ptr, uint32_t count, FILESTORE_HANDLE handle) {
		long int pos;
		size_t result;

		if (filehandles[handle].fp == NULL) {
			/* Invalid handle */
			return FILESTORE_EOF;
		} else {
			result = fwrite(ptr, sizeof(uint8_t), count, filehandles[handle].fp);
			
			/* Update file length if new file size has changed */
			pos = ftell(filehandles[handle].fp);
			if (pos > filehandles[handle].pos)
				filehandles[handle].pos = pos;

			/* Return fwrite() result */
			return result;
		}
	}
}
