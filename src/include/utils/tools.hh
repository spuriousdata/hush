#ifndef TOOLS_HH_
#define TOOLS_HH_

#include <string>
#include <stdexcept>

class CreateAndWriteException : public std::runtime_error
{
	using std::runtime_error::runtime_error;
	using std::runtime_error::what;
};

void create_and_write(const std::string& name, const void *data, size_t datalen, int mode=0600);

#endif /* TOOLS_HH_ */

