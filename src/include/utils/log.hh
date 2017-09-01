#ifndef LOG_HH_
#define LOG_HH_

#include <cstdarg>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>

#define CHAR_TO_INT(x) ((x) - '0')
#define IS_DIGIT(x) ((x) >= '0' && (x) <= '9')

namespace slog {
	typedef enum { DEBUG, INFO, WARNING, ERROR, CRITICAL, } level;

	typedef struct {
		std::string::iterator start;
		std::string::iterator end;
		uint8_t replacement_number;
	} replacement;

	class LogString
	{
		public:
			LogString(std::string const &s)
			{
				fmtstring = s;
			}

			template<typename T>
			LogString arg(T i, int length=0, char pad=' ')
			{
				return arg(std::to_string(i), length, pad);
			}

			LogString arg(std::string const &i, int length=0, char pad=' ')
			{
				replacement *r;
				std::string out = i;

				format(out, length, pad);

				r = next();
				do {
					fmtstring.replace(r->start, r->end, out);
					r = next(r->replacement_number);
				} while (r != nullptr);

				return *this;
			}

			std::string const & string() const
			{
				return fmtstring;
			}

		private:
			std::string fmtstring;
			replacement rep;

			replacement * next(int repl=0)
			{
				auto it = fmtstring.begin();
				char n;

				while (it != fmtstring.end()) {
					while (it != fmtstring.end() && (*it) != '%')
						it++;

					if (it == fmtstring.end())
						goto retnull;

					rep.start = it;

					if ((++it) == fmtstring.end())
						goto retnull;
					else if (*it == '%') // double %
						continue;


					if (IS_DIGIT(*it)) {
						n = CHAR_TO_INT(*it);
						it++;
						if (IS_DIGIT(*it)) {
							n = (n * 10) + CHAR_TO_INT(*it);
							it++;
						}
					} else {
						continue; // not a valid substitution
					}

					if (repl != 0 and n != repl)
						continue;
					
					rep.end = it;
					rep.replacement_number = n;
					return &rep;	
				}
retnull:
				return nullptr;
			}

			std::string & format(std::string &s, int length, char pad)
			{
				std::string padding;
				bool pad_left = false;

				if (s.length() >= length)
					return s;

				if (length < 0) {
					pad_left = true;
					length = length * -1;
				}

				padding.reserve(length);
				for (int i = 0; i < length; i++)
					padding += pad;

				if (pad_left)
					s = padding + s;
				else
					s = s + padding;
				return s;	
			}
	};

	class Log 
	{
		public:
			Log(level l)
			{
				loglevel = l;
			}


			template<typename... Ts>
			void debug(std::string const & fmt, Ts... params)
			{
				if (loglevel == DEBUG)
					log(fmt, params...);
			}
			
			template<typename... Ts>
			void info(std::string const & fmt, Ts... params)
			{
				if (loglevel <= INFO)
					log(fmt, params...);
			}

			template<typename... Ts>
			void warn(std::string const & fmt, Ts... params)
			{
				if (loglevel <= WARNING)
					log(fmt, params...);
			}

			template<typename... Ts>
			void warning(std::string const & fmt, Ts... params)
			{
				warn(fmt, params...);
			}

			template<typename... Ts>
			void error(std::string const & fmt, Ts... params)
			{
				if (loglevel <= ERROR)
					log(fmt, params...);
			}
			
			template<typename... Ts>
			void crit(std::string const & fmt, Ts... params)
			{
				if (loglevel <= CRITICAL)
					log(fmt, params...);
			}

			template<typename... Ts>
			void critical(std::string const & fmt, Ts... params)
			{
				crit(fmt, params...);
			}

		private:
			level loglevel;
			std::vector<std::string> args;

			/*
			 * All of this complete and utter insanity is because C++ now has
			 * the ability to have variadic function templates, BUT they don't
			 * give you any way to iterate over the fucking arguments. So you
			 * Have to do this stupid bullshit. Recursively calling the
			 * same(ish) function and 'popping' off the first argument is the
			 * only way to do something that resembles iterating the list. Since
			 * I actually need a list-like thing, I just push them onto a vector
			 * to call LogString.arg() on later.
			 */
			void collect_args()
			{
				args.shrink_to_fit();
			}
			
			template<typename...Us>
			void collect_args(std::string & first, Us... more)
			{
				args.push_back(first);
				collect_args(more...);
			}
			
			template<typename...Us>
			void collect_args(const char * first, Us... more)
			{
				std::string s = first;
				args.push_back(s);
				collect_args(more...);
			}

			template<typename T, typename... Us>
			void collect_args(T first, Us... more)
			{
				args.push_back(std::to_string(first));
				collect_args(more...);
			}

			template<typename... Ts>
			void log(std::string const & fmt, Ts... params)
			{
				LogString s = fmt;

				args.clear();
				collect_args(params...);

				for (auto i : args) 
					s.arg(i);

				output(s);
			}

			void output(LogString const &s) const
			{
				std::cerr << s.string() << std::endl;
			}
	};
};

#endif /* LOG_HH_ */

