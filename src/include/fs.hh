#ifndef FS_H_
#define FS_H_

#include <cstdint>
#include <sys/types.h>
#include <time.h>

#include "config.h"

namespace hush {
	namespace fs {
		uint32_t const MAGIC = 0x48757348; // HusH
		uint32_t const BLOCK_SIZE = HUSHFS_BLOCK_SIZE;
		uint16_t const FILENAME_MAXLEN = 255;

		typedef struct {
			uint32_t magic;
			 uint8_t version;
			uint32_t block_size;
			uint64_t disk_size;
			uint64_t total_inodes;
			uint64_t total_blocks;
			uint64_t inode_bitmap_blocks;
			uint64_t block_bitmap_blocks;
		} superblock_stats_t;

		typedef struct {
			superblock_stats_t fields;
			char padding[BLOCK_SIZE - sizeof(superblock_stats_t)];	
		} superblock_t;

		typedef struct {
			unsigned char data[BLOCK_SIZE];
		} datablock_t;

		// https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout#Overview
		typedef struct __indirect_block {
			union {
				datablock_t *blocks[BLOCK_SIZE / sizeof(datablock_t *)];
				struct __indirect_block *i_blocks[BLOCK_SIZE / sizeof(datablock_t *)];
			};
		} indirect_block_t;

		typedef struct {
			mode_t mode;
			uint64_t inode_number;
			uint64_t block_number;
			uid_t uid;
			gid_t gid;
			struct timespec atime;
			struct timespec mtime;
			struct timespec ctime;

			datablock_t *direct[12];
			indirect_block_t *first;
			indirect_block_t **second;
			indirect_block_t ***third;

			union {
				uint64_t file_size;
				uint64_t dir_children;
			};
		} inode_t;

		typedef struct {
			uint64_t used_inodes;
		} inode_table_t;

		typedef struct {
			char name[FILENAME_MAXLEN];
			uint64_t i_no;
		} dirent_t;
	};
};

#endif /* FS_H_ */
