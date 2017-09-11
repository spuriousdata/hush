#ifndef FS_H_
#define FS_H_

#include <cstdint>
#include <sys/types.h>
#include <time.h>

#include "config.h"

namespace hush {
	namespace fs {
		char     const *MAGIC            = "HusH";
		uint32_t const  BLOCK_SIZE       = HUSHFS_BLOCK_SIZE;
		uint16_t const  FILENAME_MAXLEN  = 255;
		uint16_t const  INODE_ALIGN_SIZE = 256;
		uint16_t const  INODES_PER_BLOCK = (uint64_t)(BLOCK_SIZE / INODE_ALIGN_SIZE);

		/*
		 * alignas(8) doesn't actually do anything since this is the default
		 * alignment of this struct. It's just there to point out the 8-byte
		 * alignment.
		 */
		using SuperblockStats = struct alignas(8) {
			    char magic[4];
			uint32_t version;
			uint32_t block_size;
			uint32_t padding; // unsed
			uint64_t disk_size;
			uint64_t total_inodes;
			uint64_t total_blocks;
			uint64_t inode_bitmap_blocks;
			uint64_t block_bitmap_blocks;
			uint64_t inode_table_blocks;
			uint64_t inodes_per_block;
			uint64_t inode_bitmap_offset;
			uint64_t block_bitmap_offset;
			uint64_t inode_table_offset;
			uint64_t first_datablock;
		};

		using Superblock = struct {
			SuperblockStats fields;
			uint8_t padding[BLOCK_SIZE - sizeof(SuperblockStats)];	
		};

		using Datablock = struct {
			uint8_t data[BLOCK_SIZE];
		};

		// https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout#Overview
		using IndirectBlock = struct __indirect_block {
			union {
				Datablock *blocks[BLOCK_SIZE / sizeof(Datablock *)];
				struct __indirect_block *i_blocks[BLOCK_SIZE / sizeof(Datablock *)];
			};
		};

		using InodeData = struct {
			mode_t mode;
			uint64_t inode_number;
			uint64_t block_number;
			uid_t uid;
			gid_t gid;
			struct timespec atime;
			struct timespec mtime;
			struct timespec ctime;

			uint64_t direct_ptr[12];
			uint64_t single_indirect_ptr;
			uint64_t double_indirect_ptr;
			uint64_t triple_indirect_ptr;

			union {
				uint64_t file_size;
				uint64_t dir_children;
			};
		};

		using Inode = struct {
			InodeData fields;
			uint8_t padding[INODE_ALIGN_SIZE - sizeof(InodeData)];
		};

		using InodeTableBlock = struct {
			Inode inodes[BLOCK_SIZE / sizeof(Inode)];
		};

		using InodeTable = struct {
			InodeTableBlock blocks[];
		};

		using DirEnt = struct {
			char name[FILENAME_MAXLEN];
			uint64_t i_no;
		};
	};
};

#endif /* FS_H_ */
