CFLAGS := -ggdb3 -O2 -Wall -Wextra -std=gnu11
CFLAGS += -Wmissing-prototypes

EXEC := sh
SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=%.o)

all: clean $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)

no_canonical: CFLAGS += -DSHELL_NO_CANONICAL
no_canonical: clean $(EXEC)
	
valgrind: $(EXEC)
	valgrind --leak-check=full --show-leak-kinds=all ./$(EXEC)

format: .clang-files .clang-format
	xargs -r clang-format -i <$<

clean:
	rm -rf $(EXEC) *.o core vgcore.*

.PHONY: all no_canonical clean format valgrind 
