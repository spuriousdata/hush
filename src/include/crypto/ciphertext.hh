#ifndef CIPHERTEXT_HH_
#define CIPHERTEXT_HH_

#include <cstdint>
#include <vector>

namespace hush {
	namespace crypto {
		class CipherText
		{
		public:
			CipherText() {};
			void set(unsigned char const *n, uint64_t nlen, unsigned char const *d, uint64_t dlen)
			{
				nonce.assign(n, n+nlen);
				data.assign(d, d+dlen);
			};

			std::vector<unsigned char> const & get_nonce() const { return nonce; };
			std::vector<unsigned char> const & get_data() const { return data; };

		private:
			// Can use std::vector here for ciphertext -- don't need a
			// hush::secure::vector since it's already encrypted
			std::vector<unsigned char> nonce, data;
		};
	};
};

#endif /* CIPHERTEXT_HH_ */

