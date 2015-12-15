.PHONY:clean

CC = gcc
RM = rm
RMFLAG = -rf
CFLAG = -Wall -lpthread -g

EXE = demostorage.exe
SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

$(EXE):$(OBJS)
	$(CC) $^ -o $@
%.o:%.c
	$(CC) -c $^ -o $@ $(CFLAG)

clean:
	$(RM) $(RMFLAG) $(EXE) $(OBJS)