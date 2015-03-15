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


static void isov20decode(FILE *fl, void *buffer, int size)
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

static void isov030decode(FILE *fl, void *buffer, int size)
{
	unsigned char *data = buffer;

	struct __attribute__((__packed__))  iso_header {
		uint32_t format_id;
		uint32_t version;
		uint32_t total_length;
		uint16_t number_views;
	} *header_be, header;
	struct __attribute__((__packed__)) iso_view_head {
		uint8_t finger_position;
		uint8_t view_number__impression_type; /* 4 bits __ 4 bits*/
		uint16_t size_x;
		uint16_t size_y;
		uint16_t resolution_x;
		uint16_t resolution_y;
		uint8_t finger_quality_number_blocks;
	} *view_head_be, view_head;
	struct __attribute__((__packed__)) iso_view_quality_block {
		uint8_t quality_value;
		uint16_t quality_vendor_id;
		uint16_t quality_algorithm_id;
	} *quality_block_be, quality_block;
	struct __attribute__((__packed__)) iso_view_tail {
		uint8_t minutia_field_length__ridge_ending_type; /* 4 bits __ 4 bits */
		uint16_t extraction_vendor_id;
		uint16_t extraction_algorithm_id;
		uint8_t number_minutiae;
		struct __attribute__((__packed__)) {
			uint16_t year;
			uint8_t month;
			uint8_t day;
			uint8_t hour;
			uint8_t minute;
			uint8_t second;
			uint16_t microsecond;
			uint8_t zone;
		} capture_data_time;
		uint16_t capture_device_vendor_id;
		uint16_t capture_device_type_id;
		uint8_t capture_equip_cert;
	} *view_tail_be, view_tail;
	struct __attribute__((__packed__)) iso_minutae_5_bytes {
		uint16_t type__x; /* 2 bits __ 14 bits */
		uint16_t reserved__y; /* 2 bits __ 14 bits */
		uint8_t angle;
	} *minutae_5_be;
	struct __attribute__((__packed__)) iso_minutae_6_bytes {
		uint16_t type__x; /* 2 bits __ 14 bits */
		uint16_t reserved__y; /* 2 bits __ 14 bits */
		uint8_t angle;
		uint8_t quality;
	} *minutae_6_be, minutae;
	uint16_t extended_data_block_length;
	int v, b, m;

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
	header.number_views = bswap_16(header_be->number_views);
#endif

	fprintf(fl, "Format identifier: '%s' (0x%08x)\n",
			(char *)(&header_be->format_id), header.format_id);
	if (header.format_id != 0x464d5200) {
		fprintf(stderr, "Wrong format identifier!\n");
		return;
	}

	fprintf(fl, "Version: '%s' (0x%08x)\n",
			(char *)(&header_be->version), header.version);
	if (header.version != 0x30333000) {
		fprintf(stderr, "Wrong version!\n");
		return;
	}

	fprintf(fl, "Total length: %u bytes\n", header.total_length);
	if (header.total_length != size) {
		fprintf(stderr, "Wrong total length! (expected %d)\n", size);
		return;
	}

	fprintf(fl, "Number of finger views: %u\n", header.number_views);

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
			[8] = "live-scan vertical swipe",
			[10] = "live-scan palm",
			[11] = "nonlive-scan palm",
			[12] = "latent palm impression",
			[13] = "latent palm tracing",
			[14] = "latent palm photo",
			[15] = "latent palm lift",
			[20] = "live-scan optical contact plain",
			[21] = "live-scan optical contact rolled",
			[22] = "live-scan non-optical contact plain",
			[23] = "live-scan non-optical contact rolled",
			[24] = "live-scan optical contactless plain",
			[25] = "live-scan optical contactless rolled",
			[26] = "live-scan non-optical contactless plain",
			[27] = "live-scan non-optical contactless rolled",
		};
		static const char *ridge_ending_types[] = {
			[0] = "intersection of three valley bifurcation",
			[1] = "ridge skeleton endpoints",
		};
		unsigned impression, minutia_field_length, ridge_ending_type;

		if (size < sizeof(view_head)) {
			fprintf(stderr, "Template too short for the view head!\n");
			return;
		}

		view_head_be = (struct iso_view_head *)data;
#if BE
		view_head = *view_head_be;
#else
		view_head.finger_position = view_head_be->finger_position;
		view_head.view_number__impression_type =
				view_head_be->view_number__impression_type;
		view_head.size_x = bswap_16(view_head_be->size_x);
		view_head.size_y = bswap_16(view_head_be->size_y);
		view_head.resolution_x = bswap_16(view_head_be->resolution_x);
		view_head.resolution_y = bswap_16(view_head_be->resolution_y);
