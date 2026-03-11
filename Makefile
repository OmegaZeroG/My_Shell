TARGET = my_shell
OBJ = main.c input_parser.c helpers.c builtins.c executor.c history.c
CC = gcc

all:
	$(CC) -o $(TARGET) $(OBJ)
clean:
	rm -f *-o
fclean: clean
	rm -f $(TARGET)
re: fclean all