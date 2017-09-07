#include <algorithm> // transform, tolower
#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "create.hh"
#include "utils/optparse.h"
#include "utils/log.hh"
#include "utils/tools.hh"
#include "fs.hh"

using slog::LogString;

static void usage();
static uint64_t parse_size(std::string);
static void format(int, uint64_t);
static hush::fs::superblock_t * write_superblock(int, uint64_t);
static void write_root_inode(int, hush::fs::superblock_t *);
static void write_inode_bitmap(int, hush::fs::superblock_t *);
static void write_block_bitmap(int, hush::fs::superblock_t *);

static slog::Log logger(slog::DEBUG);

extern std::string prgname;

inline uint64_t MAX(uint64_t a, uint64_t b)
{
	return (a > b) ? a : b;
}

static void format(int fd, uint64_t filelen)
{
	hush::fs::superblock_t *sb = write_superblock(fd, filelen);
	write_inode_bitmap(fd, sb);
	write_block_bitmap(fd, sb);
	write_root_inode(fd, sb);
}

static hush::fs::superblock_t * write_superblock(int fd, uint64_t filelen)
{
	size_t bytes;
	uint64_t num_blocks = (uint64_t)(filelen / hush::fs::BLOCK_SIZE);
	uint64_t num_inodes = num_blocks;
	uint64_t ibb = MAX(1, (uint64_t)((num_inodes / 8) / hush::fs::BLOCK_SIZE));
	uint64_t bbb = MAX(1, (uint64_t)((num_blocks / 8) / hush::fs::BLOCK_SIZE));
	uint64_t inode_table_blocks = (uint64_t)(num_inodes / hush::fs::BLOCK_SIZE) + 1;
	uint64_t start_bitmap_block = 1;
	hush::fs::superblock_t *sb = new hush::fs::superblock_t;

	*sb = {
		.fields = {
			.version             = HUSHFS_VERSION,
			.block_size          = hush::fs::BLOCK_SIZE,
			.disk_size           = filelen,
			.total_inodes        = num_inodes,
			.total_blocks        = num_blocks,
			.inode_bitmap_blocks = ibb,
			.block_bitmap_blocks = bbb,
			.inode_bitmap_offset = start_bitmap_block,
			.block_bitmap_offset = start_bitmap_block + ibb,
			.inode_table_offset  = start_bitmap_block + ibb + bbb,
			.first_datablock     = start_bitmap_block + ibb + bbb + inode_table_blocks,
		}
	};

	memcpy(&sb->fields.magic, hush::fs::MAGIC, 4);

	logger.debug("Creating superblock\n"
			"sb = {\n"
			"\t.fields = {\n"
			"\t\t.magic               = \"HusH\"\n"
			"\t\t.version             = %1\n"
			"\t\t.block_size          = %2\n"
			"\t\t.disk_size           = %3\n"
			"\t\t.total_inodes        = %4\n"
			"\t\t.total_blocks        = %5\n"
			"\t\t.inode_bitmap_blocks = %6\n"
			"\t\t.block_bitmap_blocks = %7\n"
			"\t\t.inode_bitmap_offset = %8\n"
			"\t\t.block_bitmap_offset = %9\n"
			"\t\t.inode_table_offset  = %10\n"
			"\t\t.first_datablock     = %11\n"
			"\t}\n"
			"}", 
			HUSHFS_VERSION,
			hush::fs::BLOCK_SIZE,
			filelen,
			num_inodes,
			num_blocks,
			ibb,
			bbb,
			start_bitmap_block,
			start_bitmap_block + ibb,
			start_bitmap_block + ibb + bbb,
			start_bitmap_block + ibb + bbb + inode_table_blocks
	);

	bytes = write(fd, sb, sizeof(hush::fs::superblock_t));

	if (bytes != sizeof(hush::fs::superblock_t)) {
		LogString ls("Error writing superblock: Wrote %1 bytes, expected %2", bytes, sizeof sb);
		logger.critical(ls.string());
		throw ls.string();
	}

	logger.info("Wrote superblock, size: %1", sizeof(sb));
	return sb;
}

static void write_inode_bitmap(int fd, hush::fs::superblock_t *sb)
{
	uint8_t *map;
	uint64_t j = 0, size = sb->fields.inode_bitmap_blocks * hush::fs::BLOCK_SIZE;

	map = new uint8_t[size];

	uint8_t byte = map[j];
	for (uint64_t i = 0; i < sb->fields.first_datablock; i++) {
		if (i != 0 && (i % 8) == 0) {
			j++;
			byte = map[j];
		}
		byte = byte | (0x80 >> i);
	}

	write(fd, map, size);
	logger.info("Wrote inode bitmap, size: %1", size);
}

static void write_block_bitmap(int fd, hush::fs::superblock_t *sb)
{
	uint8_t *map;
	uint64_t size = sb->fields.block_bitmap_blocks * hush::fs::BLOCK_SIZE;

	map = new uint8_t[size];

	// no inodes are used right now, so we write an empty map
	write(fd, map, size); 
	logger.info("Wrote block bitmap, size: %1", size);
}

static void write_root_inode(int fd, hush::fs::superblock_t *sb)
{
	struct timespec ts;
	size_t bytes;
	hush::fs::inode_t inode;

	clock_gettime(CLOCK_REALTIME, &ts);
	inode = {
		.fields = {
			.mode         = S_IFDIR,
			.inode_number = 1, // root directory inode is 1
			.block_number = 1, // root datablock is also 1
			.uid          = getuid(), // these should shadow your local user/group
			.gid          = getgid(),
			.ctime        = ts,
			.mtime        = ts,
			.dir_children = 0,
		},
	};

	bytes = write(fd, &inode, sizeof inode);

	if (bytes != sizeof inode) {
		LogString ls("Error writing root inode: Wrote %1 bytes, expected %2 ", bytes, sizeof inode);
		logger.critical(ls);
		throw ls.string();
	}

	logger.info("Wrote root inode, size: %1", sizeof(inode));
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
