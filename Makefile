CFLAGS := -O3 -Wall -Wextra -pedantic
LIBS := -lpthread
EBBNC_OBJS := main.o server.o client.o config.o misc.o ident.o xtea.o hex.o
CONF_OBJS := makeconf.o config.o misc.o hex.o xtea.o

ifeq ($(wildcard conf.h),)
$(shell echo "#undef CONF_EMBEDDED" > conf.h)
CONF_EMBEDDED := false
else
CONF_EMBEDDED := true
endif

all: ebbnc
ebbnc: $(EBBNC_OBJS)
	$(CC) $(CFLAGS) $(EBBNC_OBJS) -o ebbnc $(LIBS)
	@echo "------------------------------------------------------- --- -> >"
	@echo "ebBNC by ebftpd team built successfully"
	@echo "------------------------------------------------------- --- -> >"
	@if [ "$(CONF_EMBEDDED)" = "true" ]; then \
		echo "Run './ebbnc' to start the bouncer."; \
	else \
		echo "Edit ebbnc.conf to suit your needs, "; \
    echo "then run './ebbnc ebbnc.conf' to start the bouncer."; \
	fi
	@echo "------------------------------------------------------- --- -> >"

conf: $(CONF_OBJS)
	$(CC) $(CFLAGS) $(CONF_OBJS) -o makeconf
	@./makeconf

%.o: %.c
	$(CC) -c $(CFLAGS) -MD -o $@ $<

-include $(EBBNC_OBJS:.o=.d)
-include $(CONF_OBJS:.o=.d)

clean:
	@rm -f *.o *.d ebbnc conf.h
	
