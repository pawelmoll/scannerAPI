#include <string.h>

#include "example.h"
#include "driver.h"

#define DUMMY_NAME "Dummy"

static int on;

static int dummy_on(void)
{
	if (on)
		return -1;

	on = 1;

	return 0;
}

static void dummy_off(void)
{
	on = 0;
}

static int dummy_get_caps(struct scanner_caps *caps)
{
	if (!on)
		return -1;

	caps->name = DUMMY_NAME;
	caps->image = 1;
	caps->iso_template = 1;
	caps->image_format = scanner_image_gray_8bit;
	caps->image_width = example_image_width;
	caps->image_height = example_image_height;

	return 0;
}

static int dummy_scan(int timeout)
{
	if (!on)
		return -2;

	return 0;
}

static int dummy_get_image(void *buffer, int size)
{
	if (!on)
		return -1;

	memcpy(buffer, example_image_gray_8bit, size);

	return example_image_gray_8bit_size;
}

static int dummy_get_iso_template(void *buffer, int size)
{
	if (!on)
		return -1;

	memcpy(buffer, example_iso_template, size);

	return example_iso_template_size;
}

static struct scanner_ops dummy_ops = {
	.on = dummy_on,
	.off = dummy_off,
	.get_caps = dummy_get_caps,
	.scan = dummy_scan,
	.get_image = dummy_get_image,
	.get_iso_template = dummy_get_iso_template,
};

int dummy_init(void)
{
	return scanner_register(DUMMY_NAME, &dummy_ops);
}
__scanner_init(dummy_init);
