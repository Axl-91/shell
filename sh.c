#include "defs.h"
#include "printstatus.h"
#include "readline.h"
#include "runcmd.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

char prompt[PRMTLEN] = { 0 };

static void
signal_handler(int signo)
{
	(void) signo;
	pid_t pid;

	while ((pid = waitpid(0, &status, WNOHANG)) > 0) {
		char str[1024] = { 0 };
		snprintf(str, sizeof(str), "\nENDED ==> PID: %d\n", pid);
		write(STDOUT_FILENO, str, strlen(str));
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
		perror("sigaction");
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

int
main(void)
{
	init_shell();

	run_shell();

	return 0;
}
