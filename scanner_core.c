#include <string.h>
#include "scanner_driver.h"

#define SCANNERS_MAX 8

struct scanner {
	struct scanner_ops *ops;
	int in_use;
};

static const char *scanner_names[SCANNERS_MAX];
static struct scanner scanners[SCANNERS_MAX];
static int scanners_number;

const char **scanner_list(int *number)
{
	*number = scanners_number;

	return scanner_names;

}

int scanner_register(const char *name, struct scanner_ops *ops)
{
	if (scanners_number == SCANNERS_MAX)
		return -1;

	scanner_names[scanners_number] = name;
	scanners[scanners_number].ops = ops;

	scanners_number++;

	return 0;
}

struct scanner *scanner_get(const char *name)
{
	int i;

	for (i = 0; i < scanners_number; i++) {
		if (!scanners[i].in_use &&
				strcmp(name, scanner_names[i]) == 0) {
			scanners[i].in_use = 1;
			return &scanners[i];
		}
	}

	return NULL;
}

void scanner_put(struct scanner *scanner)
{
	scanner->in_use = 0;
}

int scanner_on(struct scanner *scanner)
{
	return scanner->ops->on();
}

void scanner_off(struct scanner *scanner)
{
	scanner->ops->off();
}

int scanner_get_caps(struct scanner *scanner, struct scanner_caps *caps)
{
	return scanner->ops->get_caps(caps);
}

int scanner_scan(struct scanner *scanner, int timeout)
{
	return scanner->ops->scan(timeout);
}

int scanner_get_image(struct scanner *scanner, void *buffer, int size)
{
	return scanner->ops->get_image(buffer, size);
}

int scanner_get_iso_template(struct scanner *scanner, void *buffer, int size)
{
	return scanner->ops->get_iso_template(buffer, size);
}
