#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <unistd.h> // access
#include <sodium.h>

#include "keygen.hh"
#include "utils/optparse.h"

#define PKLEN crypto_box_PUBLICKEYBYTES
#define SKLEN crypto_box_SECRETKEYBYTES

extern std::string prgname;
static void usage();

static void usage()
{
	std::cerr << "Usage " << prgname << " -p .path/to/privkey" << std::endl;
}


int hush_keygen(struct optparse *opts)
{
	int opt, ret = 0;
	unsigned char *pubkey = NULL;
	unsigned char *privkey = NULL;
	std::string pubpath, privpath(DEFAULT_KEYPATH);
	FILE *fp;

	while ((opt = optparse(opts, "p:h")) != -1) {
		switch (opt) {
			case 'p':
				privpath = optarg;
				break;
			case 'h':
			default:
				usage();
				ret = 1;
				goto bye;
		}
	}

	privpath.replace(privpath.find("~"), 1, getenv("HOME"));

	if (access(privpath.c_str(), F_OK) != -1) {
		std::cerr << "Error! keyfile " << privpath << " already exits" << std::endl;
		ret = 1;
		goto bye;
	}

	if (sodium_init() == -1) {
		std::cerr << "Error initializing libsodium" << std::endl;
		ret = 1;
		goto bye;
	}

	pubkey = (unsigned char *)sodium_malloc(PKLEN);
	privkey = (unsigned char *)sodium_malloc(SKLEN);

	printf("Generating Key Pair\n");
	crypto_box_keypair(pubkey, privkey);

	fp = fopen(privpath.c_str(), "w");
	fwrite(privkey, 1, SKLEN, fp);
	fclose(fp);

	pubpath = privpath + ".pub";

	fp = fopen(pubpath.c_str(), "w");
	fwrite(pubkey, 1, PKLEN, fp);
	fclose(fp);

bye:
	if (pubkey)
		sodium_free(pubkey);

	if (privkey)
		sodium_free(privkey);

	return ret;
}
