
CFLAGS := -O3 -Wall -Wextra -g -ggdb -Wfatal-errors
LIBS := -lpthread
OBJS = main.o server.o client.o config.o misc.o ident.o xtea.o hex.o

ifeq ($(wildcard conf.h),)
$(shell echo "#undef CONF_EMBEDDED" > conf.h)
endif

all: ebbnc
ebbnc: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o ebbnc $(LIBS)

conf: makeconf.o config.o misc.o hex.o xtea.o
	$(CC) $(CFLAGS) makeconf.o config.o misc.o hex.o xtea.o -o makeconf
	@./makeconf
  
%.o: %.c
	$(CC) -c $(CFLAGS) -MD -o $@ $<

-include $(OBJS:.o=.d)

clean:
	@rm -f *.o *.d ebbnc conf.h
	
