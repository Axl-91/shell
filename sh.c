#include "defs.h"
#include "printstatus.h"
#include "readline.h"
#include "runcmd.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <termios.h>

struct termios saved_attributes;

char prompt[PRMTLEN] = { 0 };

static void
signal_handler(int signo)
{
	(void) signo;
	pid_t pid;

	while ((pid = waitpid(0, &status, WNOHANG)) > 0) {
		print_back_return(pid);
	}
}

// runs a shell command
static void
run_shell()
{
	char *cmd;

	struct sigaction sa;

	sa.sa_handler = signal_handler;
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("Sigaction Error");
		exit(1);
	}

	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "%s", home);
	}
}

static void
reset_input_mode()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}


static void
set_input_mode()
{
#ifdef SHELL_NO_CANONICAL
	struct termios tattr;

	if (!isatty(STDIN_FILENO)) {
		fprintf(stderr, "Not a terminal.\n");
		exit(EXIT_FAILURE);
	}

	// Save the terminal attributes so we can restore them later
	tcgetattr(STDIN_FILENO, &saved_attributes);
	atexit(reset_input_mode);

	tcgetattr(STDIN_FILENO, &tattr);
	tattr.c_lflag &= ~(ICANON | ECHO);
	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
#endif
}

int
main(void)
{
	set_input_mode();

	init_shell();

	run_shell();

	return 0;
}
