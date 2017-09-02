#ifndef PASSWORD_HH_
#define PASSWORD_HH_

#include <stdexcept>
#include "utils/secure.hh"

namespace hush {
	namespace utils {
		class PasswordException : public std::runtime_error
		{
			using std::runtime_error::runtime_error;
			using std::runtime_error::what;
		};

		class Password
		{
		public:
			void ask(char const *prompt, bool confirm=false, bool show_asterisk=true);
			hush::secure::string get(void) { return password; }
		
		private:
			int getpwchar(void);
			hush::secure::string obtain(char const *prompt, bool show_asterisk=true);
			hush::secure::string password;
		};
	};
};

#endif /* PASSWORD_HH_ */

