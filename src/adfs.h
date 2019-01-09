/* adfs.h
 * All functions for manipulating ADFS disc images
 *
 * (c) Eelco Huininga 2017-2019
 */

#ifndef ECONET_ADFS_HEADER
#define ECONET_ADFS_HEADER

#define MAX_ADFS_PATHLENGTH 256	// Todo: find out what the max path length is according to official Acorn specs

#include <cstdio>			// FILE*

#include "netfs.h"			// struct Attributes

typedef struct {
	struct {
		unsigned char	name[10];	/* Name */
		unsigned int	loadaddr;	/* Load address (4 bytes) */
		unsigned int	execaddr;	/* Exec address (4 bytes) */
		unsigned int	length;		/* Length (4 bytes) */
		unsigned int	startsect;	/* Start sector (3 bytes) */
		unsigned char	sequencenr;	/* Sequence number (1 byte) */
		Attributes	attrib;		/* Attributes (encoded in bit 7 of Name) */
	} fsp[47];
} ADFSDir;

typedef struct {
	FILE *fp;
	struct {
		unsigned int	addr;		/* Disc address of free space area (3 bytes) */
		unsigned int	length;		/* Length of free space area (3 bytes) */
	} fsm[82];
	unsigned int		totalsectors;	/* Total number of sectors on disc (3 bytes) */
	unsigned short		discidentifier;	/* Disc identifier (2 bytes) */
	unsigned char		bootoption;	/* Boot option (1 byte) */
	ADFSDir			**dir;		/* Pointer to directory's (0 = root dir) */
	unsigned char		msn;			/* Master sequence number (1 byte) */
	unsigned char		dirtitle[19];		/* Directory title (19 bytes) */
	unsigned char		dirname[10];		/* TODO: find out if this is really part of the ADFS directory structure */
	unsigned char		diridentity[4];		/* TODO: find out if this is really part of the ADFS directory structure */
	unsigned int		parent_pointer;		/* Pointer to parent directory (3 bytes) */
	unsigned char		sect0checksum;		/* TODO: find out if this is really part of the ADFS directory structure */
	unsigned char		end_of_free_space_list_ptr;
} ADFSDisc;

class adfs {
	ADFSDisc	*disc;

	int	access(const char *listspec, const char *attributes);
	int	back(void);
	int	cat(const char *objspec);
	int	cdir(const char *objspec);
	int	compact(const char *sp, const char *lp);
	int	copy(const char *listspec, const char *objspec);
	int	del(const char *objspec);
	int	dir(const char *objspec);
	int	dismount(int handle);
	int	ex(const char *objspec);
	int	lib(const char *objspec);
	int	mount(const char *objspec, const char *discname);
};

extern ADFSDisc			adfsdisc;

#endif