#endif

		fprintf(fl, "\nView %d:\n", v);
		if (view_head.finger_position > ARRAY_SIZE(positions)) {
			fprintf(stderr, "Unknown finger position %u!\n",
					view_head.finger_position);
			return;
		}
		fprintf(fl, "\tFinger position: %s (%u)\n",
				positions[view_head.finger_position],
				view_head.finger_position);
		fprintf(fl, "\tView number: %u\n",
				(view_head.view_number__impression_type & 0xf0)
				>> 4);
		impression = view_head.view_number__impression_type & 0x0f;
		if (impression > ARRAY_SIZE(impressions) ||
				!impressions[impression]) {
			fprintf(stderr, "Unknown impression type %u!\n",
					impression);
			return;
		}
		fprintf(fl, "\tImpression type: %s (%u)\n",
				impressions[impression], impression);

		fprintf(fl, "\tImage size in X: %u pixels\n", view_head.size_x);
		fprintf(fl, "\tImage size in Y: %u pixels\n", view_head.size_y);
		fprintf(fl, "\tHorizontal (X) resolution: %u pixels per cm\n",
				view_head.resolution_x);
		fprintf(fl, "\tHorizontal (Y) resolution: %u pixels per cm\n",
				view_head.resolution_y);

		fprintf(fl, "\tNumber of finger quality blocks: %u\n",
				view_head.finger_quality_number_blocks);

		data += sizeof(view_head);
		size -= sizeof(view_head);

		for (b = 0; b < view_head.finger_quality_number_blocks; b++) {
			if (size < sizeof(quality_block)) {
				fprintf(stderr, "Template too short for the quality block!\n");
				return;
			}

			quality_block_be =
					(struct iso_view_quality_block *)data;
#if BE
			quality_block = *quality_block_be;
#else
			quality_block.quality_value =
					quality_block_be->quality_value;
			quality_block.quality_vendor_id =
					bswap_16(quality_block_be->quality_vendor_id);
			quality_block.quality_algorithm_id =
					bswap_16(quality_block_be->quality_algorithm_id);
#endif

			fprintf(fl, "\n\tQuality block %d:\n", b);

			fprintf(fl, "\t\tQuality value: %u\n",
				       quality_block.quality_value);
			fprintf(fl, "\t\tQuality vendor ID: 0x%04x (see www.ibia.org)\n",
					quality_block.quality_vendor_id);
			fprintf(fl, "\t\tQuality algorithm ID: 0x%04x\n",
					quality_block.quality_algorithm_id);

			data += sizeof(quality_block);
			size -= sizeof(quality_block);
		}

		if (size < sizeof(view_tail)) {
			fprintf(stderr, "Template too short for the view tail!\n");
			return;
		}

		view_tail_be = (struct iso_view_tail *)data;
#if BE
		view_tail = *view_tail_be;
#else
		view_tail.minutia_field_length__ridge_ending_type =
				view_tail_be->minutia_field_length__ridge_ending_type;
		view_tail.extraction_vendor_id =
				bswap_16(view_tail_be->extraction_vendor_id);
		view_tail.number_minutiae = view_tail_be->number_minutiae;
		view_tail.capture_data_time.year =
				bswap_16(view_tail_be->capture_data_time.year);
		view_tail.capture_data_time.month =
				view_tail_be->capture_data_time.month;
		view_tail.capture_data_time.day =
				view_tail_be->capture_data_time.day;
		view_tail.capture_data_time.hour =
				view_tail_be->capture_data_time.hour;
		view_tail.capture_data_time.minute =
				view_tail_be->capture_data_time.minute;
		view_tail.capture_data_time.second =
				view_tail_be->capture_data_time.second;
		view_tail.capture_data_time.microsecond =
				bswap_16(view_tail_be->capture_data_time.microsecond);
		view_tail.capture_data_time.zone =
				view_tail_be->capture_data_time.zone;

		view_tail.capture_device_vendor_id =
				bswap_16(view_tail_be->capture_device_vendor_id);
		view_tail.capture_device_type_id =
				bswap_16(view_tail_be->capture_device_type_id);
		view_tail.capture_equip_cert = view_tail_be->capture_equip_cert;
