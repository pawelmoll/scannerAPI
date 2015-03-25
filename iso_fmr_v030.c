#include <stdlib.h>
#include <string.h>

#include "iso_fmr_v030.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

static const char *iso_fmr_v030_errors[] = {
	[iso_fmr_v030_out_of_memory] = "out of memory",
	[iso_fmr_v030_premature_end_of_data] = "premature end of data",
	[iso_fmr_v030_invalid_format_id] = "invalid format id",
	[iso_fmr_v030_invalid_version] = "invalid version",
	[iso_fmr_v030_invalid_total_length] = "invalid total length",
	[iso_fmr_v030_invalid_number_representations] =
			"invalid number of representations",
	[iso_fmr_v030_invalid_device_certification_block_flag] =
			"invalid device certification block flag",
	[iso_fmr_v030_invalid_representation_length] =
			"invalid representation length",
	[iso_fmr_v030_invalid_quality_value] = "invalid quality value",
	[iso_fmr_v030_invalid_finger_position] = "invalid finger position",
	[iso_fmr_v030_invalid_sampling_rate] = "invalid sampling rate",
	[iso_fmr_v030_invalid_impression_type] = "invalid impression type",
	[iso_fmr_v030_invalid_image_size] = "invalid image size",
	[iso_fmr_v030_invalid_minutia_field_length] =
			"invalid minutia field length",
	[iso_fmr_v030_invalid_ridge_ending_type] = "invalid ridge ending type",
	[iso_fmr_v030_invalid_number_minutiae] = "invalid number of minutiae",
	[iso_fmr_v030_invalid_minutia_type] = "invalid minutia type",
	[iso_fmr_v030_invalid_minutia_quality] = "invalid minutia quality",
};

const char *iso_fmr_v030_get_error_string(enum iso_fmr_v030_error error)
{
	return error < ARRAY_SIZE(iso_fmr_v030_errors) ?
			iso_fmr_v030_errors[error] : NULL;
}

static const char *iso_fmr_v030_device_technologies[] = {
	[0] = "unknown or unspecified",
	[1] = "white light optical TIR",
	[2] = "white light optical direct view on platen",
	[3] = "white light optical touchless",
	[4] = "monochromatic visible optical TIR",
	[5] = "monochromatic visible optical direct view on platen",
	[6] = "monochromatic visible optical touchless",
	[7] = "monochromatic IR optical TIR",
	[8] = "monochromatic IR direct view on platen",
	[9] = "monochromatic IR touchless",
	[10] = "multispectral optical TIR",
	[11] = "multispectral optical direct view on platen",
	[12] = "multispectral optical touchless",
	[13] = "electro luminescent",
	[14] = "semiconductor capacitive",
	[15] = "semiconductor RF",
	[16] = "semiconductor thermal",
	[17] = "pressure sensitive",
	[18] = "ultrasound",
	[19] = "mechanical",
	[20] = "glass fiber",
};

const char *iso_fmr_v030_get_device_technology_string(uint8_t device_technology)
{
	return device_technology < ARRAY_SIZE(iso_fmr_v030_device_technologies) ?
			iso_fmr_v030_device_technologies[device_technology] :
			NULL;
}

static const char *iso_fmr_v030_finger_positions[] = {
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
	[13] = "plain right four fingers",
	[14] = "plain left four fingers",
	[15] = "left thumb and right thumb",
	[40] = "right index and middle",
	[41] = "right middle and ring",
	[42] = "right ring and middle",
	[43] = "left index and middle",
	[44] = "left middle and ring",
	[45] = "left ring and middle",
	[46] = "right index and left index",
	[47] = "right index and middle and ring",
	[48] = "right middle and ring and little",
	[49] = "left index and middle and ring",
	[50] = "left middle and ring and little",
};

const char *iso_fmr_v030_get_finger_position_string(uint8_t finger_position)
{
	return finger_position < ARRAY_SIZE(iso_fmr_v030_finger_positions) ?
			iso_fmr_v030_finger_positions[finger_position] : NULL;
}

static const char *iso_fmr_v030_impression_types[] = {
	[0] = "live-scan plain",
	[1] = "live-scan rolled",
	[2] = "nonlive-scan plain",
	[3] = "nonlive-scan rolled",
	[4] = "latent impression",
	[5] = "latent tracing",
	[6] = "latent photo",
	[7] = "latent lift",
	[8] = "live-scan swipe",
	[9] = "vertical roll",
	[24] = "live-scan optical contactless plain",
	[28] = "other",
	[29] = "unknown",
};

const char *iso_fmr_v030_get_impression_type_string(uint8_t impression_type)
{
	return impression_type < ARRAY_SIZE(iso_fmr_v030_impression_types) ?
			iso_fmr_v030_impression_types[impression_type] : NULL;
}

static const char *iso_fmr_v030_ridge_ending_types[] = {
	[0] = "intersection of three valley bifurcation",
	[1] = "ridge skeleton endpoints",
};

