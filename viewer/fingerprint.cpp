#include <iostream>

#include <QTextStream>

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


FingerprintMinutiaeRecord::FingerprintMinutiaeRecord(void *binary, size_t size)
{
    this->binary = new char[size];
    memcpy((void *)this->binary, binary, size);
    this->size = size;
}

FingerprintMinutiaeRecord::~FingerprintMinutiaeRecord()
{
    delete(binary);
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

FingerprintISOv20MinutiaeRecord::FingerprintISOv20MinutiaeRecord(void *binary, size_t size) :
    FingerprintMinutiaeRecord(binary, size)
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

QString FingerprintISOv20MinutiaeRecord::getHtml()
{
    QString r, l;
    QTextStream h(&r);

    h << "<table>";
    h << "<tr><td>Format identifier:</td>"
      << "<td>" << l.sprintf("0x%08x", record->format_id)
      << " (<tt>" << (char)((record->format_id >> 24) & 0xff)
      << (char)((record->format_id >> 16) & 0xff)
      << (char)((record->format_id >> 8) & 0xff)
      << (char)((record->format_id >> 0) & 0xff) << "</tt>)</td></tr>";
    h << "<tr><td>Version:</td>"
      << "<td>" << l.sprintf("0x%08x", record->version)
      << " (<tt>"<< (char)((record->version >> 24) & 0xff)
      << (char)((record->version >> 16) & 0xff)
      << (char)((record->version >> 8) & 0xff)\
      << (char)((record->version >> 0) & 0xff) << "</tt>)</td></tr>";
    h << "<tr><td>Total length:</td><td>"
      << record->total_length << " (bytes)</td></tr>";
    h << "<tr><td>Capture equipment certification:</td><td>"
      << record->capture_equip_cert << "</td></tr>";
    h << "<tr><td>Capture device type ID:</td><td>"
      << record->capture_device_type_id << "</td></tr>";
    h << "<tr><td>Image size in X:</td><td>"
      << record->size_x << " (pixels)</td></tr>";
    h << "<tr><td>Image size in Y:</td><td>"
      << record->size_y << " (pixels)</td></tr>";
    h << "<tr><td>Horizontal (X) resolution:</td><td>"
      << record->resolution_x << " (pixels per cm)</td></tr>";
    h << "<tr><td>Vertical (Y) resolution:</td><td>"
      << record->resolution_y << " (pixels per cm)</td></tr>";
    h << "<tr><td>Number of finger views:</td><td>"
      << record->number_views << "</td></tr>";
    h << "</table>";

    for (int v = 0; v < record->number_views; v++) {
        struct iso_fmr_v20_view *view = &record->views[v];


        h << "<table style='margin: 20px 0 0 20px;'>";
        h << "<tr><td colspan=2><center><b><a name='view" << v << "'>"
          << "View " << v << "</a></center></td></tr>";
        h << "<tr><td>Finger position:</td><td>"
          << view->finger_position << " ("
          << iso_fmr_v20_get_finger_position_string(view->finger_position)
          << ")</td></tr>";
        h << "<tr><td>Representation number:</td><td>"
          << view->representation_number << "</td></tr>";
        h << "<tr><td>Impression type:</td>"
          << "<td>" << view->impression_type << " ("
          << iso_fmr_v20_get_impression_type_string(view->impression_type)
          << ")</td></tr>";
        h << "<tr><td>Finger quality:</td><td>"
          << view->finger_quality << "</td></tr>";
        h << "<tr><td>Number of minutiae:</td><td>"
          << view->number_minutiae << "</td></tr>";
        h << "</table>";

        for (int m = 0; m < view->number_minutiae; m++) {
            struct iso_fmr_v20_minutia *minutia =
                    &view->minutiae[m];
            unsigned angle;

            h << "<table style='margin: 20px 0 0 40px;'>";
            h << "<tr><td colspan=2><center><b>"
              << "<a style='color: black; text-decoration: none;' "
              << "href='minutia://fmr.v20:" << v << "/" << m << "' "
              << "name='view" << v << "minutia" << m << "'>"
              << "Minutia " << m << "</a></b></center></td></tr>";
            h << "<tr><td>Type:</td><td>" << minutia->type << " ("
              << iso_fmr_v20_get_minutia_type_string(minutia->type)
              << ")</td></tr>";
            h << "<tr><td>X:</td><td>" << minutia->x << "</td></tr>";
            h << "<tr><td>Y:</td><td>" << minutia->y << "</td></tr>";

            angle = minutia->angle * 140625;
            h << "<tr><td>Angle:</td><td>"
              << minutia->angle << " (* 1.40625 deg = "
              << l.sprintf("%u.%05u", angle / 100000, angle % 100000)
              << " deg)</td></tr>";
            if (minutia->quality == 0)
                h << "<tr><td>Quality:</td><td>not reported</td></tr>";
            else
                h << "<tr><td>Quality:</td><td>"
                  << minutia->quality << "</td></tr>";
            h << "</table>";
        }

        h << "<table style='margin: 20px;'>";
        h << "<tr><td>Extended data block length:</td>"
          << "<td>" << view->extended_data_block_length
          << " (bytes)</td></tr>";
        if (view->extended_data_block_length)
            h << "<tr><td colspan=2><i>Skipping extended data block..."
              << "</i></td></tr>";
        h << "</table>";
    }

    return r;
}



FingerprintISOv030MinutiaeRecord::FingerprintISOv030MinutiaeRecord(void *binary, size_t size) :
    FingerprintMinutiaeRecord(binary, size)
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

QString FingerprintISOv030MinutiaeRecord::getHtml()
{
    QString r, l;
    QTextStream h(&r);

    h << "<table>";
    h << "<tr><td>Format identifier:</td>"
      << "<td>" << l.sprintf("0x%08x", record->format_id)
      << " (<tt>" << (char)((record->format_id >> 24) & 0xff)
      << (char)((record->format_id >> 16) & 0xff)
      << (char)((record->format_id >> 8) & 0xff)
      << (char)((record->format_id >> 0) & 0xff) << "</tt>)</td></tr>";
    h << "<tr><td>Version:</td>"
      << "<td>" << l.sprintf("0x%08x", record->version)
      << " (<tt>"<< (char)((record->version >> 24) & 0xff)
      << (char)((record->version >> 16) & 0xff)
      << (char)((record->version >> 8) & 0xff)
      << (char)((record->version >> 0) & 0xff) << "</tt>)</td></tr>";
    h << "<tr><td>Total length:</td><td>"
      << record->total_length << " (bytes)</td></tr>";
    h << "<tr><td>Number of finger representations:</td><td>"
      << record->number_representations << "</td></tr>";
    h << "<tr><td>Device certification block:</td><td>"
      << record->device_certification_block_flag
      << (record->device_certification_block_flag ?
         " (present)" : " (not present)") << "</td></tr>";
    h << "</table>";

    for (int v = 0; v < record->number_representations; v++) {
        struct iso_fmr_v030_representation *repr =
                &record->representations[v];

        h << "<table style='margin: 20px 0 0 20px;'>";
        h << "<tr><td colspan=2><center><b><a name='view" << v << "'>"
          << "Representation " << v << "</a></center></td></tr>";
        h << "<tr><td>Representation length:</td><td>"
          << repr->representation_length << " (bytes)</td></tr>";
        /* TODO: date and time */
        h << "<tr><td>Capture device technology ID:</td>"
          << "<td>" << repr->capture_device_technology_id << " ("
          << iso_fmr_v030_get_device_technology_string(repr->capture_device_technology_id)
          << ")</td></tr>";
        h << "<tr><td>Capture device vendor ID:</td>"
          << "<td>" << l.sprintf("0x%04x", repr->capture_device_vendor_id)
          << " (see <a href='http://www.ibia.org'>www.ibia.org</a>)</td></tr>";
        h << "<tr><td>Capture device type ID:</td>"
          << "<td>" << l.sprintf("0x%04x", repr->capture_device_type_id) << "</td></tr>";
        h << "<tr><td>Number of quality blocks:</td>"
          << "<td>" << repr->number_quality_blocks << "</td></tr>";

        if (repr->number_quality_blocks)
            h << "</table>";
        for (int b = 0; b < repr->number_quality_blocks; b++) {
            struct iso_fmr_v030_quality_block *block =
                &repr->quality_blocks[b];

            h << "<table style='margin: 20px 0 0 40px;'>";
            h << "<tr><td colspan=2><center><b><a name='view" << v << "qualityblock" << b << "'>"
              << "Quality block " << b << "</a></b></center></td></tr>";
            h << "<tr><td>Quality value:</td><td>" << block->quality_value;
            if (block->quality_value == 255)
                h << " (failed to calculate)";
            h << ("</td></tr>");
            h << "<tr><td>Quality vendor ID:</td>"
              << "<td>" << l.sprintf("0x%04x", block->quality_vendor_id)
              << " (see <a href='http://www.ibia.org>)www.ibia.org</a>)</td></tr>";
            h << "<tr><td>Quality algorithm ID:</td>"
              << "<td>" << l.sprintf("0x%04x", block->quality_algorithm_id) << "</td></tr>";
            h << "</table>";
        }
        if (repr->number_quality_blocks)
            h << "<table style='margin: 20px 0 0 20px;'>";

        if (record->device_certification_block_flag) {
            h << "<tr><td>Number of certification blocks:</td><td>"
              << repr->number_certification_blocks << "</td></tr>";
            if (repr->number_certification_blocks)
                h << "</table>";

            for (int b = 0; b < repr->number_certification_blocks; b++) {
                struct iso_fmr_v030_certification_block *block =
                    &repr->certification_blocks[b];

                h << "<table style='margin: 20px 0 0 40px;'>";
                h << "<tr><td colspan=2><center><b><a name='view" << v << "certificationblock" << b << "'>"
                  << "Certification block " << b << "</a></b></center></td></tr>";
                h << "<tr><td>Certification authority ID:</td>"
                  << "<td>"<< l.sprintf("0x%04x", block->certification_authority_id)
                  << " (see <a href='http://www.ibia.org>)www.ibia.org</a>)</td></tr>";
                h << "<tr><td>Certification scheme ID:</td>"
                  << "<td>" << l.sprintf("0x%02x", block->certification_scheme_id)
                  << "</td></tr>";
                h << "</table>";
            }
            if (repr->number_certification_blocks)
                h << "<table style='margin: 20px 0 0 20px;'>";
        }

        h << "<tr><td>Finger position:</td>"
          << "<td>" << repr->finger_position << " ("
          << iso_fmr_v030_get_finger_position_string(repr->finger_position)
          << ")</td></tr>";
        h << "<tr><td>Representation number:</td>"
          << "<td>" << repr->representation_number << "</td></tr>";
        h << "<tr><td>Horizontal image spatial sampling rate:</td>"
          << "<td>" <<  repr->sampling_rate_x << " (pixels per cm)</td></tr>";
        h << "<tr><td>Vertical image spatial sampling rate:</td>"
          << "<td>" <<  repr->sampling_rate_y << " (pixels per cm)</td></tr>";
        h << "<tr><td>Impression type:</td>"
          << "<td>" << repr->impression_type << " ("
          << iso_fmr_v030_get_impression_type_string(repr->impression_type)
          << ")</td></tr>";
        h << "<tr><td>Size of scanned image in X direction:</td>"
          << "<td>" << repr->size_x << " (pixels)</td></tr>";
        h << "<tr><td>Size of scanned image in Y direction:</td>"
          << "<td>" << repr->size_y << " (pixels)</td></tr>";
        h << "<tr><td>Minutia field length:</td>"
          << "<td>" << repr->minutia_field_length << " (bytes)</td></tr>";
        h << "<tr><td>Ridge ending type:</td>"
          << "<td>" << repr->ridge_ending_type << " ("
          << iso_fmr_v030_get_ridge_ending_type_string(repr->ridge_ending_type)
          << ")</td></tr>";
        h << "<tr><td>Number of minutiae:</td>"
          << "<td>" << repr->number_minutiae << "</td></tr>";
        h << "</table>";

        for (int m = 0; m < repr->number_minutiae; m++) {
            struct iso_fmr_v030_minutia *minutia =
                    &repr->minutiae[m];
            unsigned angle;

            h << "<table style='margin: 20px 0 0 40px;'>";
            h << "<tr><td colspan=2><center><b>"
              << "<a style='color: black; text-decoration: none;' "
              << "href='minutia://fmr.v030:" << v << "/" << m << "' "
              << "name='view" << v << "minutia" << m << "'>"
              << "Minutia " << m << "</a></b></center></td></tr>";
            h << "<tr><td>Type:</td><td>" << minutia->type << " ("
              << iso_fmr_v030_get_minutia_type_string(minutia->type)
              << ")</td></tr>";
            h << "<tr><td>X:</td><td>" << minutia->x << "</td></tr>";
            h << "<tr><td>Y:</td><td>" << minutia->y << "</td></tr>";

            angle = minutia->angle * 140625;
            h << "<tr><td>Angle:</td><td>"
              << minutia->angle << " (* 1.40625 deg = "
              << l.sprintf("%u.%05u", angle / 100000, angle % 100000)
              << " deg)</td></tr>";

            if (repr->minutia_field_length == 6) {
                h << "<tr><td>Quality:</td><td>" << minutia->quality;
                switch (minutia->quality) {
                case 254: h << " (not reported)"; break;
                case 255: h << " (failed to compute)"; break;
                }
                h << "</td></tr>";
            }
            h << "</table>";
        }

        h << "<table style='margin: 20px;'>";
        h << "<tr><td>Extended data block length:</td>"
          << "<td>" << repr->extended_data_block_length
          << " (bytes)</td></tr>";
        if (repr->extended_data_block_length)
            h << "<tr><td colspan=2><i>Skipping extended data block..."
              << "</i></td></tr>";
        h << "</table>";
    }

    return r;
}
