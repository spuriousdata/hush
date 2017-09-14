#ifndef CONFIG_H_
#define CONFIG_H_

#define HUSH_VERSION "0.1"

#define HUSHFS_VERSION 1

#define KB 1024
#define MB (1024*KB)
#define GB (1024*MB)

#define HUSHFS_BLOCK_SIZE (4 * KB)
#define HUSHFS_MAGIC "HusH"
/*
 * 248 allows 8-byte alignment of struct and 256 byte size allowing even
 * alignment within blocks
 */
#define HUSHFS_FILENAME_MAXLEN 248

#define HUSHFS_INODE_ALIGN_SIZE 256
#define HUSHFS_INODES_PER_BLOCK ((uint64_t)(HUSHFS_BLOCK_SIZE / INODE_ALIGN_SIZE))

#endif /* CONFIG_H_ */

