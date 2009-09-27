#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

void do_file (char *fname);

struct file {
	struct file *next;
	struct file *prev;
	char *name;
	char *idname;
	char *data;
	int datalen;
};

struct file files;
int nfiles;


FILE *outf;

#define FS_DIR "httpd-fs"

int
main (int argc, char **argv)
{
	DIR *dir;
	struct dirent *dp;
	int len;
	struct file *fp;
	char *p;
	int i;

	files.next = &files;
	files.prev = &files;

	if ((dir = opendir (FS_DIR)) == NULL) {
		fprintf (stderr, "can't open %s\n", FS_DIR);
		exit (1);
	}

	while ((dp = readdir (dir)) != NULL) {
		len = strlen (dp->d_name);

		if (len == 0 || dp->d_name[0] == '.')
			continue;

		if (dp->d_name[len-1] == '~')
			continue;

		do_file (dp->d_name);
	}

	if ((outf = fopen ("httpd-fsdata.c.new", "w")) == NULL) {
		fprintf (stderr, "can't create output\n");
		exit (1);
	}

	for (fp = files.next; fp != &files; fp = fp->next) {
		fprintf (outf, "static const unsigned char data_%s[] = {\n",
			 fp->idname);

		fprintf (outf, "\t/* %s */\n", fp->name);
		fprintf (outf, "\t0x%x", '/');
		for (p = fp->name; *p; p++)
			fprintf (outf, ",0x%x", *p & 0xff);
		fprintf (outf, ",0\n");

		for (i = 0; i < fp->datalen; i++)
			fprintf (outf, ",0x%x", fp->data[i] & 0xff);
		fprintf (outf, ",0\n");
		fprintf (outf, "};\n\n");
	}


	for (fp = files.next; fp != &files; fp = fp->next) {
		fprintf (outf, "const struct httpd_fsdata_file file_%s[] = ",
			 fp->idname);
		fprintf (outf, "{{");
		if (fp->prev == &files)
			fprintf (outf, "NULL");
		else
			fprintf (outf, "file_%s", fp->prev->idname);
		fprintf (outf, ", data_%s", fp->idname);
		fprintf (outf, ", data_%s + %d",
			 fp->idname, strlen (fp->name) + 2);
		fprintf (outf, ", %d", fp->datalen);
		fprintf (outf, "}};\n");
	}

	fprintf (outf, "#define HTTPD_FS_ROOT file_%s\n",
		 files.prev->idname);
	fprintf (outf, "#define HTTPD_FS_NUMFILES %d\n", nfiles);

	fclose (outf);
	rename ("httpd-fsdata.c.new", "httpd-fsdata.c");

	return (0);
}

void
do_file (char *fname)
{
	char fullname[1000];
	struct file *fp;
	FILE *inf;
	char *p;

	nfiles++;
	fp = calloc (1, sizeof *fp);
	fp->next = &files;
	fp->prev = files.prev;
	files.prev->next = fp;
	files.prev = fp;

	fp->name = strdup (fname);

	fp->idname = strdup (fname);
	for (p = fp->idname; *p; p++) {
		if (! isalnum (*p))
			*p = '_';
	}
	
	sprintf (fullname, "%s/%s", FS_DIR, fname);
	if ((inf = fopen (fullname, "r")) == NULL) {
		fprintf (stderr, "can't open %s\n", fullname);
		exit (1);
	}

	fseek (inf, 0, SEEK_END);
	fp->datalen = ftell (inf);
	fseek (inf, 0, SEEK_SET);

	
	fp->data = malloc (fp->datalen + 1);
	fread (fp->data, 1, fp->datalen, inf);
	fp->data[fp->datalen] = 0;

	fclose (inf);
}
