/*-
 * Copyright (c) 2013, 2010 Simon Tatham
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
 * fs_fileio.c - File server file I/O calls
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "aun.h"
#include "fs_proto.h"
#include "fs_errors.h"
#include "extern.h"
#include "fileserver.h"

#define OUR_DATA_PORT 0x97

static ssize_t fs_data_send(struct fs_context *, int, size_t);
static ssize_t fs_data_recv(struct fs_context *, int, size_t, int);
static int fs_close1(struct fs_context *c, int h);

/*
 * Acorn OSes implement mandatory locking in OSFIND, delegating that
 * to the fileserver on Econet.  This implementation uses BSD flock()
 * locks to achieve the same effect.  On real BSD systems, we can use
 * O_SHLOCK and O_EXLOCK, but Linux doesn't have these and we have to
 * resort to calling flock() after open().
 *
 * Using flock() causes a problem when creating a new file, becuase
 * another client could get in after the file is created and before we
 * lock it, which shouldn't be able to happen.  This doesn't matter
 * while aund is single-threaded, but once it isn't, the best approach
 * is probably to flock() the containing directory while opening files
 * in it.
 */

#if defined(O_SHLOCK) && defined(O_EXLOCK)
#define HAVE_O_xxLOCK
#endif

void
fs_open(struct fs_context *c)
{
	struct ec_fs_reply_open reply;
	struct ec_fs_req_open *request;
	char *upath;
	int openopt;
	uint8_t h;

	if (c->client == NULL) {
		fs_err(c, EC_FS_E_WHOAREYOU);
		return;
	}
	request = (struct ec_fs_req_open *)(c->req);
	request->path[strcspn(request->path, "\r")] = '\0';
	if (debug) printf("open [%s/%s, %s]\n",
	    request->must_exist ? "exist":"create",
	    request->read_only ? "read":"rdwr", request->path);
	upath = fs_unixify_path(c, request->path);
	if (upath == NULL) return;
	openopt = 0;
	if (!request->must_exist) openopt |= O_CREAT;
	if (request->read_only) {
		openopt |= O_RDONLY;
#ifdef HAVE_O_xxLOCK
		openopt |= O_SHLOCK | O_NONBLOCK;
#endif
	} else {
		openopt |= O_RDWR;
#ifdef HAVE_O_xxLOCK
		openopt |= O_EXLOCK | O_NONBLOCK;
#endif
	}
	if ((h = fs_open_handle(c->client, upath, openopt, true)) == 0) {
#ifdef HAVE_O_xxLOCK
		if (errno == EAGAIN)
			fs_err(c, EC_FS_E_OPEN);
		else
#endif
			fs_errno(c);
		free(upath);
		return;
	}
	free(upath);
#ifdef HAVE_O_xxLOCK
	if ((openopt = fcntl(c->client->handles[h]->fd, F_GETFL)) == -1 ||
	    fcntl(c->client->handles[h]->fd,
		F_SETFL, openopt & ~O_NONBLOCK) == -1) {
		fs_errno(c);
		fs_close_handle(c->client, h);
		return;
	}
#else
	if (flock(c->client->handles[h]->fd,
		(request->read_only ? LOCK_SH : LOCK_EX) | LOCK_NB) == -1) {
		if (errno == EAGAIN)
			fs_err(c, EC_FS_E_OPEN);
		else
			fs_errno(c);
		fs_close_handle(c->client, h);
		return;
	}
#endif
	reply.std_tx.command_code = EC_FS_CC_DONE;
	reply.std_tx.return_code = EC_FS_RC_OK;
	reply.handle = h;
	fs_reply(c, &(reply.std_tx), sizeof(reply));
}

void
fs_close(struct fs_context *c)
{
	struct ec_fs_reply reply;
	struct ec_fs_req_close *request;
	int h, error, thiserr;
	
	if (c->client == NULL) {
		fs_err(c, EC_FS_E_WHOAREYOU);
		return;
	}
	request = (struct ec_fs_req_close *)(c->req);
	if (debug) printf("close [%d]\n", request->handle);
	if (request->handle == 0) {
		error = 0;
		for (h = 1; h < c->client->nhandles; h++)
			if (c->client->handles[h] &&
			    c->client->handles[h]->type == FS_HANDLE_FILE &&
			    (thiserr = fs_close1(c, h)))
				error = thiserr;
	} else
		error = fs_close1(c, request->handle);
	if (error)
		fs_errno(c);
        else {
		reply.command_code = EC_FS_CC_DONE;
		reply.return_code = EC_FS_RC_OK;
		fs_reply(c, &reply, sizeof(reply));
	}
}

