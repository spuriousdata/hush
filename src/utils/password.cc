#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include "utils/password.hh"
#include "secure.hh"

using namespace hush::utils;

/*
 * I took this from cplusplus.com. I feel like it's kinda shitty to fuck with
 * the terminal before and after ever character read rather than once in the
 * beginning and then once at the end. I'm leaving it this way for now, though.
 */
int Password::getpwchar(void) {
	int ch;
	struct termios t_old, t_new;

	tcgetattr(STDIN_FILENO, &t_old);
	t_new = t_old;
	t_new.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
	return ch;
}

void Password::ask(const char *prompt, bool confirm, bool show_asterisk)
{
	password = obtain(prompt, show_asterisk);
	if (confirm && (password != obtain("Confirm: ", show_asterisk)))
		throw PasswordException("Passwords don't match!");
}

hush::secure::string Password::obtain(const char *prompt, bool show_asterisk)
{
	const char BACKSPACE=0x7F;
	const char RETURN=0x0A;
	unsigned char ch=0;
	hush::secure::string input;

	std::cout << prompt;

	while ((ch = getpwchar()) != RETURN) {
		if (ch == BACKSPACE) {
			if (input.length()!=0) {
				 if(show_asterisk)
					 std::cout <<"\b \b";
				 input.resize(input.length()-1);
			  }
		 } else {
			 input += ch;
			 if(show_asterisk)
				 std::cout << '*';
		 }
	}
	std::cout << std::endl;
	return input;
}
