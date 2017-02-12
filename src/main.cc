#include <iostream>
#include <string>
#include <sys/stat.h>

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

	while ((opt = optparse(&opts, "h")) != -1) {
		switch (opt) {
			case 'h':
			case '?':
			default:
				usage();
				return 0;
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
	else if (mode == "mount")
		return hush_mount(&opts);
	else {
		usage();
		return 1;
	}

	return 0;
}
