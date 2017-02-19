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
	int opt;
	char *tmp;

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
		return hush_mount(argc, &opts);
	} else {
		usage();
		return 1;
	}

	return 0;
}
