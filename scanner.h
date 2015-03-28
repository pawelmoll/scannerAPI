#ifndef __SCANNER_H
#define __SCANNER_H

#ifdef __cplusplus
extern "C"
{
#endif

struct scanner;

/**
 * scanner_init - initialises scanner API
 *
 * Must be called before any other function from this API is used!
 *
 * @returns:	0 for success
 *		negative value for error
 */
int scanner_init(void);

/**
 * scanner_list - returns a list of scanner
 *
 * @number:	(pointer to a) number of scanners (size of the array)
 *
 * @returns:	pointer to an array of scanner names
 */
const char **scanner_list(int *number);

/**
 * scanner_get - obtains a pointer to a scanner
 *
 * There may be only one user of a scanner at any time.
 *
 * @name:	name of a scanner, one of the @scanner_list
 *
 * @returns:	pointer to a scanner when found and available
 *		NULL when given name is wrong or the scanner is already in use
 */
struct scanner *scanner_get(const char *name);

/**
 * scanner_put - returns a pointer to a scanner
 *
 * Returns the scanner for other users.
 *
 * @scanner:	pointer to a scanner
 */

void scanner_put(struct scanner *scanner);

/**
 * scanner_on - turn the scanner on
 *
 * Scanner must be turned on before any other scanner operation is called!
 *
 * @scanner:	pointer to a scanner
 *
 * @returns:	0 for success
 *		negative value for error
 */
int scanner_on(struct scanner *scanner);

/**
 * scanner_off - turn the scanner off
 *
 * @scanner: pointer to a scanner
 */
void scanner_off(struct scanner *scanner);

/**
 * struct scanner_caps - scanner capabilities
 *
 * Image formats:
 *	gray_8bit - raw grayscale image, one byte per pixel, 0x00 means black,
 *			0xff means white
 *
 * @name:		pointer to a static, NULL-terminated string uniquely
 *				identifying (describing) the scanner
 * @image:		1 if scanner can provide a fingerprint image
 * @iso_template:	1 if scanner can provide a ISO/IEC 19794-2 (2005)
 *				complaint fingerprint template
 * @image_format:	if @image, format of the image provided
 * @image_width:	if @image, number of pixels in an image row
 * @image_height:	if @image, number of image rows
 */
struct scanner_caps {
	const char *name;

	unsigned image:1;
	unsigned iso_template:1;

	enum {
		scanner_image_gray_8bit,
		scanner_image_gray_8bit_inversed,
	} image_format;
	int image_width, image_height;
};

/**
 * scanner_get_caps - provide scanner capabilities
 *
 * @scanner:	pointer to a scanner
 * @caps:	pointer to capabilities structure
 *
 * @returns: 0 for success
 *           negative value for error
 */
int scanner_get_caps(struct scanner *scanner, struct scanner_caps *caps);

/**
 * scanner_scan - perform a single scan
 *
 * @scanner:    pointer to a scanner
 * @timeout:	in miliseconds
 *		0 checks if a scan is already available
 *		-1 waits infinitely until a scan is ready
 *
 * @returns:	0 for success
 *		-1 for timeout
 *		other negative value for error
 */
int scanner_scan(struct scanner *scanner, int timeout);

/**
 * scanner_get_image - provide fingerprint image
 *
 * Fills the given @buffer (up to its @size, never more) with an image of the
 * most recently scanned fingerprint. Returns its full size in bytes, which
 * can be lager than the given buffer size. This can be used to check the
 * required buffer size (call the function with size 0).
 *
 * @scanner:    pointer to a scanner
 * @buffer:	pointer to a buffer to be filled with the image
 * @size:	buffer size in bytes
 *
 * @returns:	non-negative value is the size of the image in bytes
 *			(can be larger than @size)
 *		negative value for error
 */
int scanner_get_image(struct scanner *scanner, void *buffer, int size);

/**
 * scanner_get_iso_template - provide ISO fingerprint template
 *
 * Fills the given @buffer (up to its @size, never more) with a ISO/IEC 19794-2
 * (2005) complaint template of the most recently scanned fingerprint. Returns
 * its full size in bytes, which can be lager than the given buffer size. This
 * can be used to check the required buffer size (call the function with
 * size 0).
 *
 * @scanner:    pointer to a scanner
 * @buffer:	pointer to a buffer to be filled with the template
 * @size:	buffer size in bytes
 *
 * @returns:	non-negative value is the size of the template in bytes
 *			(can be larger than @size)
 *		negative value for error
 */
int scanner_get_iso_template(struct scanner *scanner, void *buffer, int size);

#ifdef __cplusplus
}
#endif

#endif
