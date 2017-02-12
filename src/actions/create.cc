#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "create.hh"
#include "utils/optparse.h"
#include "fs.hh"

static void usage();
static uint64_t parse_size(std::string);
static void format(FILE *);
static void write_root_inode(FILE *fp);
static void write_superblock(FILE *fp);

extern std::string prgname;


static void format(FILE *fp)
{
	write_superblock(fp);
	write_root_inode(fp);
}

static void write_root_inode(FILE *fp)
{
	struct timespec ts;
	size_t bytes;
	hush::fs::inode ino;

	clock_gettime(CLOCK_REALTIME, &ts);
	ino = {
		.mode = S_IFDIR,
		.inode_number = 1,
		.block_number = 1,
		.uid = getuid(),
		.gid = getgid(),
		.atime = ts,
		.ctime = ts,
		.mtime = ts,
		.directory_num_children = 1,
	};

	bytes = fwrite(&ino, sizeof ino, 1, fp);

	if (bytes != 1) {
		std::ostringstream s;
		s << "Error writing root inode: Wrote " << bytes << " bytes, expected " << sizeof ino << std::endl;
		std::cerr << s.str();
		throw s.str();
	}

	std::cout << "Wrote root entry" << std::endl;
}

static void write_superblock(FILE *fp)
{
	size_t bytes;
	hush::fs::superblock sb = {
		{
			.version = 1,
			.magic = hush::fs::MAGIC,
			.block_size = hush::fs::BLOCK_SIZE,
			.inodes_count = 0,
		},
	};

	bytes = fwrite(&sb, sizeof sb, 1, fp);

	if (bytes != 1) {
		std::ostringstream s;
		s << "Error writing superblock: Wrote " << bytes << " bytes, expected " << sizeof sb << std::endl;
		std::cerr << s.str();
		throw s.str();
	}
	std::cout << "Wrote superblock" << std::endl;	
}

static void usage()
{
	std::cerr << "Usage " << prgname << " -k .path/to/keyfile -s N[k|g|m] secret.img" << std::endl;
}

static uint64_t parse_size(std::string s)
{
	std::string suffix;
	std::string::size_type end = 0;
	uint64_t n;

	n = std::stoull(s, &end, 10);

	if (end == 0) {
		std::cerr << "Invalid size specified" << std::endl;
		n = 0;
	}

	if (end) {
		if (end < s.length() - 1)
			std::cerr << "Invalid size specified" << std::endl;
		suffix = s.substr(end);
		std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
	}

	if (! suffix.empty()) {
		switch (suffix.at(0)) {
			case 'k':
				n *= 1000;
				break;
			case 'm':
				n *= 1000000;
				break;
			case 'g':
				n *= 1000000000;
				break;
			default:
				std::cerr << "Invalid size modifier. Must be one of k, g, or m"
						  << std::endl;
				n = 0;
		}
	}

	return n;
}

int hush_create(struct optparse *opts)
{
	int opt, ret = 0, fd, nullbyte = 0;
	std::string filename, keypath;
	uint64_t filelen = 0;
	char *tmp;
	FILE *fp;

	while ((opt = optparse(opts, "k:s:h")) != -1) {
		switch (opt) {
			case 'k':
				keypath = opts->optarg;
				break;
			case 's':
				filelen = parse_size(opts->optarg);
				break;
			case 'h':
			default:
				usage();
				ret = 1;
				goto bye;
		}
	}

	if ((tmp = optparse_arg(opts)) != nullptr)
		filename = tmp;

	if (filename.empty() || keypath.empty() || filelen == 0) {
		usage();
		ret = 1;
		goto bye;
	}

	std::cout << "Creating file " << filename << " of " << filelen << 
		" bytes with key " << keypath << std::endl;

	
	if ((fd = open(filename.c_str(), O_CREAT | O_RDWR | O_EXCL, 0600)) == -1) {
		ret = 1;
		std::cerr << "Error opening file " << filename << std::endl;
		goto bye;
	}

	if (flock(fd, LOCK_EX) != 0) {
		ret = 1;
		std::cerr << "Error obtaining exclusive lock on file " << filename << std::endl;
		goto bye;
	}

	if ((fp = fdopen(fd, "w+")) == NULL) {
		ret = 1;
		std::cerr << "Error getting file pointer for file " << filename << std::endl;
		goto bye;
	}

	try {
		format(fp);
	} catch (...) {
		fclose(fp);
		throw;
	}

	// create sparse file
	fseek(fp, filelen-1, SEEK_SET);
	fwrite(&nullbyte, 1, 1, fp);


prebye:
	fclose(fp);

bye:
	return ret;
}
