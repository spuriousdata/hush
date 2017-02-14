#define FUSE_USE_VERSION 26

#include <iostream>
#include <string>

#include <fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "utils/optparse.h"
#include "mount.hh"

extern std::string prgname;

static const char *hello_str = "Hello World!\n";
static const char *hello_name = "hello";

static void usage(void)
{
	std::cout << "Usage: " << prgname << " [opts] /mount/point" << std::endl
	<< "'-f'  foreground" << std::endl
	<< "'-d'  foreground, but keep the debug option " << std::endl
	<< "'-s'  single threaded" << std::endl
	<< "'-h'  help" << std::endl;
}

static int hello_stat(fuse_ino_t ino, struct stat *stbuf)
{
	stbuf->st_ino = ino;
	switch (ino) {
	case 1:
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		break;

	case 2:
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(hello_str);
		break;

	default:
		return -1;
	}
	return 0;
}

static void hush_getattr(fuse_req_t req, fuse_ino_t ino,
							 struct fuse_file_info *fi)
{
	struct stat stbuf;

	(void) fi;

	memset(&stbuf, 0, sizeof(stbuf));
	if (hello_stat(ino, &stbuf) == -1)
		fuse_reply_err(req, ENOENT);
	else
		fuse_reply_attr(req, &stbuf, 1.0);
}

static void hush_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	struct fuse_entry_param e;

	if (parent != 1 || strcmp(name, hello_name) != 0)
		fuse_reply_err(req, ENOENT);
	else {
		memset(&e, 0, sizeof(e));
		e.ino = 2;
		e.attr_timeout = 1.0;
		e.entry_timeout = 1.0;
		hello_stat(e.ino, &e.attr);

		fuse_reply_entry(req, &e);
	}
}

struct dirbuf {
	char *p;
	size_t size;
};

static void dirbuf_add(fuse_req_t req, struct dirbuf *b, const char *name,
					   fuse_ino_t ino)
{
	struct stat stbuf;
	size_t oldsize = b->size;
	b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
	b->p = (char *) realloc(b->p, b->size);
	memset(&stbuf, 0, sizeof(stbuf));
	stbuf.st_ino = ino;
	fuse_add_direntry(req, b->p + oldsize, b->size - oldsize, name, &stbuf,
					  b->size);
}

#define min(x, y) ((x) < (y) ? (x) : (y))

static int reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize,
							 off_t off, size_t maxsize)
{
	if (off < bufsize)
		return fuse_reply_buf(req, buf + off, min(bufsize - off, maxsize));
	else
		return fuse_reply_buf(req, NULL, 0);
}

static void hush_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
							 off_t off, struct fuse_file_info *fi)
{
	(void) fi;

	if (ino != 1)
		fuse_reply_err(req, ENOTDIR);
	else {
		struct dirbuf b;

		memset(&b, 0, sizeof(b));
		dirbuf_add(req, &b, ".", 1);
		dirbuf_add(req, &b, "..", 1);
		dirbuf_add(req, &b, hello_name, 2);
		reply_buf_limited(req, b.p, b.size, off, size);
		free(b.p);
	}
}

static void hush_open(fuse_req_t req, fuse_ino_t ino,
						 struct fuse_file_info *fi)
{
	if (ino != 2)
		fuse_reply_err(req, EISDIR);
	else if ((fi->flags & 3) != O_RDONLY)
		fuse_reply_err(req, EACCES);
	else
		fuse_reply_open(req, fi);
}

static void hush_read(fuse_req_t req, fuse_ino_t ino, size_t size,
						 off_t off, struct fuse_file_info *fi)
{
	(void) fi;

	assert(ino == 2);
	reply_buf_limited(req, hello_str, strlen(hello_str), off, size);
}

static struct fuse_lowlevel_ops hush_oper = {
	.lookup  = hush_lookup,
	.getattr = hush_getattr,
	.readdir = hush_readdir,
	.open    = hush_open,
	.read    = hush_read,
};

/*
 *   '-f'	        foreground
 *   '-d' '-odebug' foreground, but keep the debug option
 *   '-s'	        single threaded
 *   '-h' '--help'  help
 *   '-ho'	        help without header
 *   '-ofsname=..'  file system name, if not present, then 
 *                  set to the program name
 */
int hush_mount(struct optparse *opts)
{
	cmdopts fuseopts;
	struct fuse_chan *ch;
	char *mountpoint;
	int err = -1, opt;

	while ((opt = optparse(opts, "fdsh")) != -1) {
		switch (opt) {
			case 'f':
				fuseopts.foreground = true;
				break;
			case 'd':
				fuseopts.debug = true;
				fuseopts.foreground = true;
				break;
			case 's':
				fuseopts.single_threaded = true;
				break;
			case 'h':
				usage();
				return 0;
			default:
				usage();
				return 1;
		}
	}

	if ((mountpoint = optparse_arg(opts)) == nullptr) {
		usage();
		return 1;
	}

	if ((ch = fuse_mount(mountpoint, NULL)) != NULL) {
		struct fuse_session *se;
		se = fuse_lowlevel_new(NULL, &hush_oper, sizeof(hush_oper), NULL);
		if (se != NULL) {
			if (fuse_set_signal_handlers(se) != -1) {
				fuse_session_add_chan(se, ch);
				err = fuse_session_loop(se);
				fuse_remove_signal_handlers(se);
				fuse_session_remove_chan(ch);
			}
			fuse_session_destroy(se);
		}
		fuse_unmount(mountpoint, ch);
	}

	return err ? 1 : 0;
}
