#ifndef B64_HH_
#define B64_HH_

#include <stdexcept>
#include <algorithm> // transform, toupper
#include "crypto/ciphertext.hh"

#define BETWEEN(x, a, b) ((x) >= (a) && (x) <= (b))

namespace hush {
	namespace utils {
		class B64Exception : public std::runtime_error
		{
			using std::runtime_error::runtime_error;
			using std::runtime_error::what;
		};
		 
		// T should be some kind of vector<unsigned char>
		// R should be some kind of string
		template<class T, class R>
		class B64 
		{
		public:
			const R encode(const T& in) const
			{
				R out;
				unsigned char a, b, c, oa, ob, oc, od;
				int pad = 0;
			
				for (auto it = in.begin(); it != in.end() && pad == 0; it += 3) {
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

			const T decode(const R& in) const
			{
				T out;
				unsigned char a, b, c, d;

				for (auto it = in.begin(); it != in.end(); it += 4) {
					a = decode_byte(*it);
					b = decode_byte(*(it+1));
					c = decode_byte(*(it+2));
					d = decode_byte(*(it+3));

					if (c == -1)
						c = 0;

					out.push_back((a << 2) | ((b & 0x30) >> 4));
					out.push_back(((b & 0x0F) << 4) | ((c & 0x3C) >> 2));
					if (d != -1)
						out.push_back(((c & 0x03) << 6) | d);
				}
				return out;
			};

			const R encode(const hush::crypto::CipherText &ct) const
			{
				R out;
			
				out.append(encode(ct.get_nonce()));
				out.append(encode(ct.get_data()));
			
				return out;
			};

			const R pemify(const R& in, const char *keytype) const
			{
				std::string k = keytype;
				return pemify(in, k);
			}

			const R pemify(const R& in, const std::string& keytype) const
			{
				R out;
				int linelen = 0;
				std::string kt = keytype;
				std::transform(kt.begin(), kt.end(), kt.begin(), ::toupper); 
			
				out = "-----BEGIN SODIUM "+ kt + " KEY-----\r\n";
				for (auto it = in.begin(); it != in.end(); it++) {
					out.push_back(*it);
					if (++linelen == 64) {
						linelen = 0;
						out.append("\r\n");
					}
				}
				if (linelen != 0)
					out.append("\r\n");
				out.append("-----END SODIUM " + kt + " KEY-----");
				return out;
			};

			const R unpemify(const R& in) const
			{
				R out;
				size_t pos;
				std::string marker = "-----";

				if (in.find(marker) != 0)
					throw B64Exception("Bad PEM data, can't unencode");

				out = in.substr(in.find("\r\n")+2, in.rfind("\r\n"));

				while ((pos = out.find("\r\n")) != -1)
					out.replace(pos, 2, "");

				return out.substr(0, out.find(marker));
			};

			const int decode_byte(char x) const
			{
				if (BETWEEN(x, 'A', 'Z')) {
					return x - 'A';
				} else if (BETWEEN(x, 'a', 'z')) {
					return (x - 'a') + 26;
				} else if (BETWEEN(x, '0', '9')) {
					return (x - '0') + 52;
				} else if (x == '+') {
					return 62;
				} else if (x == '/') {
					return 63;
				} else if (x == '=') {
					return -1;
				} else {
					throw B64Exception("Invalid Base 64 Character!");
				}
			};

		private:
			const char *b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		};
	};
};

#endif /* B64_HH_ */