/*
 * Close a single handle.
 */
static int
fs_close1(struct fs_context *c, int h)
{
	struct fs_handle *hp;
	int error = 0;

	if ((h = fs_check_handle(c->client, h)) != 0) {
		hp = c->client->handles[h];
		/* ESUG says this is needed */
		if (hp->type == FS_HANDLE_FILE && fsync(hp->fd) == -1) {
			if (errno != EINVAL) /* fundamentally unfsyncable */
				error = errno;
		}
		close(hp->fd);
		fs_close_handle(c->client, h);
	}
	return error;
}

void
fs_get_args(struct fs_context *c)
{
	struct stat st;
	struct ec_fs_reply_get_args reply;
	struct ec_fs_req_get_args *request;
	off_t ptr;
	int h, fd;

	if (c->client == NULL) {
		fs_err(c, EC_FS_E_WHOAREYOU);
		return;
	}
	request = (struct ec_fs_req_get_args *)(c->req);
	if (debug) printf("get args [%d, %d]", request->handle, request->arg);
	if ((h = fs_check_handle(c->client, request->handle)) != 0) {
		fd = c->client->handles[h]->fd;
		switch (request->arg) {
		case EC_FS_ARG_PTR:
			if ((ptr = lseek(fd, 0, SEEK_CUR)) == -1) {
				fs_errno(c);
				return;
			}
			fs_write_val(reply.val, ptr, sizeof(reply.val));
			break;
		case EC_FS_ARG_EXT:
			if (fstat(fd, &st) == -1) {
				fs_errno(c);
				return;
			}
			fs_write_val(reply.val, st.st_size, sizeof(reply.val));
			break;
		case EC_FS_ARG_SIZE:
			if (fstat(fd, &st) == -1) {
				fs_errno(c);
				return;
			}
			fs_write_val(reply.val, st.st_blocks * S_BLKSIZE,
			    sizeof(reply.val));
			break;
		default:
			if (debug) printf("\n");
			fs_err(c, EC_FS_E_BADARGS);
			return;
		}
		if (debug)
			printf(" <- %ju\n",
			    fs_read_val(reply.val, sizeof(reply.val)));
		reply.std_tx.command_code = EC_FS_CC_DONE;
		reply.std_tx.return_code = EC_FS_RC_OK;
		fs_reply(c, &(reply.std_tx), sizeof(reply));
	} else {
		fs_err(c, EC_FS_E_CHANNEL);
	}
}

void
fs_set_args(struct fs_context *c)
{
	struct ec_fs_reply reply;
	struct ec_fs_req_set_args *request;
	off_t val;
	int h, fd;

	if (c->client == NULL) {
		fs_err(c, EC_FS_E_WHOAREYOU);
		return;
	}
	request = (struct ec_fs_req_set_args *)(c->req);
	val = fs_read_val(request->val, sizeof(request->val));
	if (debug)
		printf("set args [%d, %d := %ju]\n",
		    request->handle, request->arg, (uintmax_t)val);
	if ((h = fs_check_handle(c->client, request->handle)) != 0) {
		fd = c->client->handles[h]->fd;
		switch (request->arg) {
		case EC_FS_ARG_PTR:
			if (lseek(fd, val, SEEK_SET) == -1) {
				fs_errno(c);
				return;
			}
			break;
		case EC_FS_ARG_EXT:
			if (ftruncate(fd, val) == -1) {
				fs_errno(c);
				return;
			}
			break;
		default:
			fs_error(c, 0xff, "bad argument to set_args");
			return;
		}
		reply.command_code = EC_FS_CC_DONE;
		reply.return_code = EC_FS_RC_OK;
		fs_reply(c, &reply, sizeof(reply));
	} else
		fs_err(c, EC_FS_E_CHANNEL);
}

