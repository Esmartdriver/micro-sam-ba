# CFLAGS=-std=gnu99 -Wall

# BINARY=usamba
# SOURCES = sources/usamba.c sources/comm.c sources/chipid.c sources/eefc.c
# OBJS = $(SOURCES:.c=.o)

# $(BINARY): $(OBJS)

# clean:
# 	@rm -f $(OBJS) $(BINARY)

# binaries will be generated with this name (.elf, .bin, .hex, etc)

# Put your source files here (or *.c, etc)
SRCS =  ./src/usamba.c
SRCS += ./src/comm.c
SRCS += ./src/chipid.c
SRCS += ./src/eefc.c


CFLAGS=-std=gnu99 -Wall -O2 -Wfatal-errors
CFLAGS += -I./src


BINARY=usamba
OBJS = $(SRCS:.c=.o)

./src/$(BINARY): $(OBJS)


clean:
	@rm -f $(OBJS) $(BINARY)

