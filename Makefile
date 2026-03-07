TARGET = my_shell.exe
OBJ = main.c input_parser.c
CC = gcc

all:
	$(CC) -o $(TARGET) $(OBJ)
clean:
	rm -f *-o
fclean: clean
	rm -f $(TARGET)
re: fclean all