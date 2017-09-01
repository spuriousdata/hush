#include <algorithm> // transform, tolower
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
static void format(int, uint64_t);
static void write_root_inode(int);
static void write_superblock(int, uint64_t);

extern std::string prgname;

static void format(int fd, uint64_t filelen)
{
	write_superblock(fd, filelen);
	write_root_inode(fd);
}

static void write_root_inode(int fd)
{
	struct timespec ts;
	size_t bytes;
	hush::fs::inode_t inode;

	clock_gettime(CLOCK_REALTIME, &ts);
	inode = {
		.mode = S_IFDIR,
		.inode_number = 1, // root directory inode is 1
		.block_number = 1, // root datablock is also 1
		.uid = getuid(),
		.gid = getgid(),
		.ctime = ts,
		.mtime = ts,
		.dir_children = 0,
	};

	bytes = write(fd, &inode, sizeof inode);

	if (bytes != sizeof inode) {
		std::ostringstream s;
		s << "Error writing root inode: Wrote " << bytes << " bytes, expected " << sizeof inode << std::endl;
		std::cerr << s.str();
		throw s.str();
	}

	std::cout << "Wrote root inode, size: " << sizeof(inode) << std::endl;
}

static void write_superblock(int fd, uint64_t filelen)
{
	size_t bytes;
	uint64_t num_inodes = (uint64_t)(filelen / sizeof(hush::fs::inode_t));
	uint64_t num_blocks = (uint64_t)(filelen / hush::fs::BLOCK_SIZE);

	hush::fs::superblock_t sb = {
		.fields = {
			.magic = hush::fs::MAGIC,
			.version = HUSHFS_VERSION,
			.block_size = hush::fs::BLOCK_SIZE,
			.disk_size = filelen,
			.total_inodes = num_inodes,
			.total_blocks = num_blocks,
			.inode_bitmap_blocks = (uint64_t)(num_inodes / 8),
			.block_bitmap_blocks = (uint64_t)(num_blocks / 8),
		}
	};

	bytes = write(fd, &sb, sizeof sb);

	if (bytes != sizeof sb) {
		std::ostringstream s;
		s << "Error writing superblock: Wrote " << bytes << " bytes, expected " << sizeof sb << std::endl;
		std::cerr << s.str();
		throw s.str();
	}
	std::cout << "Wrote superblock, size: " << sizeof(sb) << std::endl;	
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
				n *= KB;
				break;
			case 'm':
				n *= MB;
				break;
			case 'g':
				n *= GB;
				break;
			default:
				std::cerr << "Invalid size modifier. If present, must be one of k, g, or m"
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
	bool no_sparse = false;
	off_t curpos;

	while ((opt = optparse(opts, "Sk:s:h")) != -1) {
		switch (opt) {
			case 'S':
				no_sparse = true;
				break;
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

	try {
		format(fd, filelen);
	} catch (...) {
		close(fd);
		throw;
	}

	if (no_sparse) {
		tmp = new char[1024];
		curpos = lseek(fd, 0, SEEK_CUR);
		while ((curpos = lseek(fd, 0, SEEK_CUR)) < filelen) {
			write(fd, tmp, filelen-curpos);
		}
		delete[] tmp;
	} else {
		// create sparse file
		lseek(fd, filelen-1, SEEK_SET);
		write(fd, &nullbyte, 1);
	}


prebye:
	close(fd);

bye:
	return ret;
}
