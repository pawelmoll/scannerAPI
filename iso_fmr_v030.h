#ifndef __ISO_FMR_V030_H
#define __ISO_FMR_V030_H

#include <stdint.h>

enum iso_fmr_v030_error {
	__iso_fmr_v030_no_error,
	iso_fmr_v030_out_of_memory,
	iso_fmr_v030_premature_end_of_data,
	iso_fmr_v030_invalid_format_id,
	iso_fmr_v030_invalid_version,
	iso_fmr_v030_invalid_total_length,
	iso_fmr_v030_invalid_number_representations,
	iso_fmr_v030_invalid_device_certification_block_flag,
	iso_fmr_v030_invalid_representation_length,
	iso_fmr_v030_invalid_quality_value,
	iso_fmr_v030_invalid_finger_position,
	iso_fmr_v030_invalid_sampling_rate,
	iso_fmr_v030_invalid_impression_type,
	iso_fmr_v030_invalid_image_size,
	iso_fmr_v030_invalid_minutia_field_length,
	iso_fmr_v030_invalid_ridge_ending_type,
	iso_fmr_v030_invalid_number_minutiae,
	iso_fmr_v030_invalid_minutia_type,
	iso_fmr_v030_invalid_minutia_quality,
};

struct iso_fmr_v030_representation;
struct iso_fmr_v030_quality_blocks;
struct iso_fmr_v030_certification_block;
struct iso_fmr_v030_minutia;

struct iso_fmr_v030 {
	uint32_t format_id;
	uint32_t version;
	uint32_t total_length;
	uint16_t number_representations;
	uint8_t device_certification_block_flag;
	struct iso_fmr_v030_representation *representations;
};

struct iso_fmr_v030_representation {
	uint32_t representation_length;
	struct {
		uint16_t year;
		uint8_t month;
		uint8_t day;
		uint8_t hour;
		uint8_t minute;
		uint8_t second;
		uint16_t microsecond;
	} capture_data_time;
	uint8_t capture_device_technology_id;
	uint16_t capture_device_vendor_id;
	uint16_t capture_device_type_id;
	uint8_t number_quality_blocks;
	struct iso_fmr_v030_quality_block *quality_blocks;
	uint8_t number_certification_blocks; /* Not used when device_certification_block_flag == 0 */
	struct iso_fmr_v030_certification_block *certification_blocks;
	uint8_t finger_position;
	uint8_t representation_number;
	uint16_t sampling_rate_x;
	uint16_t sampling_rate_y;
	uint8_t impression_type;
	uint16_t size_x;
	uint16_t size_y;
	uint8_t minutia_field_length:4;
	uint8_t ridge_ending_type:4;
	uint8_t number_minutiae;
	struct iso_fmr_v030_minutia *minutiae;
	uint16_t extended_data_block_length;
        void *extended_data_block;
};

struct iso_fmr_v030_quality_block {
	uint8_t quality_value;
	uint16_t quality_vendor_id;
	uint16_t quality_algorithm_id;
};

struct iso_fmr_v030_certification_block {
	uint16_t certification_authority_id;
	uint8_t certification_scheme_id;
};

struct iso_fmr_v030_minutia {
	uint8_t type:2;
	uint16_t x:14;
	uint16_t y:14;
	uint8_t angle;
	uint8_t quality; /* Not used when minutia_field_length == 5 */
};

struct iso_fmr_v030 *iso_fmr_v030_decode(int (*getbyte)(void *context),
		void *context, enum iso_fmr_v030_error *error, size_t *bytes);

void iso_fmr_v030_free(struct iso_fmr_v030 *record);

const char *iso_fmr_v030_get_error_string(enum iso_fmr_v030_error error);
const char *iso_fmr_v030_get_device_technology_string(uint8_t device_technology);
const char *iso_fmr_v030_get_finger_position_string(uint8_t finger_position);
const char *iso_fmr_v030_get_impression_type_string(uint8_t impression_type);
const char *iso_fmr_v030_get_ridge_ending_type_string(uint8_t ridge_ending_type);
const char *iso_fmr_v030_get_minutia_type_string(uint8_t minutia_type);

#endif
