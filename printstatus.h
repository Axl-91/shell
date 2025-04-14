#ifndef PRINTSTATUS_H
#define PRINTSTATUS_H

#include "defs.h"
#include "types.h"

extern int status;

void print_status_info(struct cmd *cmd);

void print_back_info(struct cmd *back);

void print_back_return(int pid);

#endif  // PRINTSTATUS_H
