#ifndef TOOLS_HH_
#define TOOLS_HH_

#include <string>
#include <cstdint>
#include <stdexcept>
#include <vector>

class CreateAndWriteException : public std::runtime_error
{
	using std::runtime_error::runtime_error;
	using std::runtime_error::what;
};

void create_and_write(std::string const & name, void const *data, size_t datalen, int mode=0600);

template<typename T> void split(std::string const &s, char delimiter, T results);
std::vector<std::string> split(std::string const &s, char delimiter);

#endif /* TOOLS_HH_ */

