all: uip htest

CFLAGS = -Wall -g -I. # -fpack-struct # -Os

UIP_FILES = main.o tapdev.o clock-arch.o \
	uip.o uip_arp.o uiplib.o psock.o timer.o uip-neighbor.o

APP_FILES = httpd.o httpd-fs.o httpd-cgi.o

HOST_FILES = htest.o psock.o

httpd-fs.o: httpd-fsdata.c

httpd-fsdata.c: makefsdata
	./makefsdata

makefsdata: makefsdata.o
	$(CC) $(CFLAGS) -o makefsdata makefsdata.o

uip: $(UIP_FILES) $(APP_FILES)
	$(CC) $(CFLAGS) -o uip $(UIP_FILES) $(APP_FILES)

htest: $(HOST_FILES) $(APP_FILES)
	$(CC) $(CFLAGS) -o htest $(HOST_FILES) $(APP_FILES)

tags:
	etags *.[ch]

clean:
	rm -f *.o *~ .gitignore~ uip httpd-fsdata.c makefsdata


