#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"

int main(void)
{
	int err;
	struct scanner_caps caps;

	printf("Turning on scanner...\n");
	err = scanner_on();
	assert(!err);
	if (err)
		return 1;

	printf("Getting scanner's capabilities...\n");
	err = scanner_get_caps(&caps);
	assert(!err);
	if (err)
		return 1;

	printf("Running scanner '%s'...\n", caps.name);
	err = scanner_scan(-1);
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
		size = scanner_get_image(NULL, 0);

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
		size2 = scanner_get_image(image, size / 2);
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
		size2 = scanner_get_image(image, size);
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
		size = scanner_get_iso_template(NULL, 0);

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
		size2 = scanner_get_iso_template(template, size / 2);
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
		size2 = scanner_get_iso_template(template, size);
		assert(size2 > 0);
		if (size2 < 0)
			return 1;
		assert(size2 == size);

		assert(memcmp(template, pattern, size) != 0);

		free(pattern);
		free(template);
	}

	printf("Turning the scanner off...\n");

	scanner_off();

	printf("Done.\n");

	return 0;
}
