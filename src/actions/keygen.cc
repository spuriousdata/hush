#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <unistd.h> // access
#include <sodium.h>

#include "keygen.hh"
#include "secure.hh"
#include "utils/optparse.h"
#include "utils/password.hh"
#include "utils/b64.hh"
#include "crypto/secretkey.hh"
#include "crypto/symmetric.hh"
#include "crypto/ciphertext.hh"

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
	size_t idx;
	hush::utils::Password password;
	hush::crypto::SecretKey secretkey;
	hush::crypto::CipherText ciphertext;
	hush::crypto::Symmetric symmetric;
	std::string pem;
	hush::utils::B64<std::vector<unsigned char>, std::string> b64;

	while ((opt = optparse(opts, "p:h")) != -1) {
		switch (opt) {
			case 'p':
				privpath = opts->optarg;
				break;
			case 'h':
			default:
				usage();
				return 1;
		}
	}

	if ((idx = privpath.find("~")) != -1)
		privpath.replace(privpath.find("~"), 1, getenv("HOME"));

	if (access(privpath.c_str(), F_OK) != -1) {
		std::cerr << "Error! keyfile " << privpath << " already exits" << std::endl;
		return 1;
	}

	if (sodium_init() == -1) {
		std::cerr << "Error initializing libsodium" << std::endl;
		return 1;
	}

	password.ask("Password: ", true, true);
	secretkey.set_salt();
	secretkey.generate_key(password.get());

	pubkey = (unsigned char *)sodium_malloc(PKLEN);
	privkey = (unsigned char *)sodium_malloc(SKLEN);

	printf("Generating Key Pair\n");
	crypto_box_keypair(pubkey, privkey);

	hush::secure::vector<unsigned char> message(privkey, privkey+SKLEN);
	symmetric.encipher(ciphertext, secretkey, message);
	pem = b64.pemify(b64.encode(ciphertext));

	fp = fopen(privpath.c_str(), "w");
	fwrite(pem.data(), 1, pem.size(), fp);
	fclose(fp);

	pubpath = privpath + ".pub";

	fp = fopen(pubpath.c_str(), "w");
	fwrite(pubkey, 1, PKLEN, fp);
	fclose(fp);

	if (pubkey)
		sodium_free(pubkey);

	if (privkey)
		sodium_free(privkey);

	return ret;
}