#endif

		minutia_field_length = view_tail.minutia_field_length__ridge_ending_type >> 4;
		fprintf(fl, "\tMinutia field lenght: %u bytes\n",
				minutia_field_length);
		if (minutia_field_length != 5 && minutia_field_length != 6) {
			fprintf(stderr, "Wrong minutia field length!\n");
			return;
		}

		ridge_ending_type =
				view_tail.minutia_field_length__ridge_ending_type & 0xf;
		if (ridge_ending_type > ARRAY_SIZE(ridge_ending_types)) {
			fprintf(stderr, "Unknown ridge ending type %u!\n",
					ridge_ending_type);
			return;
		}
		fprintf(fl, "\tRidge ending type: %s (%u)\n",
				ridge_ending_types[ridge_ending_type],
				ridge_ending_type);

		fprintf(fl, "\tExtraction vendor ID: 0x%04x (see www.ibia.org)\n",
				view_tail.extraction_vendor_id);
		fprintf(fl, "\tExtraction algorithm ID: 0x%04x\n",
				view_tail.extraction_algorithm_id);

		fprintf(fl, "\tNumber of minutiae: %d\n",
				view_tail.number_minutiae);

		if (view_tail.capture_data_time.month > 12) {
			fprintf(stderr, "Wrong month %d\n",
					view_tail.capture_data_time.month);
			return;
		}
		if (view_tail.capture_data_time.day > 31) {
			fprintf(stderr, "Wrong day %d\n",
					view_tail.capture_data_time.day);
			return;
		}
		fprintf(fl, "\tDate of capture: %04u-%02u-%02u\n",
				view_tail.capture_data_time.year,
				view_tail.capture_data_time.month,
				view_tail.capture_data_time.day);


		if (view_tail.capture_data_time.hour > 23 &&
				view_tail.capture_data_time.hour != 99) {
			fprintf(stderr, "Wrong hour %d\n",
					view_tail.capture_data_time.hour);
			return;
		}
		if (view_tail.capture_data_time.minute > 59 &&
				view_tail.capture_data_time.minute != 99) {
			fprintf(stderr, "Wrong minute %d\n",
					view_tail.capture_data_time.minute);
			return;
		}
		if (view_tail.capture_data_time.second > 59 &&
				view_tail.capture_data_time.second != 99) {
			fprintf(stderr, "Wrong second %d\n",
					view_tail.capture_data_time.second);
			return;
		}
		if (view_tail.capture_data_time.zone != 'Z') {
			fprintf(stderr, "Wrong time zone '%c'!\n",
					view_tail.capture_data_time.zone);
			return;
		}
		fprintf(fl, "\tTime of capture: %02u:%02u:%02u.%04u UTC (99 == unassigned)\n",
				view_tail.capture_data_time.hour,
				view_tail.capture_data_time.minute,
				view_tail.capture_data_time.second,
				view_tail.capture_data_time.microsecond);


		fprintf(fl, "\tCapture device vendor ID: 0x%04x (see www.ibia.org)\n",
				view_tail.capture_device_vendor_id);
		fprintf(fl, "\tCapture device type ID: 0x%04x\n",
				view_tail.capture_device_type_id);
		fprintf(fl, "\tCapture equipment certification: %u\n",
				view_tail.capture_equip_cert);

		data += sizeof(view_tail);
		size -= sizeof(view_tail);

		for (m = 0; m < view_tail.number_minutiae; m++) {
			static const char *types[] = {
				[0] = "other",
				[1] = "termination",
				[2] = "bifurcation",
			};
			unsigned type, angle;

			if (size < minutia_field_length) {
				fprintf(stderr, "Template too short for the minutae!\n");
				return;
			}

			switch (minutia_field_length) {
			case 5:
				minutae_5_be = (struct iso_minutae_5_bytes *)data;
#if BE
				minutae.type__x = minutae_5_be->type__x;
				minutae.reserved__y = minutae_5_be->reserved__y;
				minutae.angle = minutae_5_be->angle;
				minutae.quality = 0;
#else
				minutae.type__x =
						bswap_16(minutae_5_be->type__x);
				minutae.reserved__y =
						bswap_16(minutae_5_be->reserved__y);
				minutae.angle = minutae_5_be->angle;
				minutae.quality = 0;
#endif
				break;
			case 6:
				minutae_6_be = (struct iso_minutae_6_bytes *)data;
#if BE
				minutae = *minutae_6_be;
#else
				minutae.type__x =
						bswap_16(minutae_6_be->type__x);
				minutae.reserved__y =
						bswap_16(minutae_6_be->reserved__y);
				minutae.angle = minutae_6_be->angle;
				minutae.quality = minutae_6_be->quality;
#endif
				break;
			}

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

			if (minutae.quality > 0)
				fprintf(fl, "\t\tQuality: %u\n",
						minutae.quality);

			data += minutia_field_length;
			size -= minutia_field_length;
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

static void isodecode(FILE *fl, void *buffer, int size)
{
	unsigned char *data = buffer;

	struct iso_header {
		uint32_t format_id;
		uint32_t version;
	} *header_be, header;

	header_be = (struct iso_header *)data;
#if BE
	header = *header_be;
#else
	header.format_id = bswap_32(header_be->format_id);
	header.version = bswap_32(header_be->version);
#endif

	if (header.format_id != 0x464d5200) {
		fprintf(stderr, "Wrong format identifier: '%s' (0x%08x)\n",
			(char *)(&header_be->format_id), header.format_id);
		return;
	}

	switch (header.version) {
	case 0x20323000:
		isov20decode(fl, buffer, size);
		break;
	case 0x30333000:
		isov030decode(fl, buffer, size);
		break;
	default:
		fprintf(stderr, "Wrong version:: '%s' (0x%08x)\n",
			(char *)(&header_be->version), header.version);
		break;
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
