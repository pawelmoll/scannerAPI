#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <png.h>

#include "scanner.h"



static int write_grey_8bit_png(FILE *fl, void *image, int width, int height)
{
	unsigned char *pixels = image;
	png_structp png;
	png_infop info;
	int row;

	png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
			NULL, NULL, NULL);
	if (!png)
		return -__LINE__;

	info = png_create_info_struct(png);
	if (!info)
		return -__LINE__;

	if (setjmp(png_jmpbuf(png))) {
		png_destroy_write_struct(&png, &info);
		return -__LINE__;
	}

	png_init_io(png, fl);

	png_set_IHDR(png, info, width, height,
			8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png, info);

	for (row = 0; row < height; row++)
		png_write_row(png, pixels + (width * row));

	png_write_end(png, NULL);

	png_destroy_write_struct(&png, &info);

	return 0;
}



static void usage(const char *comm)
{
	fprintf(stderr, "Usage: %s [-h|-l] -s SCANNER [-x|-b|-c] [NAME]\n", comm);
	fprintf(stderr, "Usage: %s [-h|-l] -s SCANNER [NAME]\n", comm);
	fprintf(stderr, "where:\n");
	fprintf(stderr, "\t-h\tusage syntax (this message)\n");
	fprintf(stderr, "\t-l\tprint list of available scanners\n");
	fprintf(stderr, "\t-s SCANNER\tname of a scanner to be used\n");
	fprintf(stderr, "\tNAME\t(optional) output file, stdout by default\n");
}

static void list(void)
{
	const char **list;
	int num, i;

	list = scanner_list(&num);
	fprintf(stderr, "Available scanners:\n");
	for (i = 0; i < num; i++)
		fprintf(stderr, "\t%s\n", list[i]);

	return;
}

int main(int argc, char *argv[])
{
	int opt;
	FILE *fl = stdout;
	int err;
	struct scanner *scanner = NULL;
	struct scanner_caps caps;
	int size;
	unsigned char *image;
	int i;

	err = scanner_init();
	if (err) {
		fprintf(stderr, "Failed to initialize scanner API (%d)\n", err);
		return 1;
	}

	while ((opt = getopt(argc, argv, "hls:")) != -1) {
		switch (opt) {
		case 'l':
			list();
			return 1;
		case 's':
			scanner = scanner_get(optarg);
			if (!scanner) {
				fprintf(stderr, "invalid scanner '%s'!\n",
						optarg);
				return 1;
			}
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (!scanner) {
		list();
		return 1;
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

	err = scanner_on(scanner);
	if (err) {
		fprintf(stderr, "Failed to turn on scanner (%d)\n", err);
		return 1;
	}

	err = scanner_get_caps(scanner, &caps);
	if (err) {
		fprintf(stderr, "Failed to get capabilities (%d)\n", err);
		return 1;
	}

	if (!caps.image) {
		fprintf(stderr, "Scanner provides no images!\n");
		return 1;
	}

	err = scanner_scan(scanner, -1);
	if (err == -1) {
		fprintf(stderr, "Timeout when scanning...\n");
		return 1;
	}
	if (err) {
		fprintf(stderr, "Error when scanning! (%d)\n", err);
		return 1;
	}

	size = scanner_get_image(scanner, NULL, 0);
	if (size == 0) {
		fprintf(stderr, "No image size returned!\n");
		return 1;
	}
	if (size < 0) {
		fprintf(stderr, "Failed to obtain image size! (%d)\n",
				size);
		return 1;
	}

	image = malloc(size);
	if (!image) {
		fprintf(stderr, "Out of memory (needed %d bytes)!\n", size);
		return 1;
	}

	size = scanner_get_image(scanner, image, size);
	if (size == 0) {
		fprintf(stderr, "No image returned!\n");
		return 1;
	}
	if (size < 0) {
		fprintf(stderr, "Failed to obtain image! (%d)\n", size);
		return 1;
	}

	switch(caps.image_format) {
	case scanner_image_gray_8bit_inversed:
		for (i = 0; i < size; i++)
			image[i] = 0xff - image[i];
		/* Fall through */
	case scanner_image_gray_8bit:
		err = write_grey_8bit_png(fl, image,
				caps.image_width, caps.image_height);
		break;
	default:
		fprintf(stderr, "Scanner providing unknown image format (%d)\n",
				caps.image_format);
		return 1;
	}
	if (err) {
		fprintf(stderr, "Failed to write image! (%d)\n", err);
		return 1;
	}

	scanner_off(scanner);

	free(image);

	if (fl != stdout)
		fclose(fl);

	return 0;
}
