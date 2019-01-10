/* fs_native.h
 * Native file system support
 *
 * (c) Eelco Huininga 2017-2019
 */

#ifndef ECONET_FS_NATIVE_HEADER
#define ECONET_FS_NATIVE_HEADER

#include "netfs.h"	// FILESTORE_MAX_FILEHANDLES, FILESTORE_HANDLE

namespace fs_native {
	FILESTORE_NATIVE_FILEHANDLES filehandles[FILESTORE_MAX_FILEHANDLES];

	FILESTORE_HANDLE open(const char *filename, const char *mode);
	int close(FILESTORE_HANDLE handle);
	int getpos(FILESTORE_HANDLE handle, uint32_t &pos);
	int setpos(FILESTORE_HANDLE handle, const uint32_t &pos);
	int bget(FILESTORE_HANDLE handle);
	size_t fread(void *ptr, uint32_t count, FILESTORE_HANDLE handle);
	int bput(int character, FILESTORE_HANDLE handle);
	size_t fwrite(const void *ptr, uint32_t count, FILESTORE_HANDLE handle);
#endif

