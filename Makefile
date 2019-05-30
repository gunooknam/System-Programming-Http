# IPC SERVER Makefile
# Port Num 39998
# made by gunooknam

CC=gcc
EXEC=PS_server_39998
SRC=*.c
LIB=-lpthread

all :
	$(CC) -o $(EXEC) $(SRC) $(LIB)
clean :
	rm -rf $(EXEC)

