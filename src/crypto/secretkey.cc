#include <cstring>
#include <sodium.h>

#include "utils/secure.hh"
#include "crypto/secretkey.hh"

using namespace hush::crypto;

void SecretKey::set_salt(unsigned char *s)
{
	if (s)
		memcpy(salt, s, sizeof salt);
	else
		randombytes_buf(salt, sizeof salt);
	has_salt = true;
}

void SecretKey::set_key(unsigned char *k)
{
	memcpy(key, k, crypto_box_SEEDBYTES);
	has_key = true;
}

void SecretKey::generate_key(hush::secure::string const & input)
{
	if (!has_salt)
		set_salt();

	if (!has_key) {
		if (crypto_pwhash(key, crypto_box_SEEDBYTES, input.c_str(), input.length(),
					salt, crypto_pwhash_OPSLIMIT_INTERACTIVE, 
					crypto_pwhash_MEMLIMIT_INTERACTIVE, 
					crypto_pwhash_ALG_DEFAULT) != 0)
			throw SecretKeyException("Can't hash password into key, out of memory");
		has_key = true;
	} else { // has_key
		throw SecretKeyException("Already have a key, aborting!");
	} 
}

