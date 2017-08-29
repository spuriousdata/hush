#include <iostream>
#include <string>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include "utils/tools.hh"
#include "utils/loguru.hh"

template<typename T>
void split(const std::string &s, char delimiter, T result)
{
	std::stringstream ss;
	std::string item;

	ss.str(s);
	while (std::getline(ss, item, delimiter)) {
		*(result++) = item;
	}
}

std::vector<std::string> split(const std::string &s, char delimiter)
{
	std::vector<std::string> elements;
	split(s, delimiter, std::back_inserter(elements));
	return elements;
}

void create_and_write(const std::string& name, const void *data, size_t datalen, int mode)
{
	int fd;
	FILE *fp;
	size_t wrote;
	struct stat statbuf;
	std::stringstream errmsg;
	std::vector<std::string> parts;
	std::string sofar;

	parts = split(name, '/');
	for (auto it = parts.begin(); (it+1) != parts.end(); it++) {
		if ((*it).empty())
			continue;
		sofar += "/" + *it;

		/*
		DLOG_F(INFO, "sofar = %s", sofar.c_str());
		DLOG_F(INFO, "*it = %s", it->c_str());
		*/
		
		if (stat(sofar.c_str(), &statbuf) != 0) {
			LOG_F(INFO, "Attempting to create directory %s", sofar.c_str());
			mkdir(sofar.c_str(), 0700);
		}
	}

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
