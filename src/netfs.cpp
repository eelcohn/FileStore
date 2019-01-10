/* netfs.cpp
 * Handler for all NetFS commands
 *
 * (c) Eelco Huininga 2017-2019
 */

#include <cstdlib>			// free()
#include <cstring>			// strchr()

#include "main.h"			// ECONET_MAX_DISCDRIVES
#include "adfs.h"
#include "netfs.h"			// FILESTORE_HANDLE
#include "settings.h"			// settings::*
#include "platforms/platform.h"

/* Temporary code to prevent -Wunused-parameter for now */
#include <cstdio>			// printf()


using namespace std;



namespace netfs {
	bool handles[FILESTORE_MAX_FILEHANDLES] = {false};

	int access(const char *fsp, const char *flags) {
		/* Temporary code to prevent -Wunused-parameter for now */
		printf("fsp: %s\n", fsp);
		printf("attributes (optional): %s\n", flags);
		return(0);
	}

	int cat(const char *fsp) {
		/* Temporary code to prevent -Wunused-parameter for now */
		printf("Drive/path (optional): %s\n", fsp);
		return(0);
	}

	int cdir(const char *dir) {
		/* Temporary code to prevent -Wunused-parameter for now */
		printf("fsp: %s\n", dir);
		return(0);
	}

	int del(const char *fsp) {
		/* Temporary code to prevent -Wunused-parameter for now */
		printf("fsp: %s\n", fsp);
		return(0);
	}

	int dismount(const char *disc) {
		int i;

		if(disc != NULL) {
			if(fp_volume != NULL) {
				fclose(fp_volume);
				fp_volume = NULL;
				settings::volume = NULL;
			}
		} else {
			for (i = 0; i < ECONET_MAX_DISCDRIVES; i++) {
				if(fp_volume != NULL)
					fclose(discs[i]->fp);
				if(discs[i] != NULL)
					free(discs[i]);
			}
		}

		return(0);
	}

	int ex(const char *fsp) {
		/* Temporary code to prevent -Wunused-parameter for now */
		printf("Filename: %s\n", fsp);
		return(0);
	}

	int info(const char *fsp) {
		/* Temporary code to prevent -Wunused-parameter for now */
		printf("Filename: %s\n", fsp);
		return(0);
	}

	int mount(const int slot, const char *image) {
		if(slot != -1) {
			if ((fp_volume = fopen(image, "r")) != NULL) {
				settings::volume = (unsigned char *)image;
			} else {
				fp_volume = NULL;
				return(0x000000D6);	// D6 File not found
			}			
		} else {
			printf("Not implemented: show help message\n");
		}

		return(0);
	}

	int rename(const char *oldfile, const char *newfile) {
		/* Temporary code to prevent -Wunused-parameter for now */
		printf("Current filename: %s\n", oldfile);
		printf("New filename: %s\n", newfile);
		return(0);
	}

	/* Reserves a new file handle */
	FILESTORE_HANDLE newhandle(void) {
		int i;

		for (i = 0; i < FILESTORE_MAX_FILEHANDLES; i++) {
			if (netfs::handles[i] == false) {
				netfs::handles[i] = true;
				return i;
			}
		}

		return -1;
	}

	/* Frees a file handle */
	void freehandle(FILESTORE_HANDLE handle) {
		netfs::handles[handle] = false;
	}

	/* Convert a string of object attributes to internal flags */
	void strtoattrib(const char *string, Attributes *attrib) {
		if (strchr(string, 'R') != NULL)
			attrib->R = true;
		else
			attrib->R = false;

		if (strchr(string, 'W') != NULL)
			attrib->W = true;
		else
			attrib->W = false;

		if (strchr(string, 'L') != NULL)
			attrib->L = true;
		else
			attrib->L = false;

		if (strchr(string, 'D') != NULL)
			attrib->D = true;
		else
			attrib->D = false;

		if (strchr(string, 'E') != NULL)
			attrib->E = true;
		else
			attrib->E = false;

		if (strchr(string, 'r') != NULL)
			attrib->r = true;
		else
			attrib->r = false;

		if (strchr(string, 'w') != NULL)
			attrib->w = true;
		else
			attrib->w = false;

		if (strchr(string, 'e') != NULL)
			attrib->e = true;
		else
			attrib->e = false;

		if (strchr(string, 'P') != NULL)
			attrib->P = true;
		else
			attrib->P = false;
	}

//private:
	const char *getDiscTitle(int i) {
		return discs[i]->image;
	}
}

