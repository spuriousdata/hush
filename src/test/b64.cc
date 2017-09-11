#include <iostream>
#include <vector>
#include <string>
#include "utils/b64.hh"
#include "test/catch.hpp"

TEST_CASE( "encode/decode", "[hush::utils::B64]" ) {
	hush::utils::B64<std::vector<char>, std::string> b;

	SECTION( "RFC4648 Test Vectors" ) {
		REQUIRE(b.encode("") == "");
		REQUIRE(b.encode("f") == "Zg==");
		REQUIRE(b.encode("fo") == "Zm8=");
		REQUIRE(b.encode("foo") == "Zm9v");
		REQUIRE(b.encode("foob") == "Zm9vYg==");
		REQUIRE(b.encode("fooba") == "Zm9vYmE=");
		REQUIRE(b.encode("foobar") == "Zm9vYmFy");
	}

	SECTION( "Basic sentence test" ) {
		std::vector<char> in, out;
		std::string s = "This is a test of the emergency broadcast system.";

		for (auto it=s.cbegin(); it != s.cend(); it++)
			in.push_back(*it);

		std::string enc = b.encode(in);
		out = b.decode(enc);
		std::string outstr(out.begin(), out.end());

		REQUIRE(outstr == s);
	}

	SECTION ("PEMIFY / UNPEMIFY" ) {
		std::string s = "This is a test of the emergency broadcast system.";
		std::string enc = b.encode(s);

		REQUIRE(b.unpemify(b.pemify(enc, "TEST")) == enc);
	}
}
