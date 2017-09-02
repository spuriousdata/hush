#include <iostream>
#include <vector>
#include <string>

std::vector<std::string> args;

void collect_args()
{
	return;
}

template<typename...Us>
void collect_args(std::string & first, Us... more)
{
	args.push_back(first);
	collect_args(more...);
}

template<typename...Us>
void collect_args(const char * first, Us... more)
{
	std::string s = first;
	args.push_back(s);
	collect_args(more...);
}

template<typename T, typename...Us>
void collect_args(T first, Us... more)
{
	args.push_back(std::to_string(first));
	collect_args(more...);
}


int main()
{
	std::string foo = "bar";

	collect_args(1, 2.5, 33, "foo", foo);

	for (auto i : args)
		std::cout << i << std::endl;

	return 0;
}
