#include "builtin.h"
#include "defs.h"
#include <linux/limits.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	if (strcmp(cmd, "exit") == 0)
		return true;
	return false;
}

static void
change_dir(char *path)
{
	char buf[BUFLEN] = { 0 };

	if (chdir(path) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", path);
		perror(buf);
	} else {
		char path[PRMTLEN] = { 0 };
		getcwd(path, sizeof(path));
		snprintf(prompt, sizeof(prompt), "%s", path);
	}
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	char *cmd_aux = strdup(cmd);
	char *token = strtok(cmd_aux, " ");
	char *new_path = getenv("HOME");

	if (strcmp(token, "cd") == 0) {
		token = strtok(NULL, " ");

		if (token) {
			new_path = token;
		}
		change_dir(new_path);

		free(cmd_aux);
		return true;
	}

	free(cmd_aux);
	return false;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	char path_buff[PATH_MAX] = { END_STRING };
	if (strcmp(cmd, "pwd") == 0) {
		getcwd(path_buff, PATH_MAX);
		printf("%s\n", path_buff);

		return true;
	}
	return false;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here

	return 0;
}
