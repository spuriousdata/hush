#ifndef MOUNTINFO_HH_
#define MOUNTINFO_HH_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fs.hh"

using hush::fs::Superblock;

namespace hush
{
	namespace fs
	{
		class MountInfo
		{
			public:
				static MountInfo & get_instance(int fd); 
				~MountInfo();

				MountInfo(MountInfo const &) = delete;
				void operator=(MountInfo const &) = delete;

				uint64_t next_available_inode(bool mark_used=false);

			private:
				int fd;
				Superblock superblock;
				uint64_t inode_map_bytes;
				uint64_t block_map_bytes;
				uint8_t *inode_bitmap;
				uint8_t *block_bitmap;

				MountInfo(int fd);
				void read_superblock();
				void read_inode_bitmap();
				void read_block_bitmap();
		};
	};
};

#endif /* MOUNTINFO_HH_ */

