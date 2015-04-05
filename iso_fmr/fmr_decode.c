#include <byteswap.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "v20.h"
#include "v030.h"



static void hexdump(const char *prefix, void *buffer, int size)
{
	unsigned char *c = buffer;
	unsigned char ascii[17] = { 0, };
	int i;

	if (!prefix)
		prefix = "";

	for (i = 0; i < size; i++) {
		if ((i % 16) == 0) {
			if (i > 0)
				printf(" %s\n", ascii);
			memset(ascii, ' ', 16);
			printf("%s%08x  ", prefix, i);
		}
		printf("%02x ", c[i]);
		ascii[i % 16] = (c[i] >= 32 && c[i] <= 127) ? c[i] : '.';
	}
	while ((i++ % 16) != 0)
		printf("   ");
	printf(" %s\n", ascii);
}

static void v20dump(struct iso_fmr_v20 *record)
{
	int v;

	printf("Format identifier: 0x%08x (%c%c%c%c)\n",
			record->format_id,
			(record->format_id >> 24) & 0xff,
			(record->format_id >> 16) & 0xff,
			(record->format_id >> 8) & 0xff,
			(record->format_id >> 0) & 0xff);
	printf("Version: 0x%08x (%c%c%c%c)\n",
			record->version,
			(record->version >> 24) & 0xff,
			(record->version >> 16) & 0xff,
			(record->version >> 8) & 0xff,
			(record->version >> 0) & 0xff);
	printf("Total length: %u bytes\n", record->total_length);
	printf("Capture equipment certification: %u\n",
			record->capture_equip_cert);
	printf("Capture device type ID: %u\n",
			record->capture_device_type_id);
	printf("Image size in X: %u (pixels)\n", record->size_x);
	printf("Image size in Y: %u (pixels)\n", record->size_y);
	printf("Horizontal (X) resolution: %u (pixels per cm)\n",
			record->resolution_x);
	printf("Vertical (Y) resolution: %u (pixels per cm)\n",
			record->resolution_y);
	printf("Number of finger views: %u\n", record->number_views);

	for (v = 0; v < record->number_views; v++) {
		struct iso_fmr_v20_view *view = &record->views[v];
		int m;

		printf("\nView %d:\n", v);
		printf("\tFinger position: %u (%s)\n",
				view->finger_position,
				iso_fmr_v20_get_finger_position_string(
				view->finger_position));
		printf("\tRepresentation number: %u\n",
				view->representation_number);
		printf("\tImpression type: %u (%s)\n",
				view->impression_type,
				iso_fmr_v20_get_impression_type_string(
				view->impression_type));
		printf("\tFinger quality: %u\n", view->finger_quality);
		printf("\tNumber of minutiae: %d\n",
				view->number_minutiae);

		for (m = 0; m < view->number_minutiae; m++) {
			struct iso_fmr_v20_minutia *minutia =
					&view->minutiae[m];
			unsigned angle;

			printf("\n\tMinutia %d:\n", m);

			printf("\t\tType: %u (%s)\n", minutia->type,
					iso_fmr_v20_get_minutia_type_string(
					minutia->type));

			printf("\t\tX: %u\n", minutia->x);
			printf("\t\tY: %u\n", minutia->y);

			angle = minutia->angle * 140625;
			printf("\t\tAngle: %u (* 1.40625 deg = %u.%05u deg)\n",
					minutia->angle,
					angle / 100000, angle % 100000);
			if (minutia->quality == 0)
				printf("\t\tQuality: not reported\n");
			else
				printf("\t\tQuality: %u\n",
						minutia->quality);
		}

		printf("\n\tExtended data block length: %u bytes\n",
				view->extended_data_block_length);
		if (view->extended_data_block_length) {
			printf("\tExtended data block:\n");
			hexdump("\t\t", view->extended_data_block,
					view->extended_data_block_length);
		}
	}
}

