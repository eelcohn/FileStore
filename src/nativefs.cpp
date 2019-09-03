/* fs_native.cpp
 * Native file system support
 *
 * (c) Eelco Huininga 2017-2019
 */

#include <cstdio>	// FILE*, fopen(), fclose(), fgetpos(), fsetpos(), fgetc(), fputc(), fread(), fwrite()
#include <cstring>	// basename()
#include <dirent.h>	// dirent, opendir, readdir
#include <unistd.h>	// access()
#include <sys/stat.h>	/* stat */

#include "config.h"	// uint8_t, uint32_t
#include "main.h"	// ECONET_MAX_FILENAME_LEN
#include "nativefs.h"	/* FILESTORE_NATIVE_FILEHANDLE */
#include "netfs.h"	// FILESTORE_MAX_FILEHANDLES, FILESTORE_EOF, FILESTORE_HANDLE, newhandle(), freehandle(), struct Attributes



namespace nativefs {
	FILESTORE_NATIVE_FILEHANDLE filehandles[FILESTORE_MAX_FILEHANDLES];

	FILESTORE_HANDLE open(const char *filename, const char *mode) {
		FILESTORE_HANDLE handle;
		long int filesize;

		if ((handle = netfs::newhandle()) != -1) {
			filehandles[handle].fp = fopen(filename, mode);
			if (filehandles[handle].fp == NULL) {
				return FILESTORE_EOF;
			} else {
				/* Get filesize */
				fseek(filehandles[handle].fp, 0, SEEK_END);
				filesize = ftell(filehandles[handle].fp);
				fseek(filehandles[handle].fp, 0, SEEK_SET);
				filehandles[handle].obj->length = filesize;

				/* Get extra information from .INF file */
				readinf(filename, filehandles[handle].obj);

				/* Set internal variables */
				strlcpy((char *) filehandles[handle].obj->name, filename, ECONET_MAX_FILENAME_LEN);
				filehandles[handle].pos = 0;

				/* Return FileStore's file handle */
				return handle;
			}
		}

		fprintf(stderr, "0x000000C0 fopen Unable to open file: Maximum number of open files reached");
		return FILESTORE_EOF;
	}

	int close(FILESTORE_HANDLE handle) {
		if (filehandles[handle].fp == NULL) {
			/* Invalid handle */
			return FILESTORE_EOF;
		} else {
			if (fclose(filehandles[handle].fp) == 0) {
				filehandles[handle].fp = NULL;
				filehandles[handle].obj->name[0] = '\0';
				filehandles[handle].pos = 0;
				netfs::freehandle(handle);
				return 0;
			} else {
				/* Unable to close handle */
				return FILESTORE_EOF;
			}
		}
	}

	size_t load(const char *localfile, char *buffer, uint32_t bufsize) {
		FILESTORE_HANDLE handle;
		int retval;

		if ((handle = open(localfile, "r")) == FILESTORE_EOF)
			return FILESTORE_EOF;

		if ((retval = read(buffer, bufsize, handle)) == FILESTORE_EOF)
			return FILESTORE_EOF;

		if ((close(handle)) == FILESTORE_EOF)
			return FILESTORE_EOF;

		return retval;
	}

	size_t save(const char *localfile, char *buffer, uint32_t bufsize) {
		FILESTORE_HANDLE handle;
		int retval;

		if ((handle = open(localfile, "w")) == FILESTORE_EOF)
			return FILESTORE_EOF;

		if ((retval = write(buffer, bufsize, handle)) == FILESTORE_EOF)
			return FILESTORE_EOF;

		if ((close(handle)) == FILESTORE_EOF)
			return FILESTORE_EOF;

		return retval;
	}

