CFLAGS := -O3 -Wall -Wextra
LIBS := -lpthread
OBJS = main.o server.o client.o config.o misc.o ident.o

all: ebbnc

ebbnc: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o ebbnc $(LIBS)

clean:
	@rm -f *.o ebbnc
	
