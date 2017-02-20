#include <iostream>
#include <string>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>
#include "utils/tools.hh"

void create_and_write(const std::string& name, const void *data, size_t datalen, int mode)
{
	int fd;
	FILE *fp;
	size_t wrote;
	std::stringstream errmsg;

	if ((fd = open(name.c_str(), O_CREAT | O_RDWR | O_EXCL, mode)) == -1) {
		errmsg << "Error opening file " << name;
		throw CreateAndWriteException(errmsg.str());
	}

	if ((fp = fdopen(fd, "w")) == NULL) {
		errmsg << "Error getting FILE pointer from fd " << fd;
		throw CreateAndWriteException(errmsg.str());
	}

	if ((wrote = fwrite(data, 1, datalen, fp)) != datalen) {
		errmsg << "Error writing data. Excepted to write " <<
				datalen << " bytes, but only wrote " <<
				wrote << " bytes.";
		throw CreateAndWriteException(errmsg.str());
	}

	if (fclose(fp) != 0) {
		throw CreateAndWriteException("Couldn't close file!");
	}
}
