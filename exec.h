#ifndef EXEC_H
#define EXEC_H

#include "defs.h"
#include "types.h"
#include "utils.h"
#include "freecmd.h"

extern struct cmd *parsed_pipe;

void exec_cmd(struct cmd *c);
void execute_cmd(struct execcmd *exec_cmd);
int handle_pipes(struct pipecmd *pipe_cmd);
int execute_redir(char *file, int old_fd, int flags);
void redirect_fds(struct execcmd *redir_cmd);

#endif  // EXEC_H
