#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "scanner.h"



static void hexdump(FILE *fl, void *buffer, int size)
{
	unsigned char *c = buffer;
	unsigned char ascii[17] = { 0, };
	int i;

	for (i = 0; i < size; i++) {
		if ((i % 16) == 0) {
			if (i > 0)
				fprintf(fl, " %s\n", ascii);
			memset(ascii, ' ', 16);
			fprintf(fl, "%08x  ", i);
		}
		fprintf(fl, "%02x ", c[i]);
		ascii[i % 16] = (c[i] >= 32 && c[i] <= 127) ? c[i] : '.';
	}
	while ((i++ % 16) != 0)
		fprintf(fl, "   ");
	fprintf(fl, " %s\n", ascii);
}



static void cdump(FILE *fl, void *buffer, int size)
{
	unsigned char *c = buffer;
	int i;
	int columns = 8;

	fprintf(fl, "unsigned char iso_template[] = {\n");
	for (i = 0; i < size; i++) {
		if ((i % columns) == 0)
			fprintf(fl, i == 0 ? "\t" : "\n\t");
		fprintf(fl, (i % columns) == 0 ? "0x%02x," : " 0x%02x,", c[i]);
	}
	fprintf(fl, "\n};\n");
	fprintf(fl, "int iso_template_size = %d;\n", size);
}



static void usage(const char *comm)
{
	fprintf(stderr, "Usage: %s [-h] [-x|-b|-c] [NAME]\n", comm);
	fprintf(stderr, "where:\n");
	fprintf(stderr, "\t-h\tusage syntax (this message)\n");
	fprintf(stderr, "\t-x\thexdump output (default)\n");
	fprintf(stderr, "\t-b\tbinary output\n");
	fprintf(stderr, "\t-c\tC structure output\n");
	fprintf(stderr, "\tNAME\t(optional) output file, stdout by default\n");
}



int main(int argc, char *argv[])
{
	int opt;
	FILE *fl = stdout;
	enum {
		hex,
		binary,
		c_struct
	} output = hex;
	int err;
	struct scanner_caps caps;
	int size;
	unsigned char *template;

	while ((opt = getopt(argc, argv, "xbch")) != -1) {
		switch (opt) {
		case 'x':
			output = hex;
			break;
		case 'b':
			output = binary;
			break;
		case 'c':
			output = c_struct;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (argc - optind == 1) {
		fl = fopen(argv[optind], "wb");
		if (!fl) {
			perror("Failed to open file");
			return 1;
		}
	} else if (argc - optind > 1) {
		usage(argv[0]);
		return 1;
	}

	err = scanner_on();
	if (err) {
		fprintf(stderr, "Failed to turn on scanner (%d)\n", err);
		return 1;
	}

	err = scanner_get_caps(&caps);
	if (err) {
		fprintf(stderr, "Failed to get capabilities (%d)\n", err);
		return 1;
	}

	if (!caps.iso_template) {
		fprintf(stderr, "Scanner provides no ISO templates!\n");
		return 1;
	}

	err = scanner_scan(-1);
	if (err == -1) {
		fprintf(stderr, "Timeout when scanning...\n");
		return 1;
	}
	if (err) {
		fprintf(stderr, "Error when scanning! (%d)\n", err);
		return 1;
	}

	size = scanner_get_iso_template(NULL, 0);
	if (size == 0) {
		fprintf(stderr, "No template size returned!\n");
		return 1;
	}
	if (size < 0) {
		fprintf(stderr, "Failed to obtain template size! (%d)\n",
				size);
		return 1;
	}

	template = malloc(size);
	if (!template) {
		fprintf(stderr, "Out of memory (needed %d bytes)!\n", size);
		return 1;
	}

	size = scanner_get_iso_template(template, size);
	if (size == 0) {
		fprintf(stderr, "No template returned!\n");
		return 1;
	}
	if (size < 0) {
		fprintf(stderr, "Failed to obtain template! (%d)\n", size);
		return 1;
	}

	switch (output) {
	case hex:
		hexdump(fl, template, size);
		break;
	case binary:
		size = fwrite(template, size, 1, fl);
		if (size != 1) {
			fprintf(stderr, "Failed to write!\n");
			return 1;
		}
		break;
	case c_struct:
		cdump(fl, template, size);
		break;
	}

	free(template);

	scanner_off();

	if (fl != stdout)
		fclose(fl);

	return 0;
}
