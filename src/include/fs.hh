#ifndef FS_H_
#define FS_H_

#include <cstdint>
#include <sys/types.h>
#include <time.h>

namespace hush {
	namespace fs {
		uint64_t const MAGIC = 0x7345664568737548;
		uint16_t const BLOCK_SIZE = 4096;
		uint16_t const FILENAME_MAXLEN = 255;

		typedef struct {
			uint64_t version;
			uint64_t magic;
			uint64_t block_size;
			uint64_t inodes_count;
			//uint64_t free_blocks;
		} superblock_stats;

		typedef struct {
			superblock_stats fields;
			char padding[BLOCK_SIZE - sizeof(superblock_stats)];	
		} superblock;

		typedef struct {
			char name[FILENAME_MAXLEN];
			uint64_t inode_number;
		} file;

		typedef struct {
			mode_t mode;
			uint64_t inode_number;
			uint64_t block_number;
			uid_t uid;
			gid_t gid;
			struct timespec atime;
			struct timespec mtime;
			struct timespec ctime;
			superblock *superblock;

			union {
				uint64_t file_size;
				uint64_t directory_num_children;
			};
		} inode;
	};
};

#endif /* FS_H_ */

