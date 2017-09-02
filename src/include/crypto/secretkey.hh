#ifndef SECRETKEY_HH_
#define SECRETKEY_HH_

#include <stdexcept>
#include <sodium.h>
#include "utils/secure.hh"

namespace hush {
	namespace crypto {
		class SecretKeyException : public std::runtime_error
		{
			using std::runtime_error::runtime_error;
			using std::runtime_error::what;
		};

		class SecretKey
		{
		public:
			SecretKey()
			{
				key = (unsigned char*)sodium_malloc(crypto_box_SEEDBYTES);
			};

			~SecretKey() { sodium_free(key); };

			void set_salt(unsigned char *s=nullptr);
			void set_key(unsigned char *k);
			unsigned char const *get_key() const { return key; };
			void generate_key(hush::secure::string const & input);

		private:
			bool has_key = false;
			bool has_salt = false;
			unsigned char salt[crypto_pwhash_SALTBYTES];
			unsigned char *key;
		};
	};
};

#endif /* SECRETKEY_HH_ */

