#include <byteswap.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "scanner.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))



static void hexdump(FILE *fl, void *buffer, int size)
{
	unsigned char *c = buffer;
	unsigned char ascii[17] = { 0, };
	int i;

	for (i = 0; i < size; i++) {
		if ((i % 16) == 0) {
			if (i > 0)
				fprintf(fl, " %s\n", ascii);
			memset(ascii, ' ', 16);
			fprintf(fl, "%08x  ", i);
		}
		fprintf(fl, "%02x ", c[i]);
		ascii[i % 16] = (c[i] >= 32 && c[i] <= 127) ? c[i] : '.';
	}
	while ((i++ % 16) != 0)
		fprintf(fl, "   ");
	fprintf(fl, " %s\n", ascii);
}



static void cdump(FILE *fl, void *buffer, int size)
{
	unsigned char *c = buffer;
	int i;
	int columns = 8;

	fprintf(fl, "unsigned char iso_template[] = {\n");
	for (i = 0; i < size; i++) {
		if ((i % columns) == 0)
			fprintf(fl, i == 0 ? "\t" : "\n\t");
		fprintf(fl, (i % columns) == 0 ? "0x%02x," : " 0x%02x,", c[i]);
	}
	fprintf(fl, "\n};\n");
	fprintf(fl, "int iso_template_size = %d;\n", size);
}


