#include <string.h>

#include "example.h"
#include "scanner.h"

static int on;

int scanner_on(void)
{
	if (on)
		return -1;

	on = 1;

	return 0;
}

void scanner_off(void)
{
	on = 0;
}

int scanner_get_caps(struct scanner_caps *caps)
{
	if (!on)
		return -1;

	caps->name = "Dummy";
	caps->image = 1;
	caps->iso_template = 1;
	caps->image_format = scanner_image_gray_8bit;
	caps->image_width = example_image_width;
	caps->image_height = example_image_height;

	return 0;
}

int scanner_scan(int timeout)
{
	if (!on)
		return -2;

	return 0;
}

int scanner_get_image(void *buffer, int size)
{
	if (!on)
		return -1;

	memcpy(buffer, example_image_gray_8bit, size);

	return example_image_gray_8bit_size;
}

int scanner_get_iso_template(void *buffer, int size)
{
	if (!on)
		return -1;

	memcpy(buffer, example_iso_template, size);

	return example_iso_template_size;
}
