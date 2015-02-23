#ifndef __SCANNER_H
#define __SCANNER_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * scanner_on - turn the scanner on
 *
 * A scanner must be turned on before any other function is called.
 *
 * @returns: 0 for success
 *           negative value for error
 */
int scanner_on(void);

/**
 * scanner_off - turn the scanner off
 */
void scanner_off(void);

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
	} image_format;
	int image_width, image_height;
};

/**
 * scanner_get_caps - provide scanner capabilities
 *
 * @caps: pointer to capabilities structure
 *
 * @returns: 0 for success
 *           negative value for error
 */
int scanner_get_caps(struct scanner_caps *caps);

/**
 * scanner_scan - perform a single scan
 *
 * @timeout:	in miliseconds
 *		0 checks if a scan is already available
 *		-1 waits infinitely until a scan is ready
 *
 * @returns:	0 for success
 *		-1 for timeout
 *		other negative value for error
 */
int scanner_scan(int timeout);

/**
 * scanner_get_image - provide fingerprint image
 *
 * Fills the given @buffer (up to its @size, never more) with an image of the
 * most recently scanned fingerprint. Returns its full size in bytes, which
 * can be lager than the given buffer size. This can be used to check the
 * required buffer size (call the function with size 0).
 *
 * @buffer:	pointer to a buffer to be filled with the image
 * @size:	buffer size in bytes
 *
 * @returns:	non-negative value is the size of the image in bytes
 *			(can be larger than @size)
 *		negative value for error
 */
int scanner_get_image(void *buffer, int size);

/**
 * scanner_get_iso_template - provide ISO fingerprint template
 *
 * Fills the given @buffer (up to its @size, never more) with a ISO/IEC 19794-2
 * (2005) complaint template of the most recently scanned fingerprint. Returns
 * its full size in bytes, which can be lager than the given buffer size. This
 * can be used to check the required buffer size (call the function with
 * size 0).
 *
 * @buffer:	pointer to a buffer to be filled with the template
 * @size:	buffer size in bytes
 *
 * @returns:	non-negative value is the size of the template in bytes
 *			(can be larger than @size)
 *		negative value for error
 */
int scanner_get_iso_template(void *buffer, int size);

#ifdef __cplusplus
}
#endif

#endif
