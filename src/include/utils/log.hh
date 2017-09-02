#ifndef LOG_HH_
#define LOG_HH_

#include <cstdarg>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>
#include <chrono> // std::chrono*
#include <iomanip> // std::put_time

#include <sys/types.h> // getpid getppid
#include <unistd.h> // getpid getppid

#define CHAR_TO_DIGIT(x) ((x) - '0')
#define IS_DIGIT(x) ((x) >= '0' && (x) <= '9')

namespace slog {
	typedef enum { DEBUG, INFO, WARNING, ERROR, CRITICAL, } level;
	typedef enum { 
		LOGLEVEL, 
		TIMESTAMP, 
		UTCTIMESTAMP, 
		PID, 
		PPID, 
		__MSG__,
	} format_spec;

	typedef struct {
		std::string::iterator start;
		std::string::iterator end;
		uint8_t replacement_number;
	} replacement;

	/*
	 * This is kind of like a very small much less functional version of Qt's
	 * QString.
	 */
	class LogString
	{
		public:
			template<typename...Us>
			LogString(std::string const &s, Us... params)
			{
				fmtstring = s;
				args(params...);
			}

			template<typename...Us>
			LogString(LogString const & s, Us... params)
			{
				fmtstring = s.string();
				args(params...);
			}

			template<typename...Us>
			LogString(char const * s, Us... params)
			{
				fmtstring = std::string(s);
				args(params...);
			}

			template<typename T>
			LogString arg(T i, int length=0, char pad=' ')
			{
				return arg(std::to_string(i), length, pad);
			}

			LogString arg(LogString const &i, int length=0, char pad=' ')
			{
				return arg(i.string(), length, pad);
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

			LogString append(std::string const & s)
			{
				fmtstring += s;
				return *this;
			}

			LogString append(LogString const & s)
			{
				return append(s.string());
			}

			std::string const & string() const
			{
				return fmtstring;
			}

			// public interface to private insanity below
			template<typename...Us>
			void args(Us... params)
			{
				collect_args(params...);
			}

		private:
			std::string fmtstring;
			replacement rep;

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

			/*
			 * This one gets called for most of the parameter types, It takes
			 * the first arg separately from the pack of the rest of them and
			 * then calls arg() on it. However, you can't call ::to_string on a
			 * string or a char*, so we have to define two other overrides to
			 * handle those.
			 */
			template<typename T, typename... Us>
			void collect_args(T first, Us... more)
			{
				arg(std::to_string(first));
				collect_args(more...);
			}

			/*
			 * This guy handles the situation where we've arrived at a string
			 * parameter type.
			 */
			template<typename...Us>
			void collect_args(std::string & first, Us... more)
			{
				arg(first);
				collect_args(more...);
			}
			
			/*
			 * Does the same as the last function except for char*
			 */
			template<typename...Us>
			void collect_args(char const * first, Us... more)
			{
				std::string s = first;
				arg(s);
				collect_args(more...);
			}

			/*
			 * And finally, we need this guy because c++ calls the function one
			 * last time with no args when the parameter pack is empty... I
			 * don't know why, it's all insane.
			 */
			void collect_args()
			{
				return;
			}

			// End Insanity

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
						n = CHAR_TO_DIGIT(*it);
						it++;
						if (IS_DIGIT(*it)) {
							n = (n * 10) + CHAR_TO_DIGIT(*it);
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
			Log(level l, bool ce=true, bool co=false) :
				loglevel(l), log_to_cerr(ce), log_to_cout(co), format("")
			{
				set_format("[%1] [%2:%3] %4", TIMESTAMP, LOGLEVEL, PID, __MSG__);
			}

			template<typename... Ts>
			void set_format(LogString const & s, Ts...params)
			{
				format = s; 
				format_items.clear();
				collect_formatter(params...);
			}

			template<typename T, typename...Us>
			void collect_formatter(T first, Us... params)
			{
				format_items.push_back(first);
				collect_formatter(params...);
			}

			/* 
			 * See insanity in LogString for description of why these
			 * collect_formatter templates exist, etc...
			 */
			void collect_formatter() { return; }

			/*
			 * Variadic function templates. See private stuff below.
			 */
			template<typename... Ts>
			void debug(LogString const & fmt, Ts... params)
			{
				if (loglevel == DEBUG)
					log(fmt, params...);
			}
			
			template<typename... Ts>
			void info(LogString const & fmt, Ts... params)
			{
				if (loglevel <= INFO)
					log(fmt, params...);
			}

			template<typename... Ts>
			void warn(LogString const & fmt, Ts... params)
			{
				if (loglevel <= WARNING)
					log(fmt, params...);
			}

			template<typename... Ts>
			void warning(LogString const & fmt, Ts... params)
			{
				warn(fmt, params...);
			}

			template<typename... Ts>
			void error(LogString const & fmt, Ts... params)
			{
				if (loglevel <= ERROR)
					log(fmt, params...);
			}
			
			template<typename... Ts>
			void crit(LogString const & fmt, Ts... params)
			{
				if (loglevel <= CRITICAL)
					log(fmt, params...);
			}

			template<typename... Ts>
			void critical(LogString const & fmt, Ts... params)
			{
				crit(fmt, params...);
			}

		private:
			bool log_to_cerr, log_to_cout;
			level loglevel;
			LogString format;
			std::vector<format_spec> format_items;

			template<typename... Ts>
			void log(LogString const & fmt, Ts... params)
			{
				LogString s = fmt;

				s.args(params...);

				output(s);
			}

			/* End Insanity */

			std::string const loglevel_to_string() const
			{
				std::string s;
				switch (loglevel) {
					case DEBUG:
						s = "DEBUG";
						break;
					case INFO:
						s = "INFO";
						break;
					case WARNING:
						s = "WARNING";
						break;
					case ERROR:
						s = "ERROR";
						break;
					case CRITICAL:
						s = "CRITICAL";
						break;
				}
				return s;
			}

			std::string const localtime() const
			{
				auto now = std::chrono::system_clock::now();
				std::time_t now_c = std::chrono::system_clock::to_time_t(now);
				std::stringstream ss;

				ss << std::put_time(std::localtime(&now_c), "%FT%T");
				return ss.str();
			}

			std::string const gmtime() const
			{
				auto now = std::chrono::system_clock::now();
				std::time_t now_c = std::chrono::system_clock::to_time_t(now);
				std::stringstream ss;

				ss << std::put_time(std::gmtime(&now_c), "%FT%Tz");
				return ss.str();
			}

			std::string const full_log_string(LogString const & s) const
			{
				LogString out = format;
				bool msg_added = false;

				for (auto i : format_items) {
					switch (i) {
						case LOGLEVEL:
							out.arg(loglevel_to_string());
							break;
						case TIMESTAMP:
							out.arg(localtime());
							break;
						case UTCTIMESTAMP:
							out.arg(gmtime());
							break;
						case PID:
							out.arg(getpid());
							break;
						case PPID:
							out.arg(getppid());
							break;
						case __MSG__:
							out.arg(s);
							msg_added = true;
							break;
					}
				}

				if (!msg_added)
					out.append(" " + s.string());
				return out.string();
			}

			void output(LogString const & s) const
			{
				std::string msg = full_log_string(s);

				if (log_to_cerr)
					std::cerr << msg << std::endl;

				if (log_to_cout)
					std::cout << msg << std::endl;
			}
	};
};

#endif /* LOG_HH_ */
