#include <stdio.h>


#include "uip.h"

#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


struct uip_conn *uip_conn; /* current connection */
struct uip_conn uip_conns[UIP_CONNS];
void *uip_appdata;
u8_t uip_flags;
struct uip_stats uip_stat;

struct connection *uip_cp;


void
dump (void const *buf, int n)
{
	int i;
	int j;
	int c;

	for (i = 0; i < n; i += 16) {
		printf ("%04x: ", i);
		for (j = 0; j < 16; j++) {
			if (i+j < n)
				printf ("%02x ", ((unsigned char *)buf)[i+j]);
			else
				printf ("   ");
		}
		printf ("  ");
		for (j = 0; j < 16; j++) {
			if (i+j >= n)
				break;

			c = ((unsigned char *)buf)[i+j] & 0x7f;
			if (c < ' ' || c == 0x7f)
				putchar ('.');
			else
				putchar (c);
		}
		printf ("\n");

	}
}


struct listen_sock {
	struct listen_sock *next;
	int sock;
};

struct listen_sock *listen_socks;

struct connection {
	struct connection *next;
	struct connection *prev;
	int seq;
	int sock;
	struct sockaddr_in raddr;
	int dead;

	struct uip_conn uip_conn;
};
int conn_seq;

struct connection connections;

void accept_connection (struct listen_sock *lp);
void process_input (struct connection *cp);

void
usage (void)
{
	fprintf (stderr, "usage: htest\n");
	exit (1);
}

int
main (int argc, char **argv)
{
	int port;
	fd_set rset;
	int maxfd;
	struct listen_sock *lp;
	struct timeval tv;
	struct connection *cp;

	connections.next = &connections;
	connections.prev = &connections;

	for (port = 8000; port < 8100; port++) {
		if (httpd_init(port) >= 0) {
			printf ("listing on port %d\n", port);
			break;
		}
	}

	while (1) {
		FD_ZERO (&rset);

		maxfd = 0;
		for (lp = listen_socks; lp; lp = lp->next) {
			FD_SET (lp->sock, &rset);
			if (lp->sock > maxfd)
				maxfd = lp->sock;
		}

		for (cp = connections.next; cp != &connections; cp = cp->next) {
			if (cp->dead)
				continue;

			FD_SET (cp->sock, &rset);
			if (cp->sock > maxfd)
				maxfd = cp->sock;
		}

		tv.tv_sec = 0;
		tv.tv_sec = 100 * 1000;

		select (maxfd + 1, &rset, NULL, NULL, &tv);

		for (lp = listen_socks; lp; lp = lp->next) {
			if (FD_ISSET (lp->sock, &rset)) {
				accept_connection (lp);
			}
		}

		for (cp = connections.next; cp != &connections; cp = cp->next) {
			if (FD_ISSET (cp->sock, &rset)) {
				process_input (cp);
			}
		}
	}

	exit (0);
}

int
uip_listen(u16_t port_network_order)
{
	int sock;
	struct sockaddr_in addr;
	struct listen_sock *lp;

	if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf (stderr, "can't create listen socket\n");
		exit (1);
	}

	memset (&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = port_network_order;

	if (bind (sock, (struct sockaddr *)&addr, sizeof addr) < 0) {
		fprintf (stderr, "can't bind to port %d\n", 
			 ntohs (port_network_order));
		close (sock);
		return (-1);
	}

	listen (sock, 5);

	lp = calloc (1, sizeof *lp);
	lp->sock = sock;
	lp->next = listen_socks;
	listen_socks = lp;

	return (0);
}

u16_t uip_len;

void
uip_send(const void *data, int len)
{
	printf ("uip_send\n");
	dump (data, len);
	write (uip_cp->sock, data, len);
}

void
accept_connection (struct listen_sock *lp)
{
	struct connection *cp;
	struct sockaddr_in raddr;
	size_t rlen;
	int sock;

	rlen = sizeof raddr;
	sock = accept (lp->sock, (struct sockaddr *)&raddr, &rlen);
	if (sock < 0) {
		printf ("error accepting connection\n");
		return;
	}
	
	cp = calloc (1, sizeof *cp);
	cp->seq = ++conn_seq;
	cp->sock = sock;
	cp->raddr = raddr;

	fcntl (cp->sock, F_SETFL, O_NONBLOCK);

	printf ("start conn%d %s:%d\n",
		cp->seq,
		inet_ntoa (cp->raddr.sin_addr), ntohs (cp->raddr.sin_port));
	
	uip_conn = &cp->uip_conn;
	uip_conn->mss = 1024;
	
	uip_flags = UIP_CONNECTED;
	httpd_appcall ();


	cp->next = &connections;
	cp->prev = connections.prev;
	connections.prev->next = cp;
	connections.prev = cp;
}

void
process_input (struct connection *cp)
{
	char buf[10000];
	int n;

	n = read (cp->sock, buf, sizeof buf);

	if (n == 0) {
		printf ("close conn%d\n", cp->seq);
		cp->dead = 1;
		return;
	}

	if (n < 0 && errno != EWOULDBLOCK ) {
		printf ("error conn%d\n", cp->seq);
		cp->dead = 1;
		return;
	}

	printf ("input %d\n", n);
	dump (buf, n);

	uip_appdata = buf;
	uip_len = n;

	uip_flags = UIP_NEWDATA;
	uip_conn = &cp->uip_conn;
	uip_cp = cp;
	httpd_appcall ();
}