const char *iso_fmr_v030_get_ridge_ending_type_string(uint8_t ridge_ending_type)
{
	return ridge_ending_type < ARRAY_SIZE(iso_fmr_v030_ridge_ending_types) ?
			iso_fmr_v030_ridge_ending_types[ridge_ending_type] :
			NULL;
}

static const char *iso_fmr_v030_minutia_types[] = {
	[0] = "other",
	[1] = "termination",
	[2] = "bifurcation",
};

const char *iso_fmr_v030_get_minutia_type_string(uint8_t minutia_type)
{
	return minutia_type < ARRAY_SIZE(iso_fmr_v030_minutia_types) ?
			iso_fmr_v030_minutia_types[minutia_type] : NULL;
}

static uint32_t iso_fmr_v030_get_be_uint(int size,
		int (*getbyte)(void *context), void *context,
		enum iso_fmr_v030_error *error, size_t *bytes)
{
	uint32_t val = 0;
	int byte;

	while (size--) {
		byte = getbyte(context);
		if (byte < 0) {
			*error = iso_fmr_v030_premature_end_of_data;
			return 0;
		}
		val |= (byte & 0xff) << (size * 8);
		(*bytes)++;
	}

	*error = 0;

	return val;
}

struct iso_fmr_v030 *iso_fmr_v030_decode(int (*getbyte)(void *context),
		void *context, enum iso_fmr_v030_error *error, size_t *bytes)
{
	struct iso_fmr_v030 *record;
	enum iso_fmr_v030_error dummy_error;
	size_t dummy_bytes;
	int r;

#define __get(field) \
		do { \
			field = iso_fmr_v030_get_be_uint(sizeof(field), \
					getbyte, context, error, bytes); \
			if (record->total_length && \
					*bytes > record->total_length) \
				*error = iso_fmr_v030_invalid_total_length; \
			if (*error) \
				goto out; \
		} while (0)

	if (!error)
		error = &dummy_error;
	if (!bytes)
		bytes = &dummy_bytes;

	*error = 0;
	*bytes = 0;

	record = malloc(sizeof(*record));
	if (!record) {
		*error = iso_fmr_v030_out_of_memory;
		goto out;
	}
	memset(record, 0, sizeof(*record));

	__get(record->format_id);
	if (record->format_id != 0x464d5200) {
		*error = iso_fmr_v030_invalid_format_id;
		goto out;
	}

	__get(record->version);
	if (record->version != 0x30333000) {
		*error = iso_fmr_v030_invalid_version;
		goto out;
	}

	__get(record->total_length);
	if (record->total_length < 54) {
		*error = iso_fmr_v030_invalid_total_length;
		goto out;
	}

	__get(record->number_representations);
	if (record->number_representations == 0) {
		*error = iso_fmr_v030_invalid_number_representations;
		goto out;
	}
	record->representations = malloc(sizeof(*record->representations) *
			record->number_representations);
	if (!record->representations) {
		*error = iso_fmr_v030_out_of_memory;
		goto out;
	}
	memset(record->representations, 0, sizeof(*record->representations) *
			record->number_representations);

	__get(record->device_certification_block_flag);
	if (record->device_certification_block_flag > 1) {
		*error = iso_fmr_v030_invalid_device_certification_block_flag;
		goto out;
	}

	for (r = 0; r < record->number_representations; r++) {
		struct iso_fmr_v030_representation *repr =
				&record->representations[r];
		uint8_t tmp8;
		int m;

		__get(repr->representation_length);
		if (repr->representation_length < 39) {
			*error = iso_fmr_v030_invalid_representation_length;
			goto out;
		}

		/* TODO: fix date/time format */
		__get(repr->capture_data_time.year);
		__get(repr->capture_data_time.month);
		__get(repr->capture_data_time.day);
		__get(repr->capture_data_time.hour);
		__get(repr->capture_data_time.minute);
		__get(repr->capture_data_time.second);
		__get(repr->capture_data_time.microsecond);

		__get(repr->capture_device_technology_id);
		__get(repr->capture_device_vendor_id);
		__get(repr->capture_device_type_id);

		__get(repr->number_quality_blocks);
		if (repr->number_quality_blocks) {
			int b;

			repr->quality_blocks = malloc(sizeof(*repr->quality_blocks) *
					repr->number_quality_blocks);
			if (!repr->quality_blocks) {
				*error = iso_fmr_v030_out_of_memory;
				goto out;
			}
			memset(repr->quality_blocks, 0,
					sizeof(*repr->quality_blocks) *
					repr->number_quality_blocks);

			for (b = 0; b < repr->number_quality_blocks; b++) {
				struct iso_fmr_v030_quality_block *block =
						&repr->quality_blocks[b];

				__get(block->quality_value);
				if (block->quality_value > 100 &&
						block->quality_value != 0xff) {
					*error = iso_fmr_v030_invalid_quality_value;
					goto out;
				}

				__get(block->quality_vendor_id);
				__get(block->quality_algorithm_id);
			}
		}

		if (record->device_certification_block_flag)
			__get(repr->number_certification_blocks);

		if (repr->number_certification_blocks) {
			int b;

			repr->certification_blocks = malloc(sizeof(*repr->certification_blocks) *
					repr->number_certification_blocks);
			if (!repr->certification_blocks) {
				*error = iso_fmr_v030_out_of_memory;
				goto out;
			}
			memset(repr->certification_blocks, 0,
					sizeof(*repr->certification_blocks) *
					repr->number_certification_blocks);

			for (b = 0; b < repr->number_certification_blocks; b++) {
				struct iso_fmr_v030_certification_block *block =
						&repr->certification_blocks[b];

				__get(block->certification_authority_id);
				__get(block->certification_scheme_id);
			}
		}

		__get(repr->finger_position);
		if (repr->finger_position >=
				ARRAY_SIZE(iso_fmr_v030_finger_positions) ||
				!iso_fmr_v030_finger_positions[repr->finger_position]) {
			*error = iso_fmr_v030_invalid_finger_position;
			goto out;
		}

		__get(repr->representation_number);

		__get(repr->sampling_rate_x);
		if (repr->sampling_rate_x < 98) {
			*error = iso_fmr_v030_invalid_sampling_rate;
			goto out;
		}
		__get(repr->sampling_rate_y);
		if (repr->sampling_rate_y < 98) {
			*error = iso_fmr_v030_invalid_sampling_rate;
			goto out;
		}

		__get(repr->impression_type);
		if (repr->impression_type >=
				ARRAY_SIZE(iso_fmr_v030_impression_types) ||
			!iso_fmr_v030_impression_types[repr->impression_type]) {
			*error = iso_fmr_v030_invalid_impression_type;
			goto out;
		}

		__get(repr->size_x);
		if (repr->size_x > 0x3fff) {
			*error = iso_fmr_v030_invalid_image_size;
			goto out;
		}
		__get(repr->size_y);
		if (repr->size_y > 0x3fff) {
			*error = iso_fmr_v030_invalid_image_size;
			goto out;
		}

		__get(tmp8);
		repr->minutia_field_length = tmp8 >> 4;
		if (repr->minutia_field_length != 5 &&
				repr->minutia_field_length != 6) {
			*error = iso_fmr_v030_invalid_minutia_field_length;
			goto out;
		}
		repr->ridge_ending_type = tmp8 & 0xf;
		if (repr->ridge_ending_type >=
				ARRAY_SIZE(iso_fmr_v030_ridge_ending_types)) {
			*error = iso_fmr_v030_invalid_ridge_ending_type;
			goto out;
		}

		__get(repr->number_minutiae);
		if (repr->number_minutiae < 1) {
			*error = iso_fmr_v030_invalid_number_minutiae;
			goto out;
		}
		repr->minutiae = malloc(sizeof(*repr->minutiae) *
				repr->number_minutiae);
		if (!repr->minutiae) {
			*error = iso_fmr_v030_out_of_memory;
			goto out;
		}

		for (m = 0; m < repr->number_minutiae; m++) {
			struct iso_fmr_v030_minutia *minutia =
					&repr->minutiae[m];
			uint16_t tmp16;

			__get(tmp16);
			minutia->type = tmp16 >> 14;
			if (minutia->type >=
					ARRAY_SIZE(iso_fmr_v030_minutia_types)) {
				*error = iso_fmr_v030_invalid_minutia_type;
				goto out;
			}

			minutia->x = tmp16 & 0x3fff;
			__get(tmp16);
			minutia->y = tmp16 & 0x3fff;
			__get(minutia->angle);

			if (repr->minutia_field_length == 5)
				continue;

			__get(minutia->quality);
			if (minutia->quality > 100 && minutia->quality < 254) {
				*error = iso_fmr_v030_invalid_minutia_quality;
				goto out;
			}
		}

		__get(repr->extended_data_block_length);
		if (repr->extended_data_block_length) {
			size_t size = repr->extended_data_block_length;
			unsigned char *b = malloc(size);

			if (!b) {
				*error = iso_fmr_v030_out_of_memory;
				goto out;
			}
			repr->extended_data_block = b;

			while (size--)
				__get(*b);
		}
	}

#undef __get

	if (*bytes < record->total_length)
		*error = iso_fmr_v030_invalid_total_length;

out:
	return record;
}

void iso_fmr_v030_free(struct iso_fmr_v030 *record)
{
	struct iso_fmr_v030_representation *repr;

	if (!record)
		return;

	for (repr = record->representations; record->number_representations;
			repr++, record->number_representations--) {
		free(repr->minutiae);
		if (repr->extended_data_block)
			free(repr->extended_data_block);
	}
	free(record->representations);
	free(record);
}
