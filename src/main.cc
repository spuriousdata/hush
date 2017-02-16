#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm> // transform
#include <iterator> // back_inserter
#include <sys/stat.h>

#include "config.h"

#include "keygen.hh"
#include "create.hh"
#include "mount.hh"
#include "utils/optparse.h"

std::string prgname;

static void usage()
{
	std::cerr << "Usage " << prgname << " [keygen|create|mount] [...]" << std::endl;
}

int main(int argc, char **argv)
{
	std::string mode;
	struct optparse opts;
	int opt, ret;
	char *tmp;
	std::vector<std::string> args_in;
	std::vector<char*> args_out;

	umask(0);

	prgname = argv[0];

	optparse_init(&opts, argv);
	opts.permute = 0;

	while ((opt = optparse(&opts, "Vh")) != -1) {
		switch (opt) {
			case 'V':
				std::cout << HUSH_VERSION << std::endl;
				return 0;
			case 'h':
				usage();
				return 0;
			case '?':
			default:
				usage();
				return 1;
		}
	}

	if ((tmp = optparse_arg(&opts)) == 0) {
		usage();
		return 1;
	}
	mode = tmp;

	prgname.append(" " + mode);

	if (mode == "keygen")
		return hush_keygen(&opts);
	else if (mode == "create")
		return hush_create(&opts);
	else if (mode == "mount") {
		/*
		 * I really hate doing this, but fuse REALLY wants to parse the cmdline
		 * args and prior to 3.0, which isn't installed or available most
		 * anywhere, it actually stores information in argv.
		 */
		args_in.push_back(prgname);
		for (int i = opts.optind; i < argc; i++)
			args_in.push_back(opts.argv[i]);

		std::transform(args_in.begin(),
					   args_in.end(),
					   std::back_inserter(args_out),
					   [] (const std::string &s) -> char * {
						return strdup(s.c_str());
					   });

		ret = hush_mount(args_out.size(), args_out.data());
		for (auto it = args_out.begin(); it != args_out.end(); it++)
			free(*it);
		return ret;
	} else {
		usage();
		return 1;
	}

	return 0;
}