static int
fs_randomio_common(struct fs_context *c, int h)
{
	off_t off;
	int fd;

	fd = c->client->handles[h]->fd;
	if (debug) printf("%c", (c->req->aun.flag & 1) ? '/' : '\\');
	if (c->client->handles[h]->sequence != (c->req->aun.flag & 1)) {
		/*
		 * Different sequence number from last request.  Save
		 * our current offset.
		 */
		if ((off = lseek(fd, 0, SEEK_CUR)) == -1) {
			fs_errno(c);
			return -1;
		}
		c->client->handles[h]->oldoffset = off;
		c->client->handles[h]->sequence = (c->req->aun.flag & 1);
	} else {
		/* This is a repeated request. */
		if (debug) printf("<repeat>");
		off = c->client->handles[h]->oldoffset;
		if (lseek(fd, off, SEEK_SET) == -1) {
			fs_errno(c);
			return -1;
		}
	}
	return 0;
}

void
fs_putbyte(struct fs_context *c)
{
	struct ec_fs_reply reply;
	struct ec_fs_req_putbyte *request;
	int h, fd;

	if (c->client == NULL) {
		fs_err(c, EC_FS_E_WHOAREYOU);
		return;
	}
	request = (struct ec_fs_req_putbyte *)(c->req);
	if (debug)
		printf("putbyte [%d, 0x%02x]\n",
		    request->handle, request->byte);
	if ((h = fs_check_handle(c->client, request->handle)) != 0) {
		if (fs_randomio_common(c, request->handle)) return;
		fd = c->client->handles[h]->fd;
		if (write(fd, &request->byte, 1) < 0) {
			fs_errno(c);
			return;
		}
		reply.command_code = EC_FS_CC_DONE;
		reply.return_code = EC_FS_RC_OK;
		fs_reply(c, &reply, sizeof(reply));
	}
	
}

static int
at_eof(int fd)
{
	struct stat st;
	off_t off = lseek(fd, 0, SEEK_CUR);
	return (off != (off_t)-1 && fstat(fd, &st) >= 0 && off >= st.st_size);
}

void
fs_get_eof(struct fs_context *c)
{
	struct ec_fs_reply_get_eof reply;
	struct ec_fs_req_get_eof *request;
	int h, fd;

	if (c->client == NULL) {
		fs_err(c, EC_FS_E_WHOAREYOU);
		return;
	}
	request = (struct ec_fs_req_get_eof *)(c->req);
	if (debug) printf("get eof [%d]\n", request->handle);
	if ((h = fs_check_handle(c->client, request->handle)) != 0) {
		fd = c->client->handles[h]->fd;
		reply.status = at_eof(fd) ? 0xFF : 0;
		reply.std_tx.command_code = EC_FS_CC_DONE;
		reply.std_tx.return_code = EC_FS_RC_OK;
		fs_reply(c, &(reply.std_tx), sizeof(reply));
	}
}

void
fs_getbytes(struct fs_context *c)
{
	struct ec_fs_reply reply1;
	struct ec_fs_reply_getbytes2 reply2;
	struct ec_fs_req_getbytes *request;
	int h, fd;
	off_t off;
	size_t size, got;

	if (c->client == NULL) {
		fs_err(c, EC_FS_E_WHOAREYOU);
		return;
	}
	request = (struct ec_fs_req_getbytes *)(c->req);
	size = fs_read_val(request->nbytes, sizeof(request->nbytes));
	off = fs_read_val(request->offset, sizeof(request->offset));
	if (debug)
		printf("getbytes [%d, %zu%s%ju]\n",
		    request->handle, size, request->use_ptr ? "!" : "@",
		    (uintmax_t)off);
	if ((h = fs_check_handle(c->client, request->handle)) != 0) {
		if (fs_randomio_common(c, request->handle)) return;
		fd = c->client->handles[h]->fd;
		if (!request->use_ptr)
			if (lseek(fd, off, SEEK_SET) == -1) {
				fs_errno(c);
				return;
			}
		reply1.command_code = EC_FS_CC_DONE;
		reply1.return_code = EC_FS_RC_OK;
		fs_reply(c, &reply1, sizeof(reply1));
		reply2.std_tx.command_code = EC_FS_CC_DONE;
		reply2.std_tx.return_code = EC_FS_RC_OK;
		got = fs_data_send(c, fd, size);
		if (got == -1) {
			/* Error */
			fs_errno(c);
		} else {
			if (got == size && !at_eof(fd))
				reply2.flag = 0;
			else
				reply2.flag = 0x80; /* EOF reached */
			fs_write_val(reply2.nbytes, got, sizeof(reply2.nbytes));
			fs_reply(c, &(reply2.std_tx), sizeof(reply2));
		}
	}
	
}

