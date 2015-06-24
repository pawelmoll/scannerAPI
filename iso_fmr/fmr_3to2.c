#include <byteswap.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "v20.h"
#include "v030.h"


static void usage(const char *comm)
{
	fprintf(stderr, "Usage: %s [-h] [NAME030] [NAME20]\n", comm);
	fprintf(stderr, "where:\n");
	fprintf(stderr, "\t-h\tusage syntax (this message)\n");
	fprintf(stderr, "\tNAME030\t(optional) input FMR v030 file, stdin by default\n");
	fprintf(stderr, "\tNAME20\t(optional) input FMR v20 file, stdout by default\n");
}

int main(int argc, char *argv[])
{
	int opt;
	FILE *in = stdin;
	FILE *out = stdout;
	struct iso_fmr_v030 *v030;
	enum iso_fmr_v030_error v030_error;
	struct iso_fmr_v20 *v20;
	size_t bytes;
	int i;

	while ((opt = getopt(argc, argv, "h")) != -1) {
		switch (opt) {
		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (argc - optind > 0 && argc - optind <= 2) {
		in = fopen(argv[optind], "rb");
		if (!in) {
			perror("Failed to open input file");
			return 1;
		}
		if (argc - optind == 2) {
			out = fopen(argv[optind + 1], "wb");
			if (!in) {
				perror("Failed to open output file");
				return 1;
			}
		}
	} else if (argc - optind > 2) {
		usage(argv[0]);
		return 1;
	}

	v030 = iso_fmr_v030_decode((int (*)(void *))getc, in,
			&v030_error, &bytes);
	if (v030_error) {
		fprintf(stderr, "error: %s at byte %zu\n",
				iso_fmr_v030_get_error_string(v030_error),
				bytes);
		return 1;
	}

	v20 = iso_fmr_v20_init();
	if (!v20) {
		fprintf(stderr, "error: out of memory for V20 record\n");
		return 1;
	}
	if (v030->number_representations > 255) {
		fprintf(stderr, "error: V20 records can only carry 255 views (representations)\n");
		return 1;
	}
	for (i = 0; i < v030->number_representations; i++) {
		struct iso_fmr_v030_representation *v030_repr =
			&v030->representations[i];
		struct iso_fmr_v20_view *v20_view = iso_fmr_v20_add_view(v20,
				0, NULL);
		int j;

		if (!v20_view) {
			fprintf(stderr, "error: out of memory for V20 view\n");
			return 1;
		}

		if (v20->size_x && (v20->size_x != v030_repr->size_x ||
				v20->size_y != v030_repr->size_y ||
				v20->resolution_x != v030_repr->sampling_rate_x ||
				v20->resolution_y != v030_repr->sampling_rate_y)) {
			fprintf(stderr, "error: V20 records can only carry views of one size\n");
			return 1;
		} else if (!v20->size_x) {
			v20->size_x = v030_repr->size_x;
			v20->size_y = v030_repr->size_y;
			v20->resolution_x = v030_repr->sampling_rate_x;
			v20->resolution_y = v030_repr->sampling_rate_y;
		}

		if (!iso_fmr_v20_get_finger_position_string(v030_repr->finger_position)) {
			fprintf(stderr, "error: Finger position '%s' incompatible with V20\n",
					iso_fmr_v030_get_finger_position_string(v030_repr->finger_position));
			return 1;
		}
		v20_view->finger_position = v030_repr->finger_position;
		v20_view->representation_number = v030_repr->representation_number;
		if (!iso_fmr_v20_get_impression_type_string(v030_repr->impression_type)) {
			fprintf(stderr, "error: Impression type '%s' incompatible with V20\n",
					iso_fmr_v030_get_impression_type_string(v030_repr->impression_type));
			return 1;
		}
		/* V20 finger quality will be 0, we don't convert V030 data */
		for (j = 0; j < v030_repr->number_minutiae; j++) {
			struct iso_fmr_v030_minutia *v030_minutia =
				&v030_repr->minutiae[j];
			struct iso_fmr_v20_minutia *v20_minutia =
				iso_fmr_v20_add_minutia(v20, v20_view);

			if (!v20_minutia) {
				fprintf(stderr, "error: out of memory for V20 minutia\n");
				return 1;
			}

			v20_minutia->type = v030_minutia->type;
			v20_minutia->x = v030_minutia->x;
			v20_minutia->y = v030_minutia->y;
			v20_minutia->angle = v030_minutia->angle;
			if (v030_repr->minutia_field_length == 6 &&
					v030_minutia->quality < 254)
				v20_minutia->quality = v030_minutia->quality;
		}
	}

	if (iso_fmr_v20_encode(v20, (int (*)(int, void *))putc, out) < 0) {
		perror("failed to write output file");
		return 1;
	}

	iso_fmr_v030_free(v030);
	iso_fmr_v20_free(v20);

	if (in != stdin)
		fclose(in);
	if (out != stdout)
		fclose(out);

	return 0;
}
