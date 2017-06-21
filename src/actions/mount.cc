#define FUSE_USE_VERSION 26

#include <iostream>
#include <string>
#include <vector>
#include <cstring> // strdup
#include <algorithm> // transform
#include <iterator> // back_inserter

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
static bool __debug = false;

static void usage(void)
{
	std::cout << "Usage: " << prgname << " [opts] /home/user/hush.img "
	<< "/mount/point" << std::endl << "'-h'  help" << std::endl;
}

static int hush_stat(fuse_ino_t ino, struct stat *stbuf)
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
	if (__debug)
		std::cerr << "hush_getattr(req=req, ino=" << ino << ", fi=" << fi << ")" << std::endl;

	memset(&stbuf, 0, sizeof(stbuf));
	if (hush_stat(ino, &stbuf) == -1)
		fuse_reply_err(req, ENOENT);
	else
		fuse_reply_attr(req, &stbuf, 1.0);
}

static void hush_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	struct fuse_entry_param e;

	if (__debug)
		std::cerr << "hush_lookup(x, parent=" << parent << ", name=" << name << ")" << std::endl;

	if (parent != 1 || strcmp(name, hello_name) != 0)
		fuse_reply_err(req, ENOENT);
	else {
		memset(&e, 0, sizeof(e));
		e.ino = 2;
		e.attr_timeout = 1.0;
		e.entry_timeout = 1.0;
		hush_stat(e.ino, &e.attr);

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
	if (__debug)
		std::cerr << "hush_readdir(req=x, ino=" << ino << ", size=" << size << ", off=" << off << ", fi=" << fi << ")" << std::endl;

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

int hush_mount(int main_argc, struct optparse *opts)
{
	struct fuse_chan *ch;
	char *mountpoint, *tmp;
	int err = -1, opt;
	struct fuse_args args;
	std::string disk_image;
	std::vector<std::string> args_in;
	std::vector<char*> args_out;

	while ((opt = optparse(opts, "hd")) != -1) {
		switch (opt) {
			case 'h':
				usage();
				return 0;
			case 'd':
				__debug = true;
				break;
			default:
				usage();
				return 1;
		}
	}

	if ((tmp = optparse_arg(opts)) == 0) {
		usage();
		return 1;
	}

	disk_image = tmp;

	/*
	 * I really hate doing this, but fuse REALLY wants to parse the cmdline
	 * args and prior to 3.0, which isn't installed or available most
	 * anywhere, it actually stores information in argv.
	 */
	args_in.push_back("hushfs");
	for (int i = opts->optind; i < main_argc; i++)
		args_in.push_back(opts->argv[i]);

	std::transform(args_in.begin(),
				   args_in.end(),
				   std::back_inserter(args_out),
				   [] (const std::string &s) -> char * {
					return strdup(s.c_str());
				   });

	args = FUSE_ARGS_INIT(static_cast<int>(args_out.size()), args_out.data());
	/* End creation of fake argc/argv for fuse */

	if (fuse_parse_cmdline(&args, &mountpoint, NULL, NULL) != -1 && 
			(ch = fuse_mount(mountpoint, &args)) != NULL) {
		struct fuse_session *se;
		se = fuse_lowlevel_new(&args, &hush_oper, sizeof(hush_oper), NULL);
		if (se != NULL) {
			if (fuse_set_signal_handlers(se) != -1) {
				fuse_session_add_chan(se, ch);

				// main loop ?
				err = fuse_session_loop(se);

				fuse_remove_signal_handlers(se);
				fuse_session_remove_chan(ch);
			}
			fuse_session_destroy(se);
		}
		fuse_unmount(mountpoint, ch);
	}

	// clean up the mess we've made
	for (auto it = args_out.begin(); it != args_out.end(); it++)
		free(*it);

	return err ? 1 : 0;
}
