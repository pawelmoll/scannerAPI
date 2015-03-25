#ifndef __ISO_FMR_V20_H
#define __ISO_FMR_V20_H

#include <stdint.h>

enum iso_fmr_v20_error {
	__iso_fmr_v20_no_error,
	iso_fmr_v20_out_of_memory,
	iso_fmr_v20_premature_end_of_data,
	iso_fmr_v20_invalid_format_id,
	iso_fmr_v20_invalid_version,
	iso_fmr_v20_invalid_total_length,
	iso_fmr_v20_invalid_reserved_byte,
	iso_fmr_v20_invalid_finger_position,
	iso_fmr_v20_invalid_impression_type,
	iso_fmr_v20_invalid_finger_quality,
	iso_fmr_v20_invalid_minutia_type,
	iso_fmr_v20_invalid_minutia_quality,
};

struct iso_fmr_v20_view;
struct iso_fmr_v20_minutia;

struct iso_fmr_v20 {
	uint32_t format_id;
	uint32_t version;
	uint32_t total_length;
	uint8_t capture_equip_cert:4;
	uint16_t capture_device_type_id:12;
	uint16_t size_x;
	uint16_t size_y;
	uint16_t resolution_x;
	uint16_t resolution_y;
	uint8_t number_views;
	struct iso_fmr_v20_view *views;
};

struct iso_fmr_v20_view {
	uint8_t finger_position;
	uint8_t representation_number:4;
	uint8_t impression_type:4;
	uint8_t finger_quality;
	uint8_t number_minutiae;
	struct iso_fmr_v20_minutia *minutiae;
	uint16_t extended_data_block_length;
	void *extended_data_block;
};

struct iso_fmr_v20_minutia {
	uint8_t type:2;
	uint16_t x:14;
	uint16_t y:14;
	uint8_t angle;
	uint8_t quality;
};

struct iso_fmr_v20 *iso_fmr_v20_decode(int (*getbyte)(void *context),
		void *context, enum iso_fmr_v20_error *error, size_t *bytes);

void iso_fmr_v20_free(struct iso_fmr_v20 *record);

const char *iso_fmr_v20_get_error_string(enum iso_fmr_v20_error error);
const char *iso_fmr_v20_get_finger_position_string(uint8_t finger_position);
const char *iso_fmr_v20_get_impression_type_string(uint8_t impression_type);
const char *iso_fmr_v20_get_minutia_type_string(uint8_t minutia_type);

#endif
