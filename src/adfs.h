/* adfs.h
 * All functions for manipulating ADFS disc images
 *
 * (c) Eelco Huininga 2017-2018
 */

#ifndef ECONET_USERS_HEADER
#define ECONET_USERS_HEADER

#define MAX_ADFS_PATHLENGTH 256	// Todo: find out what the max path length is according to official Acorn specs

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
	unsigned int		parent_pointer[3];	/* Pointer to parent directory (3 bytes) */
} ADFSDisc;

typedef struct {
	struct {
		unsigned char	name[10];	/* Name */
		unsigned int	loadaddr;	/* Load address (4 bytes) */
		unsigned int	execaddr;	/* Exec address (4 bytes) */
		unsigned int	length;		/* Length (4 bytes) */
		unsigned int	startsect;	/* Start sector (3 bytes) */
		unsigned char	sequencenr;	/* Sequence number (1 byte) */
		struct {			/* Attributes (encoded in bit 7 of Name) */
			bool	R;
			bool	W;
			bool	L;
			bool	D;
			bool	E;
			bool	r;
			bool	w;
			bool	e;
			bool	P;
		} attrib;
	} fsp[47];
} ADFSDir;

class adfs {
	ADFSDisc	*disc;

	bool	adfs::back(void);
	bool	adfs::cat(const char *objspec);
	bool	adfs::cdir(const char *objspec);
	bool	adfs::compact(const char *sp, const char *lp);
	bool	adfs::copy(const char *listspec, const char *objspec);
	bool	adfs::delete(const char *objspec);
	bool	adfs::dir(const char *objspec);
	int	adfs::mount(const char *objspec, const char *discname)
	bool	adfs::dismount(int handle);
};
#endif

