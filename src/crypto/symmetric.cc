#include <vector>
#include <sodium.h>
#include "crypto/symmetric.hh"
#include "crypto/ciphertext.hh"
#include "secure.hh"

using namespace hush::crypto;

void Symmetric::encipher(CipherText& dest, const SecretKey& sk,
		hush::secure::vector<unsigned char>& message)
{
	unsigned char *data;
	unsigned char nonce[crypto_secretbox_NONCEBYTES];

	randombytes_buf(nonce, sizeof nonce);

	// WARNING
	// If the vector ever has a type other than char, then this will need to be
	// modified to multiply the sizeof(T) * message.size().
	data = (unsigned char *)sodium_malloc(crypto_secretbox_MACBYTES + message.size());

	crypto_secretbox_easy(data, message.data(), message.size(),
	                          nonce, sk.get_key());
	dest.set(nonce, data);
}
