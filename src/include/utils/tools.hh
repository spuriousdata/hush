#ifndef TOOLS_HH_
#define TOOLS_HH_

#include <string>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include "fs.hh"

using hush::fs::FileType;
using hush::fs::Superblock;

class CreateAndWriteException : public std::runtime_error
{
	using std::runtime_error::runtime_error;
	using std::runtime_error::what;
};

void create_and_write(std::string const & name, void const *data, size_t datalen, int mode=0600);

template<typename T> void split(std::string const &s, char delimiter, T results);
std::vector<std::string> split(std::string const &s, char delimiter);

void write_data(int fd, void const * buf, off_t from, uint64_t len, bool error_seek=false);
void write_block(int fd, void const * buf, off_t from, bool error_seek=false);

void write_inode(int fd, Superblock const * sb, std::string const & name, 
		FileType typ, mode_t umask=0022, void const * buf=nullptr, 
		uint64_t i_no=0);

#endif /* TOOLS_HH_ */

