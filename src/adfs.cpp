/* adfs.cpp
 * All functions for manipulating ADFS disc images
 *
 * (c) Eelco Huininga 2017-2019
 */

// http://www.6809.org.uk/fstools/fstools-0.1.tar.gz

#include <cstdio>			// NULL, printf(), fopen(), fgets(), feof() and fclose()
#include <cstdlib>			// calloc()
#include <cstring>			// strcmp()
#include "adfs.h"
#include "main.h"			// strlcpy()

#define ADFS_MAX_PATHLENGTH 256

char csd[ADFS_MAX_PATHLENGTH];	// Currently Selected Directory
char psd[ADFS_MAX_PATHLENGTH];	// Previously Selected Directory
char cld[ADFS_MAX_PATHLENGTH];	// Currently Selected Library

ADFSDisc adfsdisc;



int adfs::access(const char *listspec, const char *attributes) {
	int i;
	size_t a;

	// Load directory entries
	for (i = 0; i < 47; i++) {
		if (strcmp((char *)adfsdisc.dir[0]->fsp[i].name, listspec) == 0) {
			for (a = 0; a < strlen(attributes); a++) {
				switch(attributes[a]) {
					case 'R' :
						break;

					case 'W' :
						break;

					case 'L' :
						break;

					case 'D' :
						break;

					case 'E' :
						break;

					case 'r' :
						break;

					case 'w' :
						break;

					case 'e' :
						break;

					case 'P' :
						break;

					default :
						return 0x000000CF;
						break;
				}
			}
		}
	}

	
	return 0;
}

int adfs::back(void) {
	char temp[ADFS_MAX_PATHLENGTH];

	strlcpy(temp, csd, sizeof(temp));
	strlcpy(csd, psd, sizeof(csd));
	strlcpy(psd, temp, sizeof(psd));
	return 0;
}

int adfs::cat(const char *objspec) {
	/* Temporary code to prevent -Wunused-parameter for now */
	printf("%s\n\n", objspec);

	printf("Business Letters    (13)\n");
	printf("Drive: 0            Option 00 (Off)\n");
	printf("Dir. BusLet         Lib. Library1\n");
	printf("\n");
	printf("File1       WR (08) File2       WR (09) Glenn       WR (00) XDir        DLR(05)\n");
	printf("Z-test-4    WR (12) Z-test-5    LWR(13)\n");
	return 0;
}

int adfs::cdir(const char *objspec) {
	int i, j, startsect;

	startsect = -1;

	if (strcmp(objspec, "$"))
		startsect = 2;
	else {
		for (i = 0; i < 47; i++) {
			if (strcmp(objspec, (char *)adfsdisc.dir[0]->fsp[i].name) == 0)
				startsect = adfsdisc.dir[0]->fsp[i].startsect;
		}
	}

	if (startsect != -1) {
		// Load directory entries
		for (i = 0; i < 47; i++) {
			for (j = 0; j < 10; i++) {
				adfsdisc.dir[0]->fsp[i].name[j] = fgetc(adfsdisc.fp);
				switch (j) {
					case 0 :	// R - Object is readable
						adfsdisc.dir[0]->fsp[i].attrib.R = (adfsdisc.dir[0]->fsp[i].name[j] & 0x80);
						break;

					case 1 :	// W - Object is writable
						adfsdisc.dir[0]->fsp[i].attrib.W = (adfsdisc.dir[0]->fsp[i].name[j] & 0x80);
						break;

					case 2 :	// L - Object is locked
						adfsdisc.dir[0]->fsp[i].attrib.L = (adfsdisc.dir[0]->fsp[i].name[j] & 0x80);
						break;

					case 3 :	// D - Object is a directory
						adfsdisc.dir[0]->fsp[i].attrib.D = (adfsdisc.dir[0]->fsp[i].name[j] & 0x80);
						break;

					case 4 :	// E - Object is executable-obly
						adfsdisc.dir[0]->fsp[i].attrib.E = (adfsdisc.dir[0]->fsp[i].name[j] & 0x80);
						break;

					case 5 :	// r - Object is publicly readable
						adfsdisc.dir[0]->fsp[i].attrib.r = (adfsdisc.dir[0]->fsp[i].name[j] & 0x80);
						break;

					case 6 :	// w - Object is publicly writable
						adfsdisc.dir[0]->fsp[i].attrib.w = (adfsdisc.dir[0]->fsp[i].name[j] & 0x80);
						break;

					case 7 :	// e - Object is publicly executable
						adfsdisc.dir[0]->fsp[i].attrib.e = (adfsdisc.dir[0]->fsp[i].name[j] & 0x80);
						break;

					case 8 :	// P - Object is private
						adfsdisc.dir[0]->fsp[i].attrib.P = (adfsdisc.dir[0]->fsp[i].name[j] & 0x80);
						break;

					default :
						break;
				}
				adfsdisc.dir[0]->fsp[i].name[j] &= 0x80;
			}
			adfsdisc.dir[0]->fsp[i].loadaddr = fgetc(adfsdisc.fp) + (fgetc(adfsdisc.fp) * 0x100) + (fgetc(adfsdisc.fp) * 0x10000) + (fgetc(adfsdisc.fp) * 0x1000000);
			adfsdisc.dir[0]->fsp[i].execaddr = fgetc(adfsdisc.fp) + (fgetc(adfsdisc.fp) * 0x100) + (fgetc(adfsdisc.fp) * 0x10000) + (fgetc(adfsdisc.fp) * 0x1000000);
			adfsdisc.dir[0]->fsp[i].length = fgetc(adfsdisc.fp) + (fgetc(adfsdisc.fp) * 0x100) + (fgetc(adfsdisc.fp) * 0x10000) + (fgetc(adfsdisc.fp) * 0x1000000);
			adfsdisc.dir[0]->fsp[i].startsect = fgetc(adfsdisc.fp) + (fgetc(adfsdisc.fp) * 256) + (fgetc(adfsdisc.fp) * 65536);
			adfsdisc.dir[0]->fsp[i].sequencenr = fgetc(adfsdisc.fp);
		}

		j = fgetc(adfsdisc.fp);	// Dummy byte
		adfsdisc.msn = fgetc(adfsdisc.fp);
		adfsdisc.parent_pointer = fgetc(adfsdisc.fp) + (fgetc(adfsdisc.fp) * 256) + (fgetc(adfsdisc.fp) * 65536);
		for (j = 0; j < 19; i++)
			adfsdisc.dirtitle[j] = fgetc(adfsdisc.fp);
		for (j = 0; j < 10; i++)
			adfsdisc.dirname[j] = fgetc(adfsdisc.fp);
		for (j = 0; j < 4; i++)
			adfsdisc.diridentity[j] = fgetc(adfsdisc.fp);	// == Hugo
	}
	return 0;
}

