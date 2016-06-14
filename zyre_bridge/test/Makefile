	
# define the C compiler to use
CC = gcc

# define any compile-time flags
CFLAGS = -Wall -ggdb

# define any directories containing header files other than /usr/include
INCLUDES = -I../include -I/opt/zeromq-4.1.2/include -I/opt/czmq-3.0.2/include -I/opt/zyre-1.1.0/include

# define library paths in addition to /usr/lib
LFLAGS =  -L../lib -L/opt/zeromq-4.1.2/lib -L/opt/czmq-3.0.2/lib -L/opt/zyre-1.1.0/lib

# define any libraries to link into executable:
LIBS = -lczmq -lzmq -lzyre -ljansson -luuid

# define the C source files
SRCS = test_send_msg.c

# define the C object files 
OBJS = $(SRCS:.c=.o)

# define the executable file 
MAIN = test_send_msg 


.PHONY: depend clean

all: test_send_msg
	@echo all examples have been compiled

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .o's from .c's
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it
