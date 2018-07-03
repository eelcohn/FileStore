/* adfs.cpp
 * All functions for manipulating ADFS disc images
 *
 * (c) Eelco Huininga 2017-2018
 */

// http://www.6809.org.uk/fstools/fstools-0.1.tar.gz

#define ADFS_MAX_PATHLENGTH 256;

char csd[ADFS_MAX_PATHLENGTH];	// Currently Selected Directory
char psd[ADFS_MAX_PATHLENGTH];	// Previously Selected Directory
char cld[ADFS_MAX_PATHLENGTH];	// Currently Selected Library





void adfs::access(char *listspec, char *attributes) {
}

void adfs::back(void) {
	char temp[ADFS_MAX_PATHLENGTH];

	strlcpy(temp, csd, sizeof(temp));
	strlcpy(csd, psd, sizeof(csd));
	strlcpy(psd, temp, sizeof(psd));
}

void adfs::cat(char *objspec) {
	printf("Business Letters    (13)\n");
	printf("Drive: 0            Option 00 (Off)\n");
	printf("Dir. BusLet         Lib. Library1\n");
	printf("\n");
	printf("File1       WR (08) File2       WR (09) Glenn       WR (00) XDir        DLR(05)\n");
	printf("Z-test-4    WR (12) Z-test-5    LWR(13)\n");
}

void adfs::cdir(char *objspec) {
	int i, startsect;

	startsect = -1;

	if (strcmp(objspec, "$"))
		startsect = 2;
	else {
		for (i = 0; i < 47; i++) {
			if (strcmp(objspec, adfsdisc.fsp.name[i]))
				startsect = adfsdisc::dsp.startsect[i];
		}
	}

	if (startsect != -1) {
		// Load directory entries
		for (i = 0; i < 47; i++) {
			for (j = 0; j < 10; i++) {
				adfsdisc::fsp.name[i][j] = fgetc(fp);
				switch (j) {
					case 0 :	// R - Object is readable
						adfsdisc::fsp.attrib.R = (adfsdisc::fsp.name[i][j] & 0x80);
						break;

					case 1 :	// W - Object is writable
						adfsdisc::fsp.attrib.W = (adfsdisc::fsp.name[i][j] & 0x80);
						break;

					case 2 :	// L - Object is locked
						adfsdisc::fsp.attrib.L = (adfsdisc::fsp.name[i][j] & 0x80);
						break;

					case 3 :	// D - Object is a directory
						adfsdisc::fsp.attrib.D = (adfsdisc::fsp.name[i][j] & 0x80);
						break;

					case 4 :	// E - Object is executable-obly
						adfsdisc::fsp.attrib.E = (adfsdisc::fsp.name[i][j] & 0x80);
						break;

					case 5 :	// r - Object is publicly readable
						adfsdisc::fsp.attrib.r = (adfsdisc::fsp.name[i][j] & 0x80);
						break;

					case 6 :	// w - Object is publicly writable
						adfsdisc::fsp.attrib.w = (adfsdisc::fsp.name[i][j] & 0x80);
						break;

					case 7 :	// e - Object is publicly executable
						adfsdisc::fsp.attrib.e = (adfsdisc::fsp.name[i][j] & 0x80);
						break;

					case 8 :	// P - Object is private
						adfsdisc::fsp.attrib.P = (adfsdisc::fsp.name[i][j] & 0x80);
						break;

					default :
						break;
				}
				adfsdisc::fsp.name[i][j] &= 0x80;
			}
			adfsdisc::fsp.loadaddr[i] = fgetc(fp) + (fgetc(fp) * 0x100) + (fgetc(fp) * 0x10000) + (fgetc(fp) * 0x1000000);
			adfsdisc::fsp.execaddr[i] = fgetc(fp) + (fgetc(fp) * 0x100) + (fgetc(fp) * 0x10000) + (fgetc(fp) * 0x1000000);
			adfsdisc::fsp.length[i] = fgetc(fp) + (fgetc(fp) * 0x100) + (fgetc(fp) * 0x10000) + (fgetc(fp) * 0x1000000);
			adfsdisc::fsp.startsect[i] = fgetc(fp) + (fgetc(fp) * 256) + (fgetc(fp) * 65536);
			adfsdisc::fsp.sequencenr[i] = fgetc(fp);
		}

		j = fgetc(fp);	// Dummy byte
		adfsdisc::msn = fgetc(fp);
		adfsdisc::parentptr = fgetc(fp) + (fgetc(fp) * 256) + (fgetc(fp) * 65536);
		for (j = 0; j < 19; i++)
			adfsdisc::dirtitle[j] = fgetc(fp);
		for (j = 0; j < 10; i++)
			adfsdisc::dirname[j] = fgetc(fp);
		for (j = 0; j < 4; i++)
			adfsdisc::diridentity[j] = fgetc(fp);	// == Hugo
	}
}

void adfs::compact(char *sp, char *lp) {
}

void adfs::copy(char *listspec, char *objspec) {
}

void adfs::delete(char *objspec) {
}

void adfs::dir(char *objspec) {
	strlcpy(psd, csd, sizeof(psd));
	strlcpy(csd, objspec, sizeof(csd));
}

int adfs::mount(char *objspec) {
	int i;

	adfsdisc::fp = fopen(objspec);

	// Load free space map
	for (i = 0; i < 82; i++)
		adfsdisc::fsm.addr[i] = fgetc(fp) + (fgetc(fp) * 256) + (fgetc(fp) * 65536);

	fseek(fp, 252);
	adfsdisc::totalsectors = fgetc(fp) + (fgetc(fp) * 256) + (fgetc(fp) * 65536);
	adfsdisc::sect0checksum = fgetc(fp);

	for (i = 0; i < 82; i++)
		adfsdisc::fsm.length[i] = fgetc(fp) + (fgetc(fp) * 256) + (fgetc(fp) * 65536);

	fseek(fp, 251);
	adfsdisc::discidentifier = fgetc(fp) + (fgetc(fp) * 256);
	adfsdisc::bootoption = fgetc(fp);
	adfsdisc::end_of_free_space_list_ptr = fgetc(fp);

	// Change directory to $, and load directory entries
	adfs::cdir("$");

	// Return the handle for this disc image
	return handle;
}

void adfs::dismount(int handle) {
	fclose (adfs::disc[handle].fp);
}
