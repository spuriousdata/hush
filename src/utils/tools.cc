#include <iostream>
#include <string>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "config.h"

#include "utils/tools.hh"
#include "utils/log.hh"
#include "utils/mountinfo.hh"

using hush::fs::FileType;
using hush::fs::Inode;
using hush::fs::InodeTable;
using hush::fs::MountInfo;

static slog::Log logger(slog::LogLevel::DEBUG);

template<typename T>
void split(std::string const &s, char delimiter, T result)
{
	std::stringstream ss;
	std::string item;

	ss.str(s);
	while (std::getline(ss, item, delimiter)) {
		*(result++) = item;
	}
}

std::vector<std::string> split(std::string const &s, char delimiter)
{
	std::vector<std::string> elements;
	split(s, delimiter, std::back_inserter(elements));
	return elements;
}

void create_and_write(std::string const & name, void const *data, size_t datalen, int mode)
{
	int fd;
	FILE *fp;
	size_t wrote;
	struct stat statbuf;
	std::vector<std::string> parts;
	std::string sofar;

	parts = split(name, '/');
	for (auto it = parts.begin(); (it+1) != parts.end(); it++) {
		if ((*it).empty())
			continue;
		sofar += "/" + *it;

		if (stat(sofar.c_str(), &statbuf) != 0) {
			logger.info("Attempting to create directory %1", sofar);
			mkdir(sofar.c_str(), 0700);
		}
	}

	if ((fd = open(name.c_str(), O_CREAT | O_RDWR | O_EXCL, mode)) == -1) {
		slog::LogString ls("Error opening file %1", name);
		logger.error(ls);
		throw CreateAndWriteException(ls.str());
	}

	if ((fp = fdopen(fd, "w")) == NULL) {
		slog::LogString ls("Error getting FILE pointer from fd %1", fd);
		logger.error(ls);
		throw CreateAndWriteException(ls.str());
	}

	if ((wrote = fwrite(data, 1, datalen, fp)) != datalen) {
		slog::LogString ls("Error writing data. Excepted to write %1 bytes, but only wrote %2 bytes.", datalen, wrote);
		logger.error(ls);
		throw CreateAndWriteException(ls.str());
	}

	if (fclose(fp) != 0) {
		logger.error("Couldn't close file!");
		throw CreateAndWriteException("Couldn't close file!");
	}
}

void write_data(int fd, void const * buf, off_t from, uint64_t len, bool error_seek)
{
	off_t oldpos = lseek(fd, 0, SEEK_CUR);
	ssize_t wrote = 0; 


	if (error_seek && oldpos != from) {
		slog::LogString ls("Seek error. Specified start location %1 is not the same as natural location of fd %2", from, oldpos);
		logger.error(ls);
		throw ls.str();
	}

	lseek(fd, from, SEEK_SET);

	if ((wrote = write(fd, buf, len)) != len) {
		slog::LogString ls("Error writing data, excpected to write %1 bytes, but wrote %2 bytes.", len, wrote);
		logger.error(ls);
		throw ls.str();
	}
}

void write_block(int fd, void const * buf, off_t from, bool error_seek)
{
	write_data(fd, buf, from, HUSHFS_BLOCK_SIZE, error_seek);
}

void write_inode(int fd, Superblock const *sb, std::string const & name, 
		FileType typ, mode_t umask, void const * buf, uint64_t i_no)
{
	struct timespec ts = {};
	Inode *inode = new Inode {};

	clock_gettime(CLOCK_REALTIME, &ts);

	inode->fields.mode = (typ == FileType::File) ? (0666 & ~umask) : (0777 & ~umask);
	inode->fields.uid = getuid();
	inode->fields.gid = getgid();
	inode->fields.type = typ;
	inode->fields.inode_number = MountInfo::get_instance(fd).next_available_inode();
	inode->fields.atime = ts;
	inode->fields.mtime = ts;
	inode->fields.ctime = ts;
}
