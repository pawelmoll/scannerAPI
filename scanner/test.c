#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "scanner.h"

static int test(const char *name)
{
	int err;
	struct scanner *scanner = NULL;
	struct scanner_caps caps;

	printf("Getting scanner '%s'...\n", name);
	scanner = scanner_get(name);
	assert(scanner);
	if (!scanner)
		return 1;

	printf("Turning on scanner...\n");
	err = scanner_on(scanner);
	assert(!err);
	if (err)
		return 1;

	printf("Getting scanner's capabilities...\n");
	err = scanner_get_caps(scanner, &caps);
	assert(!err);
	if (err)
		return 1;

	printf("Scanning... Give the scanner a finger now! ;-)\n");
	err = scanner_scan(scanner, -1);
	assert(!err);
	if (err)
		return 1;

	if (caps.image) {
		const char *format;
		int bytes_per_pixel;
		int size, size2;
		unsigned char *pattern;
		unsigned char *image;
		int i;

		switch (caps.image_format) {
		case scanner_image_gray_8bit:
			format = "8-bit gray scale";
			bytes_per_pixel = 1;
			break;
		case scanner_image_gray_8bit_inversed:
			format = "8-bit inversed gray scale";
			bytes_per_pixel = 1;
			break;
		default: /* Unknown format */
			assert(0);
			return 1;
		}

		printf("Provides %s image %dx%d pixels...\n",
				format, caps.image_width, caps.image_height);
		assert(caps.image_width > 0);
		assert(caps.image_height > 0);

		printf("Getting image size...\n");
		size = scanner_get_image(scanner, NULL, 0);

		printf("Image is %d bytes big...\n", size);
		assert(size > 0);
		if (size < 0)
			return 1;
		assert(size == caps.image_width * caps.image_height *
				bytes_per_pixel);

		pattern = malloc(size);
		assert(pattern);
		image = malloc(size);
		assert(image);
		if (!pattern || !image)
			return 1;
		for (i = 0; i < size; i++)
			pattern[i] = i & 0xff;

		printf("Getting half of the image...\n");
		memcpy(image, pattern, size);
		size2 = scanner_get_image(scanner, image, size / 2);
		assert(size2 > 0);
		if (size2 < 0)
			return 1;
		assert(size2 == size);

		/* Check if first half of the image buffer has been changed */
		assert(memcmp(image, pattern, size / 2) != 0);

		/* Check if the rest of image buffer has not been changed */
		assert(memcmp(image + size / 2, pattern + size / 2,
					size - size / 2) == 0);

		printf("Getting the whole image...\n");
		memcpy(image, pattern, size);
		size2 = scanner_get_image(scanner, image, size);
		assert(size2 > 0);
		if (size2 < 0)
			return 1;
		assert(size2 == size);

		assert(memcmp(image, pattern, size) != 0);

		free(pattern);
		free(image);
	}

	if (caps.iso_template) {
		int size, size2;
		unsigned char *pattern;
		unsigned char *template;
		int i;

		printf("Getting ISO template size...\n");
		size = scanner_get_iso_template(scanner, NULL, 0);

		printf("Template is %d bytes big...\n", size);
		assert(size > 0);

		pattern = malloc(size);
		assert(pattern);
		template = malloc(size);
		assert(template);
		if (!pattern || !template)
			return 1;
		for (i = 0; i < size; i++)
			pattern[i] = i & 0xff;

		printf("Getting half of the template...\n");
		memcpy(template, pattern, size);
		size2 = scanner_get_iso_template(scanner, template, size / 2);
		assert(size2 > 0);
		if (size2 < 0)
			return 1;
		assert(size2 == size);

		/* Check if first half of the template has been changed */
		assert(memcmp(template, pattern, size / 2) != 0);

		/* Check if the rest of the template has not been changed */
		assert(memcmp(template + size / 2, pattern + size / 2,
					size - size / 2) == 0);

		printf("Getting the whole template...\n");
		memcpy(template, pattern, size);
		size2 = scanner_get_iso_template(scanner, template, size);
		assert(size2 > 0);
		if (size2 < 0)
			return 1;
		assert(size2 == size);

		assert(memcmp(template, pattern, size) != 0);

		free(pattern);
		free(template);
	}

	printf("Turning the scanner off...\n");

	scanner_off(scanner);

	printf("Putting scanner...\n");
	scanner_put(scanner);

	printf("Done.\n");

	return 0;
}



static void usage(const char *comm)
{
	fprintf(stderr, "Usage: %s [-h|-l] SCANNER\n", comm);
	fprintf(stderr, "where:\n");
	fprintf(stderr, "\t-h\tusage syntax (this message)\n");
	fprintf(stderr, "\t-l\tprint list of available scanners\n");
	fprintf(stderr, "\tSCANNER\tname of a scanner to be used\n");
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
	int err;
	int opt;

	err = scanner_init();
	assert(!err);
	if (err)
		return 1;

	while ((opt = getopt(argc, argv, "hl")) != -1) {
		switch (opt) {
		case 'l':
			list();
			return 1;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (argc - optind != 1) {
		usage(argv[0]);
		return 1;
	}

	test(argv[optind]);

	return 0;
}
