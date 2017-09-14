#include <iostream>
#include <string>
#include "utils/log.hh"
#include "test/catch.hpp"

using Catch::Matchers::StartsWith;

TEST_CASE( "Log Levels", "[slog::Log]" ) {
	std::ostringstream os;
	slog::Log logger(slog::LogLevel::INFO, os);

	logger.set_format("");

	logger.debug("I should not be printed");

	REQUIRE(os.str() == "");

	logger.info("I should be printed");

	REQUIRE(os.str() == "I should be printed\n");
}


TEST_CASE( "Interpolation passthrough slog::Log -> slog::LogString", "[slog::Log]" ) {

	std::ostringstream os;
	slog::Log logger(slog::LogLevel::DEBUG, os);

	logger.set_format("");

	SECTION( "Basic Interpolation" ) {
		logger.info("%1 and '%2' and %1", 32, std::string("foo"));
		REQUIRE(os.str() == "32 and 'foo' and 32\n");
	}

	SECTION( "float interpolation" ) {
		logger.info("%1", 3.14159);
		REQUIRE_THAT(os.str(), StartsWith("3.14159"));
	}

	SECTION( "double interpolation") {
		double x = 3.14159;
		logger.info("%1", x);
		REQUIRE_THAT(os.str(), StartsWith("3.14159"));
	}

	SECTION( "integer interpolation" ) {
		logger.info("%1", 32);
		REQUIRE(os.str() == "32\n");
	}

	SECTION( "std::string interpolation" ) {
		std::string x = "Foo Bar";
		logger.info("%1", x);
		REQUIRE(os.str() == "Foo Bar\n");
	}
}

TEST_CASE( "Placeholders", "[slog::LogString]" ) {

	SECTION( "Without replacement" ) {
		slog::LogString ls("%1 %2 %3");
		REQUIRE(ls.str() == "%1 %2 %3");
	}

	SECTION( "In order - single digit - variadic call" ) {
		REQUIRE(slog::LogString("%1 %2 %3", 1, 23, 45).str() == "1 23 45");
	}

	SECTION( "In order - single digit - sequential call" ) {
		slog::LogString ls("%1 %2 %3");
		ls.arg(1);
		ls.arg(23);
		ls.arg(45);
		REQUIRE(ls.str() == "1 23 45");
	}

	SECTION( "In order - single digit - chain call" ) {
		slog::LogString ls("%1 %2 %3");
		ls.arg(1).arg(23).arg(45);
		REQUIRE(ls.str() == "1 23 45");
	}

	SECTION( "Out of order - single digit - variadic call" ) {
		REQUIRE(slog::LogString("%1 %9 %3", 1, 23, 45).str() == "1 45 23");
	}

	SECTION( "Out of order - single digit - sequential call" ) {
		slog::LogString ls("%1 %9 %3");
		ls.arg(1);
		ls.arg(23);
		ls.arg(45);
		REQUIRE(ls.str() == "1 45 23");
	}

	SECTION( "Out of order - single digit - chain call" ) {
		slog::LogString ls("%1 %9 %3");
		ls.arg(1).arg(23).arg(45);
		REQUIRE(ls.str() == "1 45 23");
	}

	SECTION( "Out of order - double digit - variadic call" ) {
		REQUIRE(slog::LogString("%12 %19 %3", 1, 23, 45).str() == "23 45 1");
	}

	SECTION( "Out of order - double digit - sequential call" ) {
		slog::LogString ls("%12 %19 %3");
		ls.arg(1);
		ls.arg(23);
		ls.arg(45);
		REQUIRE(ls.str() == "23 45 1");
	}

	SECTION( "Out of order - double digit - chain call" ) {
		slog::LogString ls("%12 %19 %3");
		ls.arg(1).arg(23).arg(45);
		REQUIRE(ls.str() == "23 45 1");
	}
}
