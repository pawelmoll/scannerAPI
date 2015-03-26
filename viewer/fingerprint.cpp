#include "fingerprint.h"

FingerprintGreyscaleImage::FingerprintGreyscaleImage(int width, int height, void *image, size_t size) : FingerprintImage(width, height)
{
    unsigned char *data = (unsigned char *)image;

    buffer.insert(buffer.end(), data, data + size);

    for (int i = 0; i < 256; i++)
        grayScale.append(qRgb(i, i, i));
}

FingerprintGreyscaleImage::~FingerprintGreyscaleImage()
{
}

QImage *FingerprintGreyscaleImage::getImage()
{
    QImage *image = new QImage(&buffer[0], width, height, QImage::Format_Indexed8);
    image->setColorTable(grayScale);

    return image;
}


struct GetByteContext {
    std::vector<unsigned char>::iterator it, end;
};

static int getByte(void *c)
{
    GetByteContext *context = (GetByteContext *)c;

    if (context->it == context->end)
        return -1;

    return *(context->it)++;
}

FingerprintISOv20MinutiaeRecord::FingerprintISOv20MinutiaeRecord(void *binary, size_t size)
{
    unsigned char *data = (unsigned char *)binary;
    std::vector<unsigned char> buffer;

    buffer.insert(buffer.end(), data, data + size);

    GetByteContext context = { buffer.begin(), buffer.end() };
    iso_fmr_v20_error err;
    size_t bytes;
    record = iso_fmr_v20_decode(getByte, &context, &err, &bytes);

    if (err == iso_fmr_v20_invalid_version)
        throw FingerprintMinutiaeRecordInvalidVersion();

    if (err)
        throw FingerprintMinutiaeRecordException(iso_fmr_v20_get_error_string(err), err);

    if (bytes != size)
        throw FingerprintMinutiaeRecordException("Extra data in the binary template");

    iso_fmr_v20_view *view = &record->views[0]; // FIXME: simplification - only first view considered

    for (int m = 0; m < view->number_minutiae; m++) {
        iso_fmr_v20_minutia *minutia = &view->minutiae[m];

        minutiae.push_back(FingerprintMinutia(minutia->x, minutia->y, minutia->angle * 140625));
    }
}


FingerprintISOv20MinutiaeRecord::~FingerprintISOv20MinutiaeRecord()
{
    iso_fmr_v20_free(record);
}

QString FingerprintISOv20MinutiaeRecord::getRecord()
{
    QString r, l;

    l.sprintf("Format identifier: 0x%08x (%c%c%c%c)\n",
            record->format_id,
            (record->format_id >> 24) & 0xff,
            (record->format_id >> 16) & 0xff,
            (record->format_id >> 8) & 0xff,
            (record->format_id >> 0) & 0xff);
    r.append(l);
    l.sprintf("Version: 0x%08x (%c%c%c%c)\n",
            record->version,
            (record->version >> 24) & 0xff,
            (record->version >> 16) & 0xff,
            (record->version >> 8) & 0xff,
            (record->version >> 0) & 0xff);
    r.append(l);
    l.sprintf("Total length: %u bytes\n", record->total_length);
    r.append(l);
    l.sprintf("Capture equipment certification: %u\n",
            record->capture_equip_cert);
    r.append(l);
    l.sprintf("Capture device type ID: %u\n",
            record->capture_device_type_id);
    r.append(l);
    l.sprintf("Image size in X: %u (pixels)\n", record->size_x);
    r.append(l);
    l.sprintf("Image size in Y: %u (pixels)\n", record->size_y);
    r.append(l);
    l.sprintf("Horizontal (X) resolution: %u (pixels per cm)\n",
            record->resolution_x);
    r.append(l);
    l.sprintf("Vertical (Y) resolution: %u (pixels per cm)\n",
            record->resolution_y);
    r.append(l);
    l.sprintf("Number of finger views: %u\n", record->number_views);
    r.append(l);

    for (int v = 0; v < record->number_views; v++) {
        struct iso_fmr_v20_view *view = &record->views[v];

        l.sprintf("\nView %d:\n", v);
        r.append(l);
        l.sprintf("    Finger position: %u (%s)\n",
                view->finger_position,
                iso_fmr_v20_get_finger_position_string(
                view->finger_position));
        r.append(l);
        l.sprintf("    Representation number: %u\n",
                view->representation_number);
        r.append(l);
        l.sprintf("    Impression type: %u (%s)\n",
                view->impression_type,
                iso_fmr_v20_get_impression_type_string(
                view->impression_type));
        r.append(l);
        l.sprintf("    Finger quality: %u\n", view->finger_quality);
        r.append(l);
        l.sprintf("    Number of minutiae: %d\n",
                view->number_minutiae);
        r.append(l);

        for (int m = 0; m < view->number_minutiae; m++) {
            struct iso_fmr_v20_minutia *minutia =
                    &view->minutiae[m];
            unsigned angle;

            l.sprintf("\n    Minutia %d:\n", m);
            r.append(l);

            l.sprintf("        Type: %u (%s)\n", minutia->type,
                    iso_fmr_v20_get_minutia_type_string(
                    minutia->type));
            r.append(l);

            l.sprintf("        X: %u\n", minutia->x);
            r.append(l);
            l.sprintf("        Y: %u\n", minutia->y);
            r.append(l);

            angle = minutia->angle * 140625;
            l.sprintf("        Angle: %u (* 1.40625 deg = %u.%05u deg)\n",
                    minutia->angle,
                    angle / 100000, angle % 100000);
            r.append(l);
            if (minutia->quality == 0)
                l.sprintf("        Quality: not reported\n");
            else
                l.sprintf("        Quality: %u\n",
                        minutia->quality);
            r.append(l);
        }

        l.sprintf("\n    Extended data block length: %u bytes\n",
                view->extended_data_block_length);
        r.append(l);
        if (view->extended_data_block_length) {
            l.sprintf("    Skipping extended data block...\n");
            r.append(l);
        }
    }

    return r;
}



