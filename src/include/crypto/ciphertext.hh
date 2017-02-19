#ifndef CIPHERTEXT_HH_
#define CIPHERTEXT_HH_

namespace hush {
	namespace crypto {
		class CipherText
		{
		public:
			CipherText() {};
			CipherText(const unsigned char* n, const unsigned char *d) :
				nonce(n), data(d) {};
			void set(const unsigned char *n, const unsigned char *d)
			{
				nonce = n;
				data = d;
			};
		private:
			const unsigned char *nonce, *data;
		};
	};
};

#endif /* CIPHERTEXT_HH_ */

