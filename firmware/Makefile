all: uip

CFLAGS = -Wall -g -I. # -fpack-struct # -Os

OBJ_FILES = main.o tapdev.o clock-arch.o \
	uip.o uip_arp.o uiplib.o psock.o timer.o uip-neighbor.o \
	httpd.o httpd-fs.o httpd-cgi.o

httpd-fs.o: httpd-fsdata.c

httpd-fsdata.c: makefsdata
	./makefsdata

makefsdata: makefsdata.o
	$(CC) $(CFLAGS) -o makefsdata makefsdata.o

uip: $(OBJ_FILES)
	$(CC) $(CFLAGS) -o uip $(OBJ_FILES)

clean:
	rm -f *.o *~ .gitignore~ uip httpd-fsdata.c makefsdata