static void v030dump(struct iso_fmr_v030 *record)
{
	int r;

	printf("Format identifier: 0x%08x (%c%c%c%c)\n",
			record->format_id,
			(record->format_id >> 24) & 0xff,
			(record->format_id >> 16) & 0xff,
			(record->format_id >> 8) & 0xff,
			(record->format_id >> 0) & 0xff);
	printf("Version: 0x%08x (%c%c%c%c)\n",
			record->version,
			(record->version >> 24) & 0xff,
			(record->version >> 16) & 0xff,
			(record->version >> 8) & 0xff,
			(record->version >> 0) & 0xff);
	printf("Total length: %u bytes\n", record->total_length);
	printf("Number of finger representations: %u\n",
			record->number_representations);
	printf("Device certification block: %u (%s)\n",
			record->device_certification_block_flag,
			record->device_certification_block_flag ?
			"present" : "not present");

	for (r = 0; r < record->number_representations; r++) {
		struct iso_fmr_v030_representation *repr =
				&record->representations[r];
		int b, m;

		printf("\nRepresentation %d:\n", r);
		printf("\tRepresentation length: %u bytes\n",
				repr->representation_length);
		/* TODO: date and time */
		printf("\tCapture device technology ID: %u (%s)\n",
				repr->capture_device_technology_id,
				iso_fmr_v030_get_device_technology_string(
				repr->capture_device_technology_id));
		printf("\tCapture device vendor ID: 0x%04x (see www.ibia.org)\n",
				repr->capture_device_vendor_id);
		printf("\tCapture device type ID: 0x%04x\n",
				repr->capture_device_type_id);
		printf("\tNumber of quality blocks: %u\n",
				repr->number_quality_blocks);
		for (b = 0; b < repr->number_quality_blocks; b++) {
			struct iso_fmr_v030_quality_block *block =
				&repr->quality_blocks[b];

			printf("\n\tQuality block %d:\n", b);
			printf("\t\tQuality value: %u",
					block->quality_value);
			if (block->quality_value == 255)
				printf(" (failed to calculate)\n");
			else
				printf("\n");
			printf("\t\tQuality vendor ID: 0x%04x (see www.ibia.org)\n",
					block->quality_vendor_id);
			printf("\t\tQuality algorithm ID: 0x%04x\n",
					block->quality_algorithm_id);
		}
		if (record->device_certification_block_flag) {
			printf("\tNumber of certification blocks: %u\n",
					repr->number_certification_blocks);
			for (b = 0; b < repr->number_certification_blocks; b++) {
				struct iso_fmr_v030_certification_block *block =
					&repr->certification_blocks[b];

				printf("\n\tCertification block %d:\n", b);
				printf("\t\tCertification authority ID: 0x%04x (see www.ibia.org)\n",
						block->certification_authority_id);
				printf("\t\tCertification scheme ID: 0x%02x\n",
						block->certification_scheme_id);
			}
		}
		printf("\tFinger position: %u (%s)\n",
				repr->finger_position,
				iso_fmr_v030_get_finger_position_string(
				repr->finger_position));
		printf("\tRepresentation number: %u\n",
				repr->representation_number);
		printf("\tHorizontal image spatial sampling rate: %u (pixels per cm)\n",
				repr->sampling_rate_x);
		printf("\tVertical image spatial sampling rate: %u (pixels per cm)\n",
				repr->sampling_rate_y);
		printf("\tImpression type: %u (%s)\n",
				repr->impression_type,
				iso_fmr_v030_get_impression_type_string(
				repr->impression_type));
		printf("\tSize of scanned image in X direction: %u (pixels)\n",
				repr->size_x);
		printf("\tSize of scanned image in Y direction: %u (pixels)\n",
				repr->size_y);
		printf("\tMinutia field length: %u (bytes)\n",
				repr->minutia_field_length);
		printf("\tRidge ending type: %u (%s)\n",
				repr->ridge_ending_type,
				iso_fmr_v030_get_ridge_ending_type_string(
				repr->ridge_ending_type));

		printf("\tNumber of minutiae: %d\n",
				repr->number_minutiae);

		for (m = 0; m < repr->number_minutiae; m++) {
			struct iso_fmr_v030_minutia *minutia =
					&repr->minutiae[m];
			unsigned angle;

			printf("\n\tMinutia %d:\n", m);

			printf("\t\tType: %u (%s)\n", minutia->type,
					iso_fmr_v030_get_minutia_type_string(
					minutia->type));

			printf("\t\tX: %u\n", minutia->x);
			printf("\t\tY: %u\n", minutia->y);

			angle = minutia->angle * 140625;
			printf("\t\tAngle: %u (* 1.40625 deg = %u.%05u deg)\n",
					minutia->angle,
					angle / 100000, angle % 100000);

			if (repr->minutia_field_length == 5)
				continue;
			printf("\t\tQuality: %u", minutia->quality);
			switch (minutia->quality) {
			case 254: printf(" (not reported)\n"); break;
			case 255: printf(" (failed to compute)\n"); break;
			default:  printf("\n");
			}
		}

		printf("\n\tExtended data block length: %u bytes\n",
				repr->extended_data_block_length);
		if (repr->extended_data_block_length) {
			printf("\tExtended data block:\n");
			hexdump("\t\t", repr->extended_data_block,
					repr->extended_data_block_length);
		}
	}
}



