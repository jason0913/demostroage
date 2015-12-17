.PHONY:clean

CC = gcc
RM = rm
RMFLAG = -rf
CFLAG = -Wall -DOS_LINUX -D__DEBUG__ -lpthread -g

EXE = demostorage.exe
SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

$(EXE):$(OBJS)
	$(CC) $^ -o $@ $(CFLAG)
%.o:%.c
	$(CC) -c $^ -o $@ $(CFLAG)

clean:
	$(RM) $(RMFLAG) $(EXE) $(OBJS)