static void isodecode(FILE *fl, void *buffer, int size)
{
	unsigned char *data = buffer;

	struct iso_header {
		uint32_t format_id;
		uint32_t version;
		uint32_t total_length;
		uint16_t capture_equip_cert__capture_device_type_id; /* 4 bits __ 12 bits */
		uint16_t size_x;
		uint16_t size_y;
		uint16_t resolution_x;
		uint16_t resolution_y;
		uint8_t number_views;
		uint8_t reserved;
	} *header_be, header;
	struct iso_view {
		uint8_t finger_position;
		uint8_t view_number__impression_type; /* 4 bits __ 4 bits */
		uint8_t finger_quality;
		uint8_t number_minutiae;
	} *view;
	struct iso_minutae {
		uint16_t type__x; /* 2 bits __ 14 bits */
		uint16_t reserved__y; /* 2 bits __ 14 bits */
		uint8_t angle;
		uint8_t quality;
	} *minutae_be, minutae;
	uint16_t extended_data_block_length;
	int v, m;

	if (size < sizeof(header)) {
		fprintf(stderr, "Template too short for the header!\n");
		return;
	}

	header_be = (struct iso_header *)data;
#if BE
	header = *header_be;
#else
	header.format_id = bswap_32(header_be->format_id);
	header.version = bswap_32(header_be->version);
	header.total_length = bswap_32(header_be->total_length);
	header.capture_equip_cert__capture_device_type_id =
		bswap_16(header_be->capture_equip_cert__capture_device_type_id);
	header.size_x = bswap_16(header_be->size_x);
	header.size_y = bswap_16(header_be->size_y);
	header.resolution_x = bswap_16(header_be->resolution_x);
	header.resolution_y = bswap_16(header_be->resolution_y);
	header.number_views = header_be->number_views;
	header.reserved = header_be->reserved;
#endif

	fprintf(fl, "Format identifier: '%s' (0x%08x)\n",
			(char *)(&header_be->format_id), header.format_id);
	if (header.format_id != 0x464d5200) {
		fprintf(stderr, "Wrong format identifier!\n");
		return;
	}

	fprintf(fl, "Version: '%s' (0x%08x)\n",
			(char *)(&header_be->version), header.version);
	if (header.version != 0x20323000) {
		fprintf(stderr, "Wrong version!\n");
		return;
	}

	fprintf(fl, "Total length: %u bytes\n", header.total_length);
	if (header.total_length != size) {
		fprintf(stderr, "Wrong total length! (expected %d)\n", size);
		return;
	}

	fprintf(fl, "Capture equipment certification: %u\n",
			(header.capture_equip_cert__capture_device_type_id &
			0xf000) >> 12);
	fprintf(fl, "Capture device type ID: %u\n",
			header.capture_equip_cert__capture_device_type_id &
			0x0fff);
	fprintf(fl, "Image size in X: %u pixels\n", header.size_x);
	fprintf(fl, "Image size in Y: %u pixels\n", header.size_y);
	fprintf(fl, "Horizontal (X) resolution: %u pixels per cm\n",
			header.resolution_x);
	fprintf(fl, "Horizontal (Y) resolution: %u pixels per cm\n",
			header.resolution_y);
	fprintf(fl, "Number of finger views: %u\n", header.number_views);

	if (header.reserved != 0) {
		fprintf(stderr, "Wrong reserved byte 0x%x! (expected zero)\n",
				header.reserved);
		return;
	}

	data += sizeof(header);
	size -= sizeof(header);

	for (v = 0; v < header.number_views; v++) {
		static const char *positions[] = {
			[0] = "unknown",
			[1] = "right thumb",
			[2] = "right index",
			[3] = "right middle",
			[4] = "right ring",
			[5] = "right little",
			[6] = "left thumb",
			[7] = "left index",
			[8] = "left middle",
			[9] = "left ring",
			[10] = "left little",
		};
		static const char *impressions[] = {
			[0] = "live-scan plain",
			[1] = "live-scan rolled",
			[2] = "nonlive-scan plain",
			[3] = "nonlive-scan rolled",
			[4] = "latent impression",
			[5] = "latent tracing",
			[6] = "latent photo",
			[7] = "latent lift",
			[8] = "swipe",
		};
		unsigned impression;

		if (size < sizeof(*view)) {
			fprintf(stderr, "Template too short for the view!\n");
			return;
		}

		view = (struct iso_view *)data;

		fprintf(fl, "\nView %d:\n", v);
		if (view->finger_position > ARRAY_SIZE(positions)) {
			fprintf(stderr, "Unknown finger position!\n");
			return;
		}
		fprintf(fl, "\tFinger position: %s (%u)\n",
				positions[view->finger_position],
				view->finger_position);
		fprintf(fl, "\tView number: %u\n",
				(view->view_number__impression_type & 0xf0)
				>> 4);
		impression = view->view_number__impression_type & 0x0f;
		if (impression > ARRAY_SIZE(impressions)) {
			fprintf(stderr, "Unknown impression type!\n");
			return;
		}
		fprintf(fl, "\tImpression type: %s (%u)\n",
				impressions[impression], impression);
		fprintf(fl, "\tFinger quality: %u\n",
				view->finger_quality);
		if (view->finger_quality > 100) {
			fprintf(stderr, "Wrong finger quality!\n");
			return;
		}
		fprintf(fl, "\tNumber of minutiae: %d\n",
				view->number_minutiae);

		data += sizeof(*view);
		size -= sizeof(*view);

		for (m = 0; m < view->number_minutiae; m++) {
			static const char *types[] = {
				[0] = "other",
				[1] = "termination",
				[2] = "bifurcation",
			};
			unsigned type, angle;

			if (size < sizeof(minutae)) {
				fprintf(stderr, "Template too short for the minutae!\n");
				return;
			}

			minutae_be = (struct iso_minutae *)data;
#if BE
			minutae = *minutae_be;
#else
			minutae.type__x = bswap_16(minutae_be->type__x);
			minutae.reserved__y = bswap_16(minutae_be->reserved__y);
			minutae.angle = minutae_be->angle;
			minutae.quality = minutae_be->quality;
#endif

			fprintf(fl, "\n\tMinutae %d:\n", m);

			type = (minutae.type__x & 0xc000) >> 14;
			if (type >= ARRAY_SIZE(types)) {
				fprintf(stderr, "Wrong type!\n");
				return;
			}
			fprintf(fl,"\t\tType: '%s' (%u)\n", types[type], type);

			fprintf(fl, "\t\tX: %u\n", minutae.type__x & 0x3fff);
			fprintf(fl, "\t\tY: %u\n",
					minutae.reserved__y & 0x3fff);
			angle = minutae.angle * 140625;
			fprintf(fl, "\t\tAngle: %u.%05u deg (%u * 1.40625)\n",
					angle / 100000, angle % 100000,
					minutae.angle);
			if (minutae.quality == 0)
				fprintf(fl, "\t\tQuality: not reported\n");
			else
				fprintf(fl, "\t\tQuality: %u\n",
						minutae.quality);

			data += sizeof(minutae);
			size -= sizeof(minutae);
		}

		if (size < sizeof(extended_data_block_length)) {
			fprintf(stderr, "Template too short for the extended data block length field!\n");
			return;
		}
#if BE
		extended_data_block_length = *((uint16_t *)data);
#else
		extended_data_block_length = bswap_16(*((uint16_t *)data));
#endif
		fprintf(fl, "\n\tExtended data block length: %d bytes (skipping)\n",
				extended_data_block_length);
		data += sizeof(extended_data_block_length) +
			extended_data_block_length;
		size -= sizeof(extended_data_block_length) +
				extended_data_block_length;
	}
}