static void usage(const char *comm)
{
	fprintf(stderr, "Usage: %s [-h] [-x|-b|-c] [NAME]\n", comm);
	fprintf(stderr, "where:\n");
	fprintf(stderr, "\t-h\tusage syntax (this message)\n");
	fprintf(stderr, "\t-f\tforce dumping decoded information (despite of an error)\n");
	fprintf(stderr, "\tNAME\t(optional) input file, stdin by default\n");
}

static char cache[8]; /* Cache header and version fields */
static int cache_used, cache_index;
static int getc_cached(void *context)
{
	FILE *fl = context;
	int c;

	if (cache_index >= sizeof(cache))
		return getc(fl);

	if (cache_used <= cache_index) {
		c = getc(fl);
		if (c < 0)
			return c;
		cache[cache_used++] = c;
	}

	return cache[cache_index++];
}

int main(int argc, char *argv[])
{
	int opt;
	FILE *fl = stdin;
	int force = 0;
	enum iso_fmr_v20_error v20_error;
	struct iso_fmr_v20 *v20_record;
	size_t bytes;

	while ((opt = getopt(argc, argv, "hf")) != -1) {
		switch (opt) {
		case 'f':
			force = 1;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (argc - optind == 1) {
		fl = fopen(argv[optind], "rb");
		if (!fl) {
			perror("Failed to open file");
			return 1;
		}
	} else if (argc - optind > 1) {
		usage(argv[0]);
		return 1;
	}

	v20_record = iso_fmr_v20_decode(getc_cached, fl,
			&v20_error, &bytes);
	if (v20_error == iso_fmr_v20_invalid_version) {
		struct iso_fmr_v030 *v030_record;
		enum iso_fmr_v030_error v030_error;

		cache_index = 0; /* Re-read the header... */

		v030_record = iso_fmr_v030_decode(getc_cached, fl,
			&v030_error, &bytes);
		if (v030_error)
			fprintf(stderr, "error: %s at byte %zu\n",
				iso_fmr_v030_get_error_string(v030_error),
				bytes);
		if (!v030_error || force)
			v030dump(v030_record);
		iso_fmr_v030_free(v030_record);
	} else {
		if (v20_error)
			fprintf(stderr, "error: %s at byte %zu\n",
				iso_fmr_v20_get_error_string(v20_error),
				bytes);
		if (!v20_error || force)
			v20dump(v20_record);
	}
	iso_fmr_v20_free(v20_record);

	if (fl != stdin)
		fclose(fl);

	return 0;
}