FingerprintISOv030MinutiaeRecord::FingerprintISOv030MinutiaeRecord(void *binary, size_t size)
{
    unsigned char *data = (unsigned char *)binary;
    std::vector<unsigned char> buffer;

    buffer.insert(buffer.end(), data, data + size);

    GetByteContext context = { buffer.begin(), buffer.end() };
    iso_fmr_v030_error err;
    size_t bytes;
    record = iso_fmr_v030_decode(getByte, &context, &err, &bytes);

    if (err == iso_fmr_v030_invalid_version)
        throw FingerprintMinutiaeRecordInvalidVersion();

    if (err)
        throw FingerprintMinutiaeRecordException(iso_fmr_v030_get_error_string(err), err);

    if (bytes != size)
        throw FingerprintMinutiaeRecordException("Extra data in the binary template");

    iso_fmr_v030_representation *repr = &record->representations[0]; // FIXME: simplification - only first view considered

    for (int m = 0; m < repr->number_minutiae; m++) {
        iso_fmr_v030_minutia *minutia = &repr->minutiae[m];

        minutiae.push_back(FingerprintMinutia(minutia->x, minutia->y, minutia->angle * 140625));
    }
}


FingerprintISOv030MinutiaeRecord::~FingerprintISOv030MinutiaeRecord()
{
    iso_fmr_v030_free(record);
}

