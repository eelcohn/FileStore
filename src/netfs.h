/* netfs.h
 * Handler for all NetFS commands
 *
 * (c) Eelco Huininga 2017-2019
 */

#ifndef ECONET_NETFS_HEADER
#define ECONET_NETFS_HEADER

#include "config.h"	// int16_t

#define FILESTORE_MAX_FILEHANDLES 256
#define FILESTORE_EOF -1

#include "main.h"		/* ECONET_MAX_FILENAME_LEN */

typedef struct {		/* Attributes (encoded in bit 7 of Name) */
	bool	R;		/* Read access */
	bool	W;		/* Write access */
	bool	L;		/* Locked */
	bool	D;		/* Directory */
	bool	E;
	bool	r;
	bool	w;
	bool	e;
	bool	P;		/* Private item: invisible to all users except owners */
} FSAttributes;

typedef struct {
	char		name[ECONET_MAX_FILENAME_LEN];	/* Acorn filename */
	uint32_t	loadaddr;	/* Load address (4 bytes) */
	uint32_t	execaddr;	/* Exec address (4 bytes) */
	uint32_t	length;		/* Length (4 bytes) */
	uint32_t	startsect;	/* Start sector (3 bytes) */
	uint8_t		sequencenr;	/* Sequence number (1 byte) */
	FSAttributes	attrib;		/* Attributes (encoded in bit 7 of Name) */
	time_t		ctime;		/* Creation date/time */
	time_t		mtime;		/* Modification date/time */
} FSObject;

typedef struct {
	FSObject	fsp[ECONET_MAX_DIRENTRIES];	/* TODO: Replace [47] with dynamic memory allocation using malloc/realloc and double pointers */
} FSDirectory;


//typedef struct {
//	uint8_t		disc;
//	uint32_t	ptr;
//} FileHandles;

typedef int16_t FILESTORE_HANDLE;



namespace netfs {
	extern bool handles[FILESTORE_MAX_FILEHANDLES];

	int access(const char *fsp, const char *flags);
	int catalogue(uint8_t csd, FSDirectory *dir, const char *mask, uint8_t entrypoint, uint8_t numentries);
	int cdir(const char *dir);
	int del(const char *fsp);
	int dismount(const char *disc);
	int ex(const char *fsp);
	int info(const char *fsp);
	int mount(const int slot, const char *image);
	int rename(const char *oldfile, const char *newfile);
	FILESTORE_HANDLE newhandle(void);
	void freehandle(FILESTORE_HANDLE handle);
	void strtoattrib(const char *string, FSAttributes *attrib);
	intd attribtostr(const FSAttributes *attrib, char *string);

//private:
	const char *getDiscTitle(int i);
}
#endif