void
fs_getbyte(struct fs_context *c)
{
	struct ec_fs_reply_getbyte reply;
	struct ec_fs_req_getbyte *request;
	int h, fd, ret;

	if (c->client == NULL) {
		fs_err(c, EC_FS_E_WHOAREYOU);
		return;
	}
	request = (struct ec_fs_req_getbyte *)(c->req);
	if (debug) printf("getbyte [%d]\n", request->handle);
	if ((h = fs_check_handle(c->client, request->handle)) != 0) {
		if (fs_randomio_common(c, request->handle)) return;
		fd = c->client->handles[h]->fd;
		if ((ret = read(fd, &reply.byte, 1)) < 0) {
			fs_errno(c);
			return;
		}
		reply.std_tx.command_code = EC_FS_CC_DONE;
		reply.std_tx.return_code = EC_FS_RC_OK;
		if (ret == 0) {
			reply.flag = 0xC0;
			reply.byte = 0xFF;
		} else {
			reply.flag = at_eof(fd) ? 0x80 : 0;
		}
		fs_reply(c, &(reply.std_tx), sizeof(reply));
	}
}

void
fs_putbytes(struct fs_context *c)
{
	struct ec_fs_reply_putbytes1 reply1;
	struct ec_fs_reply_putbytes2 reply2;
	struct ec_fs_req_putbytes *request;
	int h, fd, replyport;
	off_t off;
	size_t size, got;

	if (c->client == NULL) {
		fs_err(c, EC_FS_E_WHOAREYOU);
		return;
	}
	replyport = c->req->reply_port;
	request = (struct ec_fs_req_putbytes *)(c->req);
	size = fs_read_val(request->nbytes, sizeof(request->nbytes));
	off = fs_read_val(request->offset, sizeof(request->offset));
	if (debug)
		printf("putbytes [%d, %zu%s%ju]\n",
		    request->handle, size, request->use_ptr ? "!" : "@",
		    (uintmax_t)off);
	if ((h = fs_check_handle(c->client, request->handle)) != 0) {
		if (fs_randomio_common(c, request->handle)) return;
		fd = c->client->handles[h]->fd;
		if (!request->use_ptr)
			if (lseek(fd, off, SEEK_SET) == -1) {
				fs_errno(c);
				return;
			}
		reply1.std_tx.command_code = EC_FS_CC_DONE;
		reply1.std_tx.return_code = EC_FS_RC_OK;
		reply1.data_port = OUR_DATA_PORT;
	        fs_write_val(reply1.block_size, aunfuncs->max_block,
			     sizeof(reply1.block_size));
		fs_reply(c, &(reply1.std_tx), sizeof(reply1));
		reply2.std_tx.command_code = EC_FS_CC_DONE;
		reply2.std_tx.return_code = EC_FS_RC_OK;
		got = fs_data_recv(c, fd, size, c->req->urd);
		if (got == -1) {
			/* Error */
			fs_errno(c);
		} else {
			reply2.zero = 0;
			fs_write_val(reply2.nbytes, got, sizeof(reply2.nbytes));
			c->req->reply_port = replyport;
			fs_reply(c, &(reply2.std_tx), sizeof(reply2));
		}
	}
}

