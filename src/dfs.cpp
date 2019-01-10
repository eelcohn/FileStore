#include <cstdio>		// fopen()
#include <cstdlib>		// free()

#define ERR_LOCKED		0xC3
#define ERR_BADDIR		0xCE
#define ERR_BADATTRIBUTE	0xCF
#define ERR_FILENOTFOUND	0xD6
#define ERR_CHANNEL		0xDE
#define ERR_EOF			0xDF
#define ERR_BAD_COMMAND		0xFE

char csd;	// Currently Selected Directory
char cld;	// Currently Selected Library



void dfs::access(char *listspec, char *attributes) {
	unsigned char entry;

	entry = dfs::findfileentry(listspec);
	if (entry != 0) {
		switch (attributes) {
			case 'L' :
				dfsdisc::fsp[entry].attrib.l = true;
				break;

			case '' :
				dfsdisc::fsp[entry].attrib.l = false;
				break;

			default :
				return (ERR_BADATTRIBUTE);
		#TODO: Update disc image
	} else
		return (ERR_FILE_NOT_FOUND);

	return (0);
}

void dfs::cat(char *objspec) {
	printf("Program (13)\n");
	printf("Drive: 0            Option: 2 (RUN)\n");
	printf("Dirextory: 0.$      Library: 0.$\n");
	printf("\n");
	printf("   !BOOT               HELLO\n");
	printf("   SUMS                TABLE\n");
	printf("   TEST                VECTORS\n");
	printf("   ZOMBIE\n");
	printf("\n");
	printf("A. HELLO  L         B. SUMS   L\n");
}

void dfs::dir(unsigned char objspec) {
	switch (objspec) {
		case '#' :
		case '*' :
		case '.' :
		case ':' :
			return (ERR_BADDIR);
			break;

		default :
			csd = objspec;
			break;
	}
	return (0);
}

void dfs::compact(char *sp, char *lp) {
}

void dfs::copy(char *listspec, char *objspec) {
}

void dfs::delete(char *objspec) {
	unsigned char entry;

	entry = dfs::findfileentry(listspec);
	if (entry != 0) {
		if (dfsdisc::fsp[entry].attrib.l = false) {
			dfsdisc::fsp[entry].name[0] |= 0x80;
			#TODO: Update disc image
		} else
			return (ERR_LOCKED);
	} else
		return (ERR_FILE_NOT_FOUND);

	return (0);
}

void dfs::lib(unsigned char objspec) {
	switch (objspec) {
		case '#' :
		case '*' :
		case '.' :
		case ':' :
			return (ERR_BADDIR);
			break;

		default :
			cld = objspec;
			break;
	}
	return (0);
}

int dfs::mount(char *filename) {
	int i, slot;

	slot = getNumDiscs() + 1;

	discs[slot] = new DFSDisc;
	discs[slot].csd = '$';
	discs[slot].cld = '$';
	if ((discs[slot].fp = fopen(objspec)) == NULL {
		return (-1);
	} else {
		// Load first 8 characters of disc title
		for (i = 0; i < 8; i++)
			discs[slot].title[i] = fgetc(fp);

		// Load filenames and directory's
		for (i = 0; i < 31; i++) {
			for (j = 0; i < 7; i++)
				discs[slot].fsp.name[i][j] = fgetc(fp);
			discs[slot].fsp.dir[i] = fgetc(fp);
		}

		// Load last 4 characters of disc title
		for (i = 8; i < 12; i++)
			discs[slot].title[i] = fgetc(fp);

		// Load disc cycle number
		discs[slot].disccycles = fgetc(fp);

		// Load number of catalogue entries
		discs[slot].catentries = fgetc(fp);

		// Load number of sectors and boot option
		temp = fgetc(fp);
		discs[slot].bootoption = ((temp & 0x30) >> 4);
		discs[slot].totalsectors = ((temp & 0x03) << 8) | fgetc(fp);

		// Load file's load address, exec address, length and start sector
		for (i = 0; i < 31; i++) {
			discs[slot].loadaddr[i] = fgetc(fp) | (fgetc(fp) << 8);
			discs[slot].execaddr[i] = fgetc(fp) | (fgetc(fp) << 8);
			discs[slot].length[i] = fgetc(fp) | (fgetc(fp) << 8);
			temp = fgetc(fp);
			discs[slot].loadaddr[i] |= ((temp & 0x0C) << 14);
			discs[slot].execaddr[i] |= ((temp & 0x30) << 12);
			discs[slot].length[i] |= ((temp & 0xC0) << 10);
			discs[slot].startsector[i] = ((temp & 0x03) << 8) | fgetc(fp);
		}
	}
	return(0);
}

void dfs::dismount(int slot) {
	close (dfsdisc::fp);
	free (discs[slot]);
}