int adfs::compact(const char *sp, const char *lp) {
	/* Temporary code to prevent -Wunused-parameter for now */
	printf("%s\n", sp);
	printf("%s\n", lp);
	return 0;
}

int adfs::copy(const char *listspec, const char *objspec) {
	/* Temporary code to prevent -Wunused-parameter for now */
	printf("%s\n", listspec);
	printf("%s\n", objspec);
	return 0;
}

int adfs::del(const char *objspec) {
	/* Temporary code to prevent -Wunused-parameter for now */
	printf("%s\n", objspec);
	return 0;
}

int adfs::dir(const char *objspec) {
	strlcpy(psd, csd, sizeof(psd));
	strlcpy(csd, objspec, sizeof(csd));
	return 0;
}

int adfs::dismount(int handle) {
	/* Temporary code to prevent -Wunused-parameter for now */
	printf("%i\n", handle);

//	if (adfsdisc[handle].fp != NULL) {
	if (adfsdisc.fp != NULL) {
		fclose (adfsdisc.fp);
		free(adfsdisc.dir[0]);
		return 0;
	}
	return -1;
}

int adfs::ex(const char *objspec) {
	/* Temporary code to prevent -Wunused-parameter for now */
	printf("%s\n", objspec);
	return 0;
}

int adfs::lib(const char *objspec) {
	strlcpy(psd, csd, sizeof(psd));
	strlcpy(cld, objspec, sizeof(cld));
	return 0;
}

int adfs::mount(const char *objspec, const char *discname) {
	int i, handle;
	ADFSDir adfsdir;

	/* Temporary code to prevent -Wunused-parameter for now */
	printf("%s\n", discname);

	if ((adfsdisc.fp = fopen(objspec, "r")) == NULL) {
		return (0x000000D6);
	}

	if ((adfsdisc.dir[0] = (ADFSDir *) malloc(1 * sizeof(adfsdir))) == NULL) {
		fclose(adfsdisc.fp);
//		errorHandler(0xFFFFFFFF, "adfs::mount: allocation error");
		return (0xFFFFFFFF);
	}

	// Load free space map
	for (i = 0; i < 82; i++)
		adfsdisc.fsm[i].addr = fgetc(adfsdisc.fp) + (fgetc(adfsdisc.fp) * 256) + (fgetc(adfsdisc.fp) * 65536);

	fseek(adfsdisc.fp, 252, SEEK_SET);
	adfsdisc.totalsectors = fgetc(adfsdisc.fp) + (fgetc(adfsdisc.fp) * 256) + (fgetc(adfsdisc.fp) * 65536);
	adfsdisc.sect0checksum = fgetc(adfsdisc.fp);

	for (i = 0; i < 82; i++)
		adfsdisc.fsm[i].length = fgetc(adfsdisc.fp) + (fgetc(adfsdisc.fp) * 256) + (fgetc(adfsdisc.fp) * 65536);

	fseek(adfsdisc.fp, 251, SEEK_SET);
	adfsdisc.discidentifier = fgetc(adfsdisc.fp) + (fgetc(adfsdisc.fp) * 256);
	adfsdisc.bootoption = fgetc(adfsdisc.fp);
	adfsdisc.end_of_free_space_list_ptr = fgetc(adfsdisc.fp);

	// Change directory to $, and load directory entries
	adfs::cdir("$");

	// Return the handle for this disc image
	handle = 0;
	return handle;
}