void
fs_load(struct fs_context *c)
{
	struct ec_fs_reply_load1 reply1;
	struct ec_fs_reply_load2 reply2;
	struct ec_fs_req_load *request;
	char *upath, *upathlib, *path_argv[3];
	int fd, as_command;
	size_t got;
	FTS *ftsp;
	FTSENT *f;

	if (c->client == NULL) {
		fs_err(c, EC_FS_E_WHOAREYOU);
		return;
	}
	request = (struct ec_fs_req_load *)(c->req);
	request->path[strcspn(request->path, "\r")] = '\0';
	as_command = c->req->function == EC_FS_FUNC_LOAD_COMMAND;
	if (debug) printf("load%s [%s]\n",
	     as_command ? " as command" : "", request->path);
	/*
	 * 8-bit clients tend to send the whole command line for "load
	 * as command", so we trim it for them.
	 */
	request->path[strcspn(request->path, " ")] = '\0';
	upath = fs_unixify_path(c, request->path);
	if (upath == NULL) return;
	path_argv[0] = upath;
	path_argv[1] = NULL;
	if (as_command) {
		c->req->csd = c->req->lib;
		upathlib = fs_unixify_path(c, request->path);
		if (upathlib == NULL) {
			free(upath);
			return;
		}
		path_argv[1] = upathlib;
		path_argv[2] = NULL;
	}
	ftsp = fts_open(path_argv, FTS_LOGICAL, NULL);
	f = fts_read(ftsp);
	if (as_command && f->fts_info == FTS_NS && f->fts_errno == ENOENT)
		f = fts_read(ftsp);
	if (f->fts_info == FTS_ERR || f->fts_info == FTS_NS) {
		fs_errno(c);
		goto out;
	}
	if (S_ISDIR(f->fts_statp->st_mode)) {
		fs_err(c, EC_FS_E_ISDIR);
		goto out;
	}
	if ((fd = open(f->fts_accpath, O_RDONLY)) == -1) {
		fs_errno(c);
		goto out;
	}
	fs_get_meta(f, &(reply1.meta));
	fs_write_val(reply1.size, f->fts_statp->st_size, sizeof(reply1.size));
	reply1.access = fs_mode_to_access(f->fts_statp->st_mode);
	fs_write_date(&(reply1.date), fs_get_birthtime(f));
	reply1.std_tx.command_code = EC_FS_CC_DONE;
	reply1.std_tx.return_code = EC_FS_RC_OK;
	fs_reply(c, &(reply1.std_tx), sizeof(reply1));
	reply2.std_tx.command_code = EC_FS_CC_DONE;
	reply2.std_tx.return_code = EC_FS_RC_OK;
	got = fs_data_send(c, fd, f->fts_statp->st_size);
	if (got == -1) {
		/* Error */
		fs_errno(c);
	} else {
		fs_reply(c, &(reply2.std_tx), sizeof(reply2));
	}
	close(fd);
out:
	fts_close(ftsp);
	free(upath);
	if (as_command) free(upathlib);
}

void
fs_save(struct fs_context *c)
{
	struct ec_fs_reply_save1 reply1;
	struct ec_fs_reply_save2 reply2;
	struct ec_fs_req_save *request;
	struct ec_fs_meta meta;
	char *upath, *path_argv[2];
	int fd, ackport, replyport;
	size_t size, got;
	FTS *ftsp;
	FTSENT *f;

	if (c->client == NULL) {
		fs_err(c, EC_FS_E_WHOAREYOU);
		return;
	}
	request = (struct ec_fs_req_save *)(c->req);
	request->path[strcspn(request->path, "\r")] = '\0';
	replyport = c->req->reply_port;
	ackport = c->req->urd;
	if (debug) printf("save [%s]\n", request->path);
	size = fs_read_val(request->size, sizeof(request->size));
	upath = fs_unixify_path(c, request->path);
	if (upath == NULL) return;
	if ((fd = open(upath, O_CREAT|O_TRUNC|O_RDWR, 0666)) == -1) {
		fs_errno(c);
		free(upath);
		return;
	}
	meta = request->meta;
	reply1.std_tx.command_code = EC_FS_CC_DONE;
	reply1.std_tx.return_code = EC_FS_RC_OK;
	reply1.data_port = OUR_DATA_PORT;
	fs_write_val(reply1.block_size, aunfuncs->max_block,
		     sizeof(reply1.block_size));
	fs_reply(c, &(reply1.std_tx), sizeof(reply1));
	reply2.std_tx.command_code = EC_FS_CC_DONE;
	reply2.std_tx.return_code = EC_FS_RC_OK;
	got = fs_data_recv(c, fd, size, ackport);
	close(fd);
	if (got == -1) {
		/* Error */
		fs_errno(c);
	} else {
		/*
		 * Write load and execute addresses from the
		 * request, and return the file date in the
		 * response.
		 */
		path_argv[0] = upath;
		path_argv[1] = NULL;
		ftsp = fts_open(path_argv, FTS_LOGICAL, NULL);
		f = fts_read(ftsp);
		fs_set_meta(f, &meta);
		fs_write_date(&(reply2.date), fs_get_birthtime(f));
		reply2.access = fs_mode_to_access(f->fts_statp->st_mode);
		fts_close(ftsp);
		c->req->reply_port = replyport;
		fs_reply(c, &(reply2.std_tx), sizeof(reply2));
	}
	free(upath);
}

