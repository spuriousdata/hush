#include <iostream>
#include <string>
#include "utils/log.hh"

int main()
{
	slog::Log logger(slog::DEBUG);
	int x = 12;
	std::string foo = "My Name is Earl";

	logger.info("The value of x is %1 and the value of foo is '%2'. Remember x is %1", x, foo);

	return 0;
}