static void usage(const char *comm)
{
	fprintf(stderr, "Usage: %s [-h] [-x|-b|-c] [NAME]\n", comm);
	fprintf(stderr, "where:\n");
	fprintf(stderr, "\t-h\tusage syntax (this message)\n");
	fprintf(stderr, "\t-d\tdecode template (default)\n");
	fprintf(stderr, "\t-x\traw hexdump output\n");
	fprintf(stderr, "\t-b\traw binary output\n");
	fprintf(stderr, "\t-c\traw C structure output\n");
	fprintf(stderr, "\tNAME\t(optional) output file, stdout by default\n");
}



int main(int argc, char *argv[])
{
	int opt;
	FILE *fl = stdout;
	enum {
		decode,
		hex,
		binary,
		c_struct
	} output = decode;
	int err;
	struct scanner_caps caps;
	int size;
	unsigned char *template;

	while ((opt = getopt(argc, argv, "dxbch")) != -1) {
		switch (opt) {
		case 'd':
			output = decode;
			break;
		case 'x':
			output = hex;
			break;
		case 'b':
			output = binary;
			break;
		case 'c':
			output = c_struct;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
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

	err = scanner_on();
	if (err) {
		fprintf(stderr, "Failed to turn on scanner (%d)\n", err);
		return 1;
	}

	err = scanner_get_caps(&caps);
	if (err) {
		fprintf(stderr, "Failed to get capabilities (%d)\n", err);
		return 1;
	}

	if (!caps.iso_template) {
		fprintf(stderr, "Scanner provides no ISO templates!\n");
		return 1;
	}

	err = scanner_scan(-1);
	if (err == -1) {
		fprintf(stderr, "Timeout when scanning...\n");
		return 1;
	}
	if (err) {
		fprintf(stderr, "Error when scanning! (%d)\n", err);
		return 1;
	}

	size = scanner_get_iso_template(NULL, 0);
	if (size == 0) {
		fprintf(stderr, "No template size returned!\n");
		return 1;
	}
	if (size < 0) {
		fprintf(stderr, "Failed to obtain template size! (%d)\n",
				size);
		return 1;
	}

	template = malloc(size);
	if (!template) {
		fprintf(stderr, "Out of memory (needed %d bytes)!\n", size);
		return 1;
	}

	size = scanner_get_iso_template(template, size);
	if (size == 0) {
		fprintf(stderr, "No template returned!\n");
		return 1;
	}
	if (size < 0) {
		fprintf(stderr, "Failed to obtain template! (%d)\n", size);
		return 1;
	}

	switch (output) {
	case decode:
		isodecode(fl, template, size);
		break;
	case hex:
		hexdump(fl, template, size);
		break;
	case binary:
		size = fwrite(template, size, 1, fl);
		if (size != 1) {
			fprintf(stderr, "Failed to write!\n");
			return 1;
		}
		break;
	case c_struct:
		cdump(fl, template, size);
		break;
	}

	free(template);

	scanner_off();

	if (fl != stdout)
		fclose(fl);

	return 0;
}
