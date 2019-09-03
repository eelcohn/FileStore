/* fs_native.h
 * Native file system support
 *
 * (c) Eelco Huininga 2017-2019
 */

#ifndef ECONET_FS_NATIVE_HEADER
#define ECONET_FS_NATIVE_HEADER

#include "netfs.h"			/* FILESTORE_MAX_FILEHANDLES, FILESTORE_HANDLE */
#include "platforms/platform.h"		/* PATH_MAX */

typedef struct {
	char localobj[PATH_MAX];	/* Full path to object on local filesystem */
	FILE *fp;			/* If object is a file, this is the filepointer to this file */
	FSObject *obj;			/* ADFS Object information */
	uint32_t pos;			/* Sequential pointer */
} FILESTORE_NATIVE_FILEHANDLE;

namespace nativefs {
	extern FILESTORE_NATIVE_FILEHANDLE filehandles[FILESTORE_MAX_FILEHANDLES];

	FILESTORE_HANDLE open(const char *filename, const char *mode);
	int close(FILESTORE_HANDLE handle);
	size_t load(const char *localfile, char *buffer, uint32_t bufsize);
	size_t save(const char *localfile, char *buffer, uint32_t bufsize);
	int catalogue(const char *localpath, FSDirectory *dir, const char *mask, const uint8_t startentry, const uint8_t numentries);
	int remove(const char *objspec);
	int rename(const char *oldname, const char *newname);
	int getpos(FILESTORE_HANDLE handle, uint32_t &pos);
	int setpos(FILESTORE_HANDLE handle, const uint32_t &pos);
	int bget(FILESTORE_HANDLE handle);
	size_t read(void *ptr, uint32_t count, FILESTORE_HANDLE handle);
	int bput(int character, FILESTORE_HANDLE handle);
	size_t write(const void *ptr, uint32_t count, FILESTORE_HANDLE handle);
	void readinf(const char *filename, FSObject *obj);
	void writeinf(const char *filename, const FSObject *obj);
}
#endif

