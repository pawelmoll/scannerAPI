#ifndef __SCANNER_DRIVER_H
#define __SCANNER_DRIVER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "scanner.h"

/**
 * struct scanner_ops - scanner driver operations
 *
 * Pointers to functions implementing scanner API. See scanner.h for details.
 *
 * @on:			scanner_on() implementation
 * @off:		scanner_off() implementation
 * @get_caps:		scanner_get_caps() implementation
 * @scan:		scanner_scan() implementation
 * @get_image:		scanner_get_image() implementation
 * @get_iso_template:	scanner_get_iso_template() implementation
 */
struct scanner_ops {
	int (*on)(void);
	void (*off)(void);
	int (*get_caps)(struct scanner_caps *caps);
	int (*scan)(int timeout);
	int (*get_image)(void *buffer, int size);
	int (*get_iso_template)(void *buffer, int size);
};

/**
 * scanner_register - register a scanner driver
 *
 * Registers a scanner driver, making it available to scanner API users.
 *
 * @name:	unique scanner name
 * @ops:	scanner operations function pointers
 *
 * @returns:	0 for success
 *		negative value for error
 */
int scanner_register(const char *name, struct scanner_ops *ops);

/**
 * scanner_init - scanner driver initialisation
 *
 * This macro declares a public int (function)(void) as a scanner driver
 * initialisation function. This function will be called once during
 * the drivers lifetime and should be used to execute one-off hardware
 * initialisation routines and to register the driver operations.
 *
 * @function:	a function returning int value (0 for success, -1 for error)
 *              and taking no arguments
 */
#define __scanner_init(function) \
	int (*__##function)(void) \
			__attribute__((section("scanner_init"))) = function

#ifdef __cplusplus
}
#endif

#endif
