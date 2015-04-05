#include <stdio.h>

extern int dummy_init(void);

int scanner_init(void) {
	int err;

	err = dummy_init();
	if (err) {
		fprintf(stderr, "error: dummy_init() returned %d\n", err);
		return err;
	}

	return 0;
}
