#ifndef B64_HH_
#define B64_HH_

#include "crypto/ciphertext.hh"

namespace hush {
	namespace utils {
		 
		// T should be some kind of vector<unsigned char>
		// R should be some kind of string
		template<class T, class R>
		class B64 
		{
		public:
			const R encode(const T& in)
			{
				R out;
				unsigned char a, b, c, oa, ob, oc, od;
				int pad = 0;
			
				for (auto it = in.begin(); it != in.end() && pad == 0; it+=3) {
					a = *it;
					if ((it+1) == in.end()) {
						b = c = 0;
						pad = 2;
					} else if ((it+2) == in.end()) {
						b = *(it+1);
						c = 0;
						pad = 1;
					} else {
						b = *(it+1);
						c = *(it+2);
					}
			
					oa = (a & 0xFC) >> 2;
					ob = ((a & 0x03) << 4) | ((b & 0xF0) >> 4);
					oc = ((b & 0x0F) << 2) | ((c & 0xC0) >> 6);
					od = c & 0x3F;
			
					out.push_back(b64chars[oa]);
					out.push_back(b64chars[ob]);
					if (pad == 2) {
						out.append("==");
					} else if (pad == 1) {
						out.push_back(b64chars[oc]);
						out.append("=");
					} else {
						out.push_back(b64chars[oc]);
						out.push_back(b64chars[od]);
					}
				}
			
				return out;
			};

			const R encode(const hush::crypto::CipherText &ct)
			{
				R out;
			
				out.append(encode(ct.get_nonce()));
				out.append(encode(ct.get_data()));
			
				return out;
			};

			const R pemify(const R& in)
			{
				R out;
				int linelen = 0;
			
				out = "-----BEGIN SODIUM PRIVATE KEY-----\r\n";
				for (auto it = in.begin(); it != in.end(); it++) {
					out.push_back(*it);
					if (++linelen == 64) {
						linelen = 0;
						out.append("\r\n");
					}
				}
				if (linelen != 0)
					out.append("\r\n");
				out.append("-----BEGIN SODIUM PRIVATE KEY-----");
				return out;
			};

		private:
			const char *b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		};
	};
};

#endif /* B64_HH_ */
