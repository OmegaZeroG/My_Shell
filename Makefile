TARGET  = my_shell
SRCS    = main.c input_parser.c helpers.c builtins.c executor.c history.c signals.c jobs.c
OBJS    = $(SRCS:.c=.o)
CC      = gcc
CFLAGS  = -Wall -Wextra -Wshadow -Werror -g

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c my_shell.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(TARGET)

re: fclean all

.PHONY: all clean fclean re
