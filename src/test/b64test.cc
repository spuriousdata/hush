#include <iostream>
#include <vector>
#include <string>
#include "utils/b64.hh"

int main()
{
	std::vector<char> in, out;
	std::string s;
	hush::utils::B64<std::vector<char>, std::string> b;

	s = "This is a test of the emergency broadcast system.";

	for (auto it=s.begin(); it != s.end(); it++)
		in.push_back(*it);

	std::cout << b.encode(in) << std::endl;
	std::cout << b.unpemify(b.pemify(b.encode(in), "TEST")) << std::endl;
	
	std::cout << s << std::endl;

	out = b.decode(b.encode(in));
	s.assign(out.begin(), out.end());
	std::cout << s << std::endl;
}