QString FingerprintISOv030MinutiaeRecord::getRecord()
{
    QString r, l;

    l.sprintf("Format identifier: 0x%08x (%c%c%c%c)\n",
            record->format_id,
            (record->format_id >> 24) & 0xff,
            (record->format_id >> 16) & 0xff,
            (record->format_id >> 8) & 0xff,
            (record->format_id >> 0) & 0xff);
    r.append(l);
    l.sprintf("Version: 0x%08x (%c%c%c%c)\n",
            record->version,
            (record->version >> 24) & 0xff,
            (record->version >> 16) & 0xff,
            (record->version >> 8) & 0xff,
            (record->version >> 0) & 0xff);
    r.append(l);
    l.sprintf("Total length: %u bytes\n", record->total_length);
    r.append(l);
    l.sprintf("Number of finger representations: %u\n",
            record->number_representations);
    r.append(l);
    l.sprintf("Device certification block: %u (%s)\n",
            record->device_certification_block_flag,
            record->device_certification_block_flag ?
            "present" : "not present");
    r.append(l);

    for (int v = 0; v < record->number_representations; v++) {
        struct iso_fmr_v030_representation *repr =
                &record->representations[v];

        l.sprintf("\nRepresentation %d:\n", v);
        r.append(l);
        l.sprintf("    Representation length: %u bytes\n",
                repr->representation_length);
        r.append(l);
        /* TODO: date and time */
        l.sprintf("    Capture device technology ID: %u (%s)\n",
                repr->capture_device_technology_id,
                iso_fmr_v030_get_device_technology_string(
                repr->capture_device_technology_id));
        r.append(l);
        l.sprintf("    Capture device vendor ID: 0x%04x (see www.ibia.org)\n",
                repr->capture_device_vendor_id);
        r.append(l);
        l.sprintf("    Capture device type ID: 0x%04x\n",
                repr->capture_device_type_id);
        r.append(l);
        l.sprintf("    Number of quality blocks: %u\n",
                repr->number_quality_blocks);
        r.append(l);
        for (int b = 0; b < repr->number_quality_blocks; b++) {
            struct iso_fmr_v030_quality_block *block =
                &repr->quality_blocks[b];

            l.sprintf("\n    Quality block %d:\n", b);
            r.append(l);
            l.sprintf("        Quality value: %u",
                    block->quality_value);
            r.append(l);
            if (block->quality_value == 255)
                l.sprintf(" (failed to calculate)\n");
            else
                l.sprintf("\n");
            r.append(l);
            l.sprintf("        Quality vendor ID: 0x%04x (see www.ibia.org)\n",
                    block->quality_vendor_id);
            r.append(l);
            l.sprintf("        Quality algorithm ID: 0x%04x\n",
                    block->quality_algorithm_id);
            r.append(l);
        }
        if (record->device_certification_block_flag) {
            l.sprintf("    Number of certification blocks: %u\n",
                    repr->number_certification_blocks);
            r.append(l);
            for (int b = 0; b < repr->number_certification_blocks; b++) {
                struct iso_fmr_v030_certification_block *block =
                    &repr->certification_blocks[b];

                l.sprintf("\n    Certification block %d:\n", b);
                r.append(l);
                l.sprintf("        Certification authority ID: 0x%04x (see www.ibia.org)\n",
                        block->certification_authority_id);
                r.append(l);
                l.sprintf("        Certification scheme ID: 0x%02x\n",
                        block->certification_scheme_id);
                r.append(l);
            }
        }
        l.sprintf("    Finger position: %u (%s)\n",
                repr->finger_position,
                iso_fmr_v030_get_finger_position_string(
                repr->finger_position));
        r.append(l);
        l.sprintf("    Representation number: %u\n",
                repr->representation_number);
        r.append(l);
        l.sprintf("    Horizontal image spatial sampling rate: %u (pixels per cm)\n",
                repr->sampling_rate_x);
        r.append(l);
        l.sprintf("    Vertical image spatial sampling rate: %u (pixels per cm)\n",
                repr->sampling_rate_y);
        r.append(l);
        l.sprintf("    Impression type: %u (%s)\n",
                repr->impression_type,
                iso_fmr_v030_get_impression_type_string(
                repr->impression_type));
        r.append(l);
        l.sprintf("    Size of scanned image in X direction: %u (pixels)\n",
                repr->size_x);
        r.append(l);
        l.sprintf("    Size of scanned image in Y direction: %u (pixels)\n",
                repr->size_y);
        r.append(l);
        l.sprintf("    Minutia field length: %u (bytes)\n",
                repr->minutia_field_length);
        r.append(l);
        l.sprintf("    Ridge ending type: %u (%s)\n",
                repr->ridge_ending_type,
                iso_fmr_v030_get_ridge_ending_type_string(
                repr->ridge_ending_type));
        r.append(l);

        l.sprintf("    Number of minutiae: %d\n",
                repr->number_minutiae);
        r.append(l);

        for (int m = 0; m < repr->number_minutiae; m++) {
            struct iso_fmr_v030_minutia *minutia =
                    &repr->minutiae[m];
            unsigned angle;

            l.sprintf("\n    Minutia %d:\n", m);
            r.append(l);

            l.sprintf("        Type: %u (%s)\n", minutia->type,
                    iso_fmr_v030_get_minutia_type_string(
                    minutia->type));
            r.append(l);

            l.sprintf("        X: %u\n", minutia->x);
            r.append(l);
            l.sprintf("        Y: %u\n", minutia->y);
            r.append(l);

            angle = minutia->angle * 140625;
            l.sprintf("        Angle: %u (* 1.40625 deg = %u.%05u deg)\n",
                    minutia->angle,
                    angle / 100000, angle % 100000);
            r.append(l);

            if (repr->minutia_field_length == 5)
                continue;
            l.sprintf("        Quality: %u", minutia->quality);
            r.append(l);
            switch (minutia->quality) {
            case 254: l.sprintf(" (not reported)\n"); break;
            case 255: l.sprintf(" (failed to compute)\n"); break;
            default:  l.sprintf("\n");
            }
            r.append(l);
        }

        l.sprintf("\n    Extended data block length: %u bytes\n",
                repr->extended_data_block_length);
        r.append(l);
        if (repr->extended_data_block_length) {
            l.sprintf("    Skipping extended data block...\n");
            r.append(l);
        }
    }

    return r;
}
