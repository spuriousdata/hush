#include <vector>
#include <sodium.h>
#include <cstdint>
#include "crypto/symmetric.hh"
#include "crypto/ciphertext.hh"
#include "secure.hh"

using namespace hush::crypto;

void Symmetric::encipher(CipherText& dest, const SecretKey& sk,
		hush::secure::vector<unsigned char>& message)
{
	unsigned char *data;
	unsigned char nonce[crypto_secretbox_NONCEBYTES];
	// WARNING
	// If the vector ever has a type other than char, then this will need to be
	// modified to multiply the sizeof(T) * message.size().
	uint64_t msglen = crypto_secretbox_MACBYTES + message.size();

	randombytes_buf(nonce, sizeof nonce);

	data = (unsigned char *)sodium_malloc(msglen);

	crypto_secretbox_easy(data, message.data(), message.size(),
	                          nonce, sk.get_key());

	dest.set(nonce, sizeof nonce, data, msglen);
}
