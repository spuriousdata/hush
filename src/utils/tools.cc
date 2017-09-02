#include <iostream>
#include <string>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "utils/tools.hh"
#include "utils/log.hh"
#include "config.h"

static slog::Log logger(LOGLEVEL);

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
		throw CreateAndWriteException(ls.string());
	}

	if ((fp = fdopen(fd, "w")) == NULL) {
		slog::LogString ls("Error getting FILE pointer from fd %1", fd);
		logger.error(ls);
		throw CreateAndWriteException(ls.string());
	}

	if ((wrote = fwrite(data, 1, datalen, fp)) != datalen) {
		slog::LogString ls("Error writing data. Excepted to write %1 bytes, but only wrote %2 bytes.", datalen, wrote);
		logger.error(ls);
		throw CreateAndWriteException(ls.string());
	}

	if (fclose(fp) != 0) {
		logger.error("Couldn't close file!");
		throw CreateAndWriteException("Couldn't close file!");
	}
}
