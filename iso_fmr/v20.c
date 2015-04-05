#include <stdlib.h>
#include <string.h>

#include "v20.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

static const char *iso_fmr_v20_errors[] = {
	[iso_fmr_v20_out_of_memory] = "out of memory",
	[iso_fmr_v20_premature_end_of_data] = "premature end of data",
	[iso_fmr_v20_invalid_format_id] = "invalid format id",
	[iso_fmr_v20_invalid_version] = "invalid version",
	[iso_fmr_v20_invalid_total_length] = "invalid total length",
	[iso_fmr_v20_invalid_reserved_byte] = "invalid reserved byte",
	[iso_fmr_v20_invalid_finger_position] = "invalid finger position",
	[iso_fmr_v20_invalid_impression_type] = "invalid impression type",
	[iso_fmr_v20_invalid_finger_quality] = "invalid finger quality",
	[iso_fmr_v20_invalid_minutia_type] = "invalid minutia type",
	[iso_fmr_v20_invalid_minutia_quality] = "invalid minutia quality",
};

const char *iso_fmr_v20_get_error_string(enum iso_fmr_v20_error error)
{
	return error < ARRAY_SIZE(iso_fmr_v20_errors) ?
			iso_fmr_v20_errors[error] : NULL;
}

static const char *iso_fmr_v20_finger_positions[] = {
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

const char *iso_fmr_v20_get_finger_position_string(uint8_t finger_position)
{
	return finger_position < ARRAY_SIZE(iso_fmr_v20_finger_positions) ?
			iso_fmr_v20_finger_positions[finger_position] : NULL;
}

static const char *iso_fmr_v20_impression_types[] = {
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

const char *iso_fmr_v20_get_impression_type_string(uint8_t impression_type)
{
	return impression_type < ARRAY_SIZE(iso_fmr_v20_impression_types) ?
			iso_fmr_v20_impression_types[impression_type] : NULL;
}

static const char *iso_fmr_v20_minutia_types[] = {
	[0] = "other",
	[1] = "termination",
	[2] = "bifurcation",
};

const char *iso_fmr_v20_get_minutia_type_string(uint8_t minutia_type)
{
	return minutia_type < ARRAY_SIZE(iso_fmr_v20_minutia_types) ?
			iso_fmr_v20_minutia_types[minutia_type] : NULL;
}

static uint32_t iso_fmr_v20_get_be_uint(int size,
		int (*getbyte)(void *context), void *context,
		enum iso_fmr_v20_error *error, size_t *bytes)
{
	uint32_t val = 0;
	int byte;

	while (size--) {
		byte = getbyte(context);
		if (byte < 0) {
			*error = iso_fmr_v20_premature_end_of_data;
			return 0;
		}
		val |= (byte & 0xff) << (size * 8);
		(*bytes)++;
	}

	*error = 0;

	return val;
}

struct iso_fmr_v20 *iso_fmr_v20_decode(int (*getbyte)(void *context),
		void *context, enum iso_fmr_v20_error *error, size_t *bytes)
{
	struct iso_fmr_v20 *record;
	enum iso_fmr_v20_error dummy_error;
	size_t dummy_bytes;
	uint16_t tmp16;
	uint8_t reserved;
	int v;

#define __get(field) \
		do { \
			field = iso_fmr_v20_get_be_uint(sizeof(field), \
					getbyte, context, error, bytes); \
			if (record->total_length && \
					*bytes > record->total_length) \
				*error = iso_fmr_v20_invalid_total_length; \
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
		*error = iso_fmr_v20_out_of_memory;
		goto out;
	}
	memset(record, 0, sizeof(*record));

	__get(record->format_id);
	if (record->format_id != 0x464d5200) {
		*error = iso_fmr_v20_invalid_format_id;
		goto out;
	}

	__get(record->version);
	if (record->version != 0x20323000) {
		*error = iso_fmr_v20_invalid_version;
		goto out;
	}

	__get(record->total_length);
	if (record->total_length < 24) {
		*error = iso_fmr_v20_invalid_total_length;
		goto out;
	}

	__get(tmp16);
	record->capture_equip_cert = tmp16 >> 12;
	record->capture_device_type_id = tmp16 & 0xfff;

	__get(record->size_x);
	__get(record->size_y);
	__get(record->resolution_x);
	__get(record->resolution_y);

	__get(record->number_views);
	record->views = malloc(sizeof(*record->views) * record->number_views);
	if (!record->views) {
		*error = iso_fmr_v20_out_of_memory;
		goto out;
	}
	memset(record->views, 0, sizeof(*record->views) * record->number_views);

	__get(reserved);
	if (reserved != 0) {
		*error = iso_fmr_v20_invalid_reserved_byte;
		goto out;
	}

	for (v = 0; v < record->number_views; v++) {
		struct iso_fmr_v20_view *view = &record->views[v];
		uint8_t tmp8;
		int m;

		__get(view->finger_position);
		if (view->finger_position >=
				ARRAY_SIZE(iso_fmr_v20_finger_positions)) {
			*error = iso_fmr_v20_invalid_finger_position;
			goto out;
		}

		__get(tmp8);
		view->representation_number = tmp8 >> 4;
		view->impression_type = tmp8 & 0xf;
		if (view->impression_type >=
				ARRAY_SIZE(iso_fmr_v20_impression_types)) {
			*error = iso_fmr_v20_invalid_impression_type;
			goto out;
		}

		__get(view->finger_quality);
		if (view->finger_quality > 100) {
			*error = iso_fmr_v20_invalid_finger_quality;
			goto out;
		}

		__get(view->number_minutiae);
		view->minutiae = malloc(sizeof(*view->minutiae) *
				view->number_minutiae);
		if (!view->minutiae) {
			*error = iso_fmr_v20_out_of_memory;
			goto out;
		}
		memset(view->minutiae, 0, sizeof(*view->minutiae) *
				view->number_minutiae);

		for (m = 0; m < view->number_minutiae; m++) {
			struct iso_fmr_v20_minutia *minutia =
					&view->minutiae[m];

			__get(tmp16);
			minutia->type = tmp16 >> 14;
			if (minutia->type >=
					ARRAY_SIZE(iso_fmr_v20_minutia_types)) {
				*error = iso_fmr_v20_invalid_minutia_type;
				goto out;
			}

			minutia->x = tmp16 & 0x3fff;
			__get(tmp16);
			minutia->y = tmp16 & 0x3fff;
			__get(minutia->angle);
			__get(minutia->quality);
			if (minutia->quality > 100) {
				*error = iso_fmr_v20_invalid_minutia_quality;
				goto out;
			}
		}

		__get(view->extended_data_block_length);
		if (view->extended_data_block_length) {
			size_t size = view->extended_data_block_length;
			unsigned char *b = malloc(size);

			if (!b) {
				*error = iso_fmr_v20_out_of_memory;
				goto out;
			}
			view->extended_data_block = b;
			memset(view->extended_data_block, 0, size);
 
			while (size--)
				__get(*b);
		}
	}

#undef __get

	if (*bytes < record->total_length)
		*error = iso_fmr_v20_invalid_total_length;

out:
	return record;
}

void iso_fmr_v20_free(struct iso_fmr_v20 *record)
{
	struct iso_fmr_v20_view *view;

	if (!record)
		return;

	for (view = record->views; record->number_views;
			view++, record->number_views--) {
		free(view->minutiae);
		if (view->extended_data_block)
			free(view->extended_data_block);
	}
	free(record->views);
	free(record);
}
