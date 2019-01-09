/*-
 * Copyright (c) 1998, 2010 Ben Harris
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * This is part of aund, an implementation of Acorn Universal
 * Networking for Unix.
 */	
/*
 * fs_error.c -- generating errors
 */

#include <sys/types.h>

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "extern.h"
#include "fileserver.h"
#include "fs_proto.h"
#include "fs_errors.h"

const static struct {
	int errnoval;
	uint8_t fs_err;
} errnotab[] = {
	{ EPERM,       	EC_FS_E_NOPRIV },
	{ ENOENT,	EC_FS_E_NOTFOUND },
	{ EIO,		EC_FS_E_DISCERR },
	{ ENOMEM,	EC_FS_E_NOMEM },
	{ EACCES,	EC_FS_E_NOACCESS },
	{ EXDEV,	EC_FS_E_RENXDEV },
	{ ENOTDIR,	EC_FS_E_NOTDIR },
	{ EISDIR,	EC_FS_E_ISDIR },
	{ ENFILE,	EC_FS_E_MANYOPEN },
	{ EMFILE,	EC_FS_E_MANYOPEN },
	{ ENOSPC,	EC_FS_E_DISCFULL },
	{ EROFS,	EC_FS_E_DISCPROT },
	{ ENAMETOOLONG,	EC_FS_E_BADNAME },
	{ ENOTEMPTY,	EC_FS_E_DIRNOTEMPTY },
	{ EUSERS,	EC_FS_E_MANYUSERS },
	{ EDQUOT,	EC_FS_E_DISCFULL },
};

#define NERRNOS (sizeof(errnotab) / sizeof(errnotab[0]))

const static struct {
	uint8_t err;
	char *msg;
} errmsgtab[] = {
	{EC_FS_E_BADEXAMINE,	"Bad EXAMINE argument"},

	{0x69, "Object not a file"},

	{EC_FS_E_BADINFO,	"Bad INFO argument"},
	{EC_FS_E_BADARGS,	"Bad RDARGS argument"},

	{EC_FS_E_NOMEM, "Server out of memory"},

	{0xae, "User not logged on"},
	{0xaf, "Types don't match"},

	{0xb0, "Renaming across two discs"},
	{0xb1, "User id. already exists"},
	{0xb2, "Password file full"},
	{0xb3, "Maximum directory size reached"},
	{0xb4, "Directory not empty"},
	{0xb5, "Is a directory"},
	{0xb6, "Disc error on map read/write"},
	{0xb7, "Attempt to point outside a file"},
	{0xb8, "Too many users"},
	{0xb9, "Bad password"},
	{0xba, "Insufficient privilege"},
	{0xbb, "Incorrect password"},
	{0xbc, "User not known"},
	{0xbd, "Insufficient access"},
	{0xbe, "Object not a directory"},
	{0xbf, "Who are you?"},

	{0xc0, "Too many open files"},
	{0xc1, "File not open for update"},
	{0xc2, "Already open"},
	{0xc3, "Entry locked"},
	{0xc6, "Disc full"},
	{0xc7, "Unrecoverable disc error"},
	{0xc8, "Disc number not found"},
	{0xc9, "Disc protected"},
	{0xcc, "Bad file name"},
	{0xcf, "Invalid access string"},

	{0xd6, "Not found"},
	{0xde, "Channel"},
	{0xdf, "End of file"},

	{0xfd, "Bad string"},
	{0xfe, "Bad command"},
};

#define NMSGS (sizeof(errmsgtab)/sizeof(errmsgtab[0]))

void
fs_errno(struct fs_context *c)
{
	int i;

	for (i=0; i<NERRNOS; i++)
		if (errnotab[i].errnoval == errno) {
			fs_err(c, errnotab[i].fs_err);
			return;
		}
	fs_error(c, 0xff, strerror(errno));
}


void
fs_err(struct fs_context *c, uint8_t err)
{
	int i;

	for (i=0; i<NMSGS; i++)
		if (errmsgtab[i].err == err) {
			fs_error(c, err, errmsgtab[i].msg);
			return;
		}
	fs_error(c, err, "Internal server error");
}

void
fs_error(struct fs_context *c, uint8_t err, const char *report)
{
	struct ec_fs_reply *reply;

	if ((reply = malloc(sizeof(*reply) + strlen(report)+2)) == NULL)
		exit(2);
	reply->command_code = EC_FS_CC_DONE;
	reply->return_code = err;
	strcpy(reply->data, report);
	*strchr(reply->data, '\0') = 13;
	if (debug) printf("fs_error: 0x%x/%s\n", err, report);
	fs_reply(c, reply, sizeof(*reply) + strlen(report) + 1);
	free(reply);
}

