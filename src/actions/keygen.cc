#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <unistd.h> // access
#include <sodium.h>

#include "keygen.hh"
#include "utils/secure.hh"
#include "utils/optparse.h"
#include "utils/password.hh"
#include "utils/b64.hh"
#include "utils/tools.hh"
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
	size_t idx;
	std::string pubpath, privpath(DEFAULT_KEYPATH), pem;
	hush::utils::Password password;
	hush::crypto::SecretKey secretkey;
	hush::crypto::CipherText ciphertext;
	hush::crypto::Symmetric symmetric;
	hush::secure::vector<unsigned char> message;
	std::vector<unsigned char> s_message;
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

	message.clear();
	message.assign(privkey, privkey+SKLEN);
	symmetric.encipher(ciphertext, secretkey, message);
	pem = b64.pemify(b64.encode(ciphertext), "PRIVATE");

	create_and_write(privpath, pem.data(), pem.size(), 0600);

	pubpath = privpath + ".pub";

	s_message.clear();
	s_message.assign(pubkey, pubkey+PKLEN);
	pem = b64.pemify(b64.encode(s_message), "PUBLIC");
	create_and_write(pubpath, pem.data(), pem.size(), 0600);

	if (pubkey)
		sodium_free(pubkey);

	if (privkey)
		sodium_free(privkey);

	return ret;
}
