#include <algorithm> // transform, tolower
#include <iostream>
#include <memory>
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

using LogString = slog::LogString;
using Superblock = hush::fs::Superblock;

static void usage();
static uint64_t parse_size(std::string);
static void format(int, uint64_t);
static std::shared_ptr<Superblock> write_superblock(int, uint64_t);
static void write_root_inode(int, std::shared_ptr<Superblock> const &);
static void write_inode_bitmap(int, std::shared_ptr<Superblock> const &);
static void write_block_bitmap(int, std::shared_ptr<Superblock> const &);
static void write_inode_table(int, std::shared_ptr<Superblock> const &);

static slog::Log logger(slog::LogLevel::DEBUG);

extern std::string prgname;

inline uint64_t MAX(uint64_t a, uint64_t b)
{
	return (a > b) ? a : b;
}

static void format(int fd, uint64_t filelen)
{
	std::shared_ptr<Superblock> sb = write_superblock(fd, filelen);
	write_inode_bitmap(fd, sb);
	write_block_bitmap(fd, sb);
	write_inode_table(fd, sb);
	//write_root_inode(fd, sb);

	sb.reset();
}

static std::shared_ptr<Superblock> write_superblock(int fd, uint64_t filelen)
{
	uint64_t num_blocks = (uint64_t)(filelen / HUSHFS_BLOCK_SIZE);
	uint64_t num_inodes = num_blocks;
	uint64_t inodes_per_block = (uint64_t)(HUSHFS_BLOCK_SIZE / sizeof(hush::fs::Inode));
	uint64_t ibb = MAX(1, (uint64_t)((num_inodes / 8) / HUSHFS_BLOCK_SIZE));
	uint64_t bbb = MAX(1, (uint64_t)((num_blocks / 8) / HUSHFS_BLOCK_SIZE));
	uint64_t inode_table_blocks = (uint64_t)(num_inodes / inodes_per_block) + 1;
	uint64_t start_bitmap_block = 1;
	auto sb = std::make_shared<Superblock>();

	*sb = {
		.fields = {
			.version             = HUSHFS_VERSION,
			.block_size          = HUSHFS_BLOCK_SIZE,
			.disk_size           = filelen,
			.total_inodes        = num_inodes,
			.total_blocks        = num_blocks,
			.inode_bitmap_blocks = ibb,
			.block_bitmap_blocks = bbb,
			.inode_table_blocks  = inode_table_blocks,
			.inodes_per_block    = inodes_per_block,
			.inode_bitmap_offset = start_bitmap_block,
			.block_bitmap_offset = start_bitmap_block + ibb,
			.inode_table_offset  = start_bitmap_block + ibb + bbb,
			.first_datablock     = start_bitmap_block + ibb + bbb + inode_table_blocks,
		}
	};

	memcpy(&sb->fields.magic, HUSHFS_MAGIC, 4);

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
			"\t\t.inode_table_blocks  = %8\n"
			"\t\t.inodes_per_block    = %9\n"
			"\t\t.inode_bitmap_offset = %10\n"
			"\t\t.block_bitmap_offset = %11\n"
			"\t\t.inode_table_offset  = %12\n"
			"\t\t.first_datablock     = %13\n"
			"\t}\n"
			"}", 
			HUSHFS_VERSION,
			HUSHFS_BLOCK_SIZE,
			filelen,
			num_inodes,
			num_blocks,
			ibb,
			bbb,
			inode_table_blocks,
			inodes_per_block,
			start_bitmap_block,
			start_bitmap_block + ibb,
			start_bitmap_block + ibb + bbb,
			start_bitmap_block + ibb + bbb + inode_table_blocks
	);

	write_block(fd, sb.get(), 0);

	logger.info("Wrote superblock, size: %1", sizeof(sb));
	return sb;
}

static void write_inode_bitmap(int fd, std::shared_ptr<Superblock> const & sb)
{
	uint8_t *map;
	uint64_t size = sb->fields.inode_bitmap_blocks * HUSHFS_BLOCK_SIZE;

	map = new uint8_t[size] {};

	// no inodes are used right now, so we write an empty map
	write_data(fd, map, sb->fields.inode_bitmap_offset * HUSHFS_BLOCK_SIZE, size, true);
	logger.info("Wrote inode bitmap, size: %1", size);

	delete[] map;
}

static void write_block_bitmap(int fd, std::shared_ptr<Superblock> const & sb)
{
	uint64_t j = 0, size = sb->fields.block_bitmap_blocks * HUSHFS_BLOCK_SIZE;
	uint8_t *map = new uint8_t[size] {};
	uint8_t byte = map[j];

	for (uint64_t i = 0; i < sb->fields.first_datablock; i++) {
		if (i != 0 && (i % 8) == 0) {
			j++;
			byte = map[j];
		}
		byte = byte | (0x80 >> i);
	}

	write_data(fd, map, sb->fields.block_bitmap_offset * HUSHFS_BLOCK_SIZE, size, true);

	logger.info("Wrote block bitmap, size: %1", size);

	delete[] map;
}

static void write_inode_table(int fd, std::shared_ptr<Superblock> const & sb)
{
	hush::fs::InodeTableBlock block = {};
	uint64_t startblock = sb->fields.inode_table_offset;

	logger.debug("Writing inode table: %1 blocks", sb->fields.inode_table_blocks);
	// the inodes are all initially empty so we just write zeros here
	for (int i = 0; i < sb->fields.inode_table_blocks; i++) {
		write_block(fd, &block, (startblock++) * HUSHFS_BLOCK_SIZE, true);
	}

	size_t tell = lseek(fd, 0, SEEK_CUR);
	logger.debug("tell = %1 block = %2", tell, tell / HUSHFS_BLOCK_SIZE);
}

static void write_root_inode(int fd, std::shared_ptr<Superblock> const & sb)
{
	struct timespec ts = {};
	size_t bytes;

	clock_gettime(CLOCK_REALTIME, &ts);
	hush::fs::Inode inode = {
		.fields = {
			.mode         = S_IFDIR,
			.inode_number = 1, // root directory inode is 1
			.uid          = getuid(), // these should shadow your local user/group
			.gid          = getgid(),
			.atime        = ts,
			.mtime        = ts,
			.ctime        = ts,
			.dir_children = 0,
		},
	};

	bytes = write(fd, &inode, sizeof inode);

	if (bytes != sizeof inode) {
		LogString ls("Error writing root inode: Wrote %1 bytes, expected %2 ", bytes, sizeof inode);
		logger.critical(ls);
		throw ls.str();
	}

	logger.info("Wrote root inode, size: %1", sizeof(inode));
}

static void usage()
{
	std::cerr << "Usage " << prgname << " [-S] -k .path/to/keyfile -s N[k|g|m] secret.img" << std::endl;
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
		goto close_and_exit;
	}

	try {
		format(fd, filelen);
	} catch (...) {
		close(fd);
		throw;
	}

	if (no_sparse) {
		logger.info("Not creating sparse file");
		char empty[8192] = {};
		while ((curpos = lseek(fd, 0, SEEK_CUR)) < filelen) {
			uint64_t sz = filelen-curpos;

			sz = sz > 8192 ? 8192 : sz;
			write(fd, empty, sz);
		}
	} else {
		logger.info("Creating sparse file");
		// create sparse file
		lseek(fd, filelen-1, SEEK_SET);
		write(fd, &nullbyte, 1);
	}


close_and_exit:
	close(fd);

bye:
	return ret;
}
