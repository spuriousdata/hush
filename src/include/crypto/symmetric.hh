#ifndef SYMMETRIC_HH_
#define SYMMETRIC_HH_

#include <sodium.h>
#include "crypto/secretkey.hh"
#include "crypto/ciphertext.hh"
#include "utils/secure.hh"

namespace hush {
	namespace crypto {
		class Symmetric
		{
		public:
			void encipher(CipherText& dest, const SecretKey& sk,
					hush::secure::vector<unsigned char>& message);
		};
	};
};

#endif /* SYMMETRIC_HH_ */