void
fs_create(struct fs_context *c)
{
	struct ec_fs_reply_create reply;
	struct ec_fs_req_create *request;
	struct ec_fs_meta meta;
	char *upath, *path_argv[2];
	int fd, replyport;
	size_t size;
	FTS *ftsp;
	FTSENT *f;

	if (c->client == NULL) {
		fs_err(c, EC_FS_E_WHOAREYOU);
		return;
	}
	request = (struct ec_fs_req_create *)(c->req);
	request->path[strcspn(request->path, "\r")] = '\0';
	replyport = c->req->reply_port;
	if (debug) printf("create [%s]\n", request->path);
	size = fs_read_val(request->size, sizeof(request->size));
	upath = fs_unixify_path(c, request->path);
	if (upath == NULL) return;
	if ((fd = open(upath, O_CREAT|O_TRUNC|O_RDWR, 0666)) == -1) {
		fs_errno(c);
		free(upath);
		return;
	}
	if (ftruncate(fd, size) != 0) {
		fs_errno(c);
		close(fd);
		free(upath);
		return;
	}
	meta = request->meta;
	reply.std_tx.command_code = EC_FS_CC_DONE;
	reply.std_tx.return_code = EC_FS_RC_OK;
	close(fd);
	/*
	 * Write load and execute addresses from the
	 * request, and return the file date in the
	 * response.
	 */
	path_argv[0] = upath;
	path_argv[1] = NULL;
	ftsp = fts_open(path_argv, FTS_LOGICAL, NULL);
	f = fts_read(ftsp);
	fs_set_meta(f, &meta);
	fs_write_date(&(reply.date), fs_get_birthtime(f));
	reply.access = fs_mode_to_access(f->fts_statp->st_mode);
	fts_close(ftsp);
	free(upath);
	c->req->reply_port = replyport;
	fs_reply(c, &(reply.std_tx), sizeof(reply));
}

static ssize_t
fs_data_send(struct fs_context *c, int fd, size_t size)
{
	struct aun_packet *pkt;
	void *buf;
	ssize_t result;
	size_t this, done;
	int faking;

	if ((pkt = malloc(sizeof(*pkt) +
	    (size > aunfuncs->max_block ? aunfuncs->max_block : size))) ==
	    NULL) { 
		fs_err(c, EC_FS_E_NOMEM);
		return -1;
	}
	buf = pkt->data;
	faking = 0;
	done = 0;
	while (size) {
		this = size > aunfuncs->max_block ? aunfuncs->max_block : size;
		if (!faking) {
			result = read(fd, buf, this);
			if (result > 0) {
				/* Normal -- the kernel had something for us */
				this = result;
				done += this;
			} else { /* EOF or error */
				if (result == -1) done = result;
				faking = 1;
			}
		}
		pkt->type = AUN_TYPE_UNICAST;
		pkt->dest_port = c->req->urd;
		pkt->flag = c->req->aun.flag & 1;
		if (aunfuncs->xmit(pkt, sizeof(*pkt) + this, c->from) == -1)
			warn("send data");
		size -= this;
	}
	free(pkt);
	return done;
}

static ssize_t
fs_data_recv(struct fs_context *c, int fd, size_t size, int ackport)
{
	struct aun_packet *pkt, *ack;
	ssize_t msgsize, result;
	struct aun_srcaddr from;
	size_t done;

	if ((ack = malloc(sizeof(*ack) + 1)) == NULL) {
		fs_err(c, EC_FS_E_NOMEM);
		return -1;
	}
	done = 0;
	while (size) {
		from = *c->from;
		pkt = aunfuncs->recv(&msgsize, &from, OUR_DATA_PORT);
		if (!pkt) {
			warn("receive data");
			return -1;     /* no reply: client has gone away */
		}
		msgsize -= sizeof(struct aun_packet);
		if (pkt->dest_port != OUR_DATA_PORT ||
		    memcmp(&from, c->from, sizeof(from))) {
			fs_error(c, 0xFF, "I'm confused");
			return -1;
		}
		result = write(fd, pkt->data, msgsize);
		if (result < 0) {
			fs_errno(c);
			return -1;
		}
		size -= msgsize;
		if (size) {
			/*
			 * Send partial ACK.
			 */
			ack->type = AUN_TYPE_UNICAST;
			ack->dest_port = ackport;
			ack->flag = 0;
			ack->data[0] = 0;
			if (aunfuncs->xmit(ack, sizeof(*ack) + 1, c->from) ==
			    -1)
				warn("send data");
		}
	}
	free(ack);
	return done;
}
