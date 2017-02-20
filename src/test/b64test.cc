#include <iostream>
#include <vector>
#include <string>
#include "utils/b64.hh"

int main()
{
	std::vector<char> in;
	std::string s;
	hush::utils::B64<std::vector<char>, std::string> b;

	s = "This is a test of the emergency broadcast system.";

	for (auto it=s.begin(); it != s.end(); it++)
		in.push_back(*it);

	std::cout << b.encode(in) << std::endl;
}
