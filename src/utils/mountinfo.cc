#include <unistd.h>
#include "utils/mountinfo.hh"
#include "config.h"

using hush::fs::MountInfo;

MountInfo::MountInfo(int fd) : fd(fd)
{
	read_superblock();
	read_inode_bitmap();
	read_block_bitmap();
}

MountInfo::~MountInfo()
{
	delete[] inode_bitmap;
	delete[] block_bitmap;
}

MountInfo & MountInfo::get_instance(int fd)
{
	static MountInfo instance(fd);

	return instance;
}

void MountInfo::read_superblock()
{
	lseek(fd, 0, SEEK_SET);
	read(fd, &superblock, sizeof(superblock));
}

void MountInfo::read_inode_bitmap()
{
	inode_map_bytes = superblock.fields.inode_bitmap_blocks * HUSHFS_BLOCK_SIZE;

	inode_bitmap = new uint8_t[inode_map_bytes];
	lseek(fd, superblock.fields.inode_bitmap_offset, SEEK_SET);
	read(fd, inode_bitmap, inode_map_bytes);
}

void MountInfo::read_block_bitmap()
{
	block_map_bytes = superblock.fields.block_bitmap_blocks * HUSHFS_BLOCK_SIZE;

	block_bitmap = new uint8_t[block_map_bytes];
	lseek(fd, superblock.fields.block_bitmap_offset, SEEK_SET);
	read(fd, block_bitmap, block_map_bytes);
}

uint64_t MountInfo::next_available_inode(bool mark_used)
{
	uint64_t i_no = 0;
	uint8_t val = 0;

	for (int i = 0; i < inode_map_bytes; i++) {
		val = inode_bitmap[i];

		/*
		 * These maps start at the 'left' -- the 8th bit being 1, 7th being 2,
		 * etc. So if the lowest bit is set then they are all set and we can
		 * just jump to the next byte.
		 */
		if (val & 1) {
			i_no += 8;
			continue;
		}

		break;
	}

	while (val != 0) {
		i_no++;
		val >>= 1;
	}

	if (mark_used) {
		// set the bit in the map and write it back
	}

	return i_no + 1;
}