	int catalogue(const char *localpath, FSDirectory *dir, const char *mask, const uint8_t startentry, const uint8_t numentries) {
		DIR *localdir;
		struct dirent *direntry;
		struct stat64 localattribs;
		int i;

		i = 0;
		if (numentries <= ECONET_MAX_DIRENTRIES) {
			if ((localdir = opendir(localpath)) != NULL) {
				while (((direntry = readdir(localdir)) != NULL) && (i < ECONET_MAX_DIRENTRIES)) {
					if (i >= startentry) {
						strlcpy((char *) dir->fsp[i].name, direntry->d_name, ECONET_MAX_FILENAME_LEN);
						dir->fsp[i].loadaddr = 0;
						dir->fsp[i].execaddr = 0;
						dir->fsp[i].length = 0;
						dir->fsp[i].attrib.R = false;
						dir->fsp[i].attrib.W = false;
						dir->fsp[i].attrib.L = false;
						dir->fsp[i].attrib.D = false;
						dir->fsp[i].attrib.E = false;
						dir->fsp[i].attrib.r = false;
						dir->fsp[i].attrib.w = false;
						dir->fsp[i].attrib.e = false;
						dir->fsp[i].attrib.P = false;
						if (stat64(direntry->d_name, &localattribs)) {
							dir->fsp[i].length = localattribs.st_size;
							dir->fsp[i].ctime = localattribs.st_ctime;
							dir->fsp[i].mtime = localattribs.st_mtime;
							if ((S_ISDIR(localattribs.st_mode)) != 0)
								dir->fsp[i].attrib.D = true;
						} else
							fprintf(stderr, "stat failed\n");
						readinf(direntry->d_name, &dir->fsp[i]);
						i++;
					}
				}
				closedir(localdir);
				return i;
			}
		}

		return 0;
	}

	int remove(const char *objspec) {
		int i;

		for (i = 0; i < FILESTORE_MAX_FILEHANDLES; i++)
			if (strcmp((const char *) filehandles[i].obj->name, objspec) == 0)
				return 0x000000C3;				/* Object is open: return 'Object locked' */

		return remove(objspec);
	}

	int rename(const char *oldname, const char *newname) {
		FILE *fp;
		int i;

		for (i = 0; i < FILESTORE_MAX_FILEHANDLES; i++)
			if (strcmp((const char *) filehandles[i].obj->name, oldname) == 0)
				return 0x000000C3;				/* Object is open: return 'Object locked' */

		if ((fp = fopen(newname, "r")) != NULL) {
			fclose(fp);
			return 0x000000C4;				/* Object already exists */
		}

		return rename(oldname, newname);
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
			if (filehandles[handle].pos <= filehandles[handle].obj->length) {
				return fgetc(filehandles[handle].fp);
			} else {
				return FILESTORE_EOF;
			}
		}
	}

	size_t read(void *ptr, uint32_t count, FILESTORE_HANDLE handle) {
		if (filehandles[handle].fp == NULL) {
			/* Invalid handle */
			return FILESTORE_EOF;
		} else {
			if (filehandles[handle].pos <= filehandles[handle].obj->length) {
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

	size_t write(const void *ptr, uint32_t count, FILESTORE_HANDLE handle) {
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

	void readinf(const char *filename, FSObject *obj) {
		FILE *fp_inffile;
		char inffilename[ECONET_MAX_FILENAME_LEN];
		char inf_filename[ECONET_MAX_FILENAME_LEN];
		char access[FILESTORE_MAX_ATTRIBS];
		uint32_t length;

		/* Assemble filename for .INF file */
		strlcpy(inffilename, basename(filename), sizeof(inffilename));
		strcat(inffilename, ".INF");

		/* Read contents of .INF file */
		fp_inffile = fopen(inffilename, "r");
		if (fp_inffile != NULL) {
			fscanf(fp_inffile, "%s %x %x %x %s", inf_filename, &obj->loadaddr, &obj->execaddr, &length, access);
			netfs::strtoattrib(access, &obj->attrib);
			if (obj->length != length)
				fprintf(stderr, "nativefs::open Actual filesize and .INF filesize for %s differ!\n", filename);
			fclose(fp_inffile);
		} else {
			obj->loadaddr = 0;
			obj->execaddr = 0;
		}
	}

	void writeinf(const char *filename, const FSObject *obj) {
		FILE *fp_inffile;
		char inffilename[ECONET_MAX_FILENAME_LEN];
		char inf_filename[ECONET_MAX_FILENAME_LEN];
		char access[FILESTORE_MAX_ATTRIBS];
		uint32_t length;

		/* Assemble filename for .INF file */
		strlcpy(inffilename, basename(filename), sizeof(inffilename));
		strcat(inffilename, ".INF");

		/* Write contents of .INF file */
		fp_inffile = fopen(inffilename, "w");
		if (fp_inffile != NULL) {
			netfs::attribtostr(&obj->attrib, access);
			fprintf(fp_inffile, "%s %08x %08x %06x %s", inf_filename, obj->loadaddr, obj->execaddr, length, access);
			fclose(fp_inffile);
		} else {
			fprintf(stderr, "nativefs::writeinf Unable to write .INF file %s\n", inffilename);
		}
	}
}
