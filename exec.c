#include "exec.h"
#include "defs.h"
#include "freecmd.h"
#include "parsing.h"
#include "printstatus.h"
#include "types.h"
#include "utils.h"
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	// Your code here
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	int fd = open(file, flags, 0644);

	if (fd == -1) {
		perror("Error");
		return -1;
	}
	return fd;
}

void
execute_cmd(struct execcmd *exec_cmd)
{
	if (execvp(exec_cmd->argv[0], exec_cmd->argv) == -1) {
		bool is_command = exec_cmd->scmd[0] != '.';
		free_command((struct cmd *) exec_cmd);

		if (errno == ENOENT && is_command) {
			printf_debug("Error: Command not found\n", errno);
		} else {
			perror("Error");
		}
		exit(EXIT_FAILURE);
	}
}

// Executes the redirection from the old_fd to the new one pointing to the file
int
execute_redir(char *file, int old_fd, int flags)
{
	pid_t new_fd;

	if (strcmp(file, "&1") == 0) {
		new_fd = STDOUT_FILENO;
	} else if ((new_fd = open_redir_fd(file, flags)) == -1) {
		return -1;
	}
	if (dup2(new_fd, old_fd) == -1) {
		perror("Error");
		close(new_fd);
		return -1;
	}
	return 0;
}

void
redirect_fds(struct execcmd *redir_cmd)
{
	char *file;
	// Flags for output files (write only, create if not exist, truncate, close on exec).
	int out_flags = O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC;
	// Flags for input files (read only, close on exec).
	int in_flags = O_RDONLY | O_CLOEXEC;
	// Flags for error files (write only, create if not exist, close on exec).
	int err_flags = O_WRONLY | O_CREAT | O_CLOEXEC;

	// There is a redirection for stdout
	if (strlen(redir_cmd->out_file) > 0) {
		file = redir_cmd->out_file;
		if (execute_redir(file, STDOUT_FILENO, out_flags) == -1) {
			free_command((struct cmd *) redir_cmd);
			exit(EXIT_FAILURE);
		}
	}
	// There is a redirection for stdin
	if (strlen(redir_cmd->in_file) > 0) {
		file = redir_cmd->in_file;
		if (execute_redir(file, STDIN_FILENO, in_flags) == -1) {
			free_command((struct cmd *) redir_cmd);
			exit(EXIT_FAILURE);
		}
	}
	// There is a redirection for stderr
	if (strlen(redir_cmd->err_file) > 0) {
		file = redir_cmd->err_file;
		if (execute_redir(file, STDERR_FILENO, err_flags) == -1) {
			free_command((struct cmd *) redir_cmd);
			exit(EXIT_FAILURE);
		}
	}
}

int handle_pipes(struct pipecmd *pipe_cmd) {
	int fds[2];
	pipe(fds);

	pid_t pid_left = fork();

	if (pid_left == 0) {
		close(fds[READ]);
		dup2(fds[WRITE], STDOUT_FILENO);
		close(fds[WRITE]);
		struct cmd *left_cmd = pipe_cmd->leftcmd;
		
		free_command(pipe_cmd->rightcmd);
		free(pipe_cmd);

		exec_cmd(left_cmd);
	}

	struct cmd *new_right_cmd = parse_line(pipe_cmd->rightcmd->scmd);
	bool is_pipe = new_right_cmd->type == PIPE;

	pid_t pid_right = fork();

	if (pid_right == 0) {
		close(fds[WRITE]);
		dup2(fds[READ], STDIN_FILENO);
		close(fds[READ]);

		free_command((struct cmd *) pipe_cmd);

		if (!is_pipe){
			exec_cmd(new_right_cmd);
		} else {
			struct pipecmd* new_pipe = (struct pipecmd *) new_right_cmd;
			int exit_status = handle_pipes(new_pipe);
			free_command(new_right_cmd);
			exit(exit_status);
		}
	}

	close(fds[READ]);
	close(fds[WRITE]);
	free_command(new_right_cmd);

	int exit_status = EXIT_SUCCESS;
		
	if (waitpid(pid_left, &status, 0) != -1) {
		print_status_info(pipe_cmd->leftcmd);
		if (WEXITSTATUS(status) == 1) {
			exit_status = EXIT_FAILURE;
		}
	}

	if (waitpid(pid_right, &status, 0) != -1 ) {
		if (!is_pipe) {
			print_status_info(pipe_cmd->rightcmd);
		}
		if (WEXITSTATUS(status) == 1) {
			exit_status = EXIT_FAILURE;
		}
	}
	return exit_status;
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *exec_cmd;
	struct backcmd *back_cmd;
	struct execcmd *redir_cmd;
	struct pipecmd *pipe_cmd;

	switch (cmd->type) {
	case EXEC:
		// spawns a command
		//
		// Your code here
		exec_cmd = (struct execcmd *) cmd;
		execute_cmd(exec_cmd);
		break;

	case BACK: {
		// runs a command in background
		//
		// Your code here
		printf("Background process are not yet implemented\n");
		_exit(-1);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero
		redir_cmd = (struct execcmd *) cmd;
		redirect_fds(redir_cmd);
		execute_cmd(redir_cmd);
		break;
	}

	case PIPE: {
		// pipes two commands
		pipe_cmd = (struct pipecmd *) cmd;
		int exit_status = handle_pipes(pipe_cmd);

		// free the memory allocated
		// for the pipe tree structure
		free_command(parsed_pipe);

		exit(exit_status);
	}
	}
}
