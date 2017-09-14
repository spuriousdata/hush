#ifndef FS_H_
#define FS_H_

#include <cstdint>
#include <sys/types.h>
#include <time.h>

#include "config.h"

namespace hush {
	namespace fs {

		enum class FileType : uint32_t { File, Directory };

		using SuperblockStats = struct alignas(8) {
			    char magic[4];
			uint8_t  version;
			    char unused[7];
			uint32_t block_size;
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

		using Superblock = struct alignas(8) {
			SuperblockStats fields;
			uint8_t padding[HUSHFS_BLOCK_SIZE - sizeof(SuperblockStats)];	
		};

		using Datablock = struct alignas(8) {
			uint8_t data[HUSHFS_BLOCK_SIZE];
		};

		// https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout#Overview
		using IndirectBlock = struct alignas(8) __indirect_block {
			union {
				Datablock *blocks[HUSHFS_BLOCK_SIZE / sizeof(Datablock *)];
				struct __indirect_block *i_blocks[HUSHFS_BLOCK_SIZE / sizeof(Datablock *)];
			};
		};

		using InodeData = struct alignas(8) {
			mode_t mode; //uint32
			uid_t uid; //uint32
			gid_t gid; //uint32
			FileType type; //uint32
			uint64_t inode_number;
			struct timespec atime; //uint64_t[2]
			struct timespec mtime; //uint64_t[2]
			struct timespec ctime; //uint64_t[2]

			uint64_t direct_ptr[12];
			uint64_t single_indirect_ptr;
			uint64_t double_indirect_ptr;
			uint64_t triple_indirect_ptr;

			union {
				uint64_t file_size;
				uint64_t dir_children;
			};
		};

		using Inode = struct alignas(8) {
			InodeData fields;
			uint8_t padding[HUSHFS_INODE_ALIGN_SIZE - sizeof(InodeData)];
		};

		using InodeTableBlock = struct alignas(8) {
			Inode inodes[HUSHFS_BLOCK_SIZE / sizeof(Inode)];
		};

		using InodeTable = struct alignas(8) {
			InodeTableBlock blocks[];
		};

		// total length 256
		using DirEnt = struct alignas(8) {
			char name[HUSHFS_FILENAME_MAXLEN];
			uint64_t i_no;
		};
	};
};

#endif /* FS_H_ */
