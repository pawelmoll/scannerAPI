#include "scanner.h"

#include <exception>

Scanner::Scanner()
{
    int err = scanner_on();
    if (err)
        throw ScannerException("Failed to turn on the scanner", err);

    err = scanner_get_caps(&caps);
    if (err)
        throw ScannerException("Failed to obtaing caps", err);
}

Scanner::~Scanner()
{
    scanner_off();
}

const char *Scanner::getName()
{
    return caps.name;
}

Fingerprint *Scanner::getFingerprint(int timeout)
{
    int err = scanner_scan(timeout);

    if (err == -1)
        throw ScannerTimeout();

    if (err)
        throw ScannerException("Failed to scan", err);

    Fingerprint *fingerprint = new Fingerprint();

    if (caps.image) {
        int size = scanner_get_image(NULL, 0);

        if (size < 0)
            throw ScannerException("Zero sized image");

        unsigned char buffer[size];

        size = scanner_get_image((void *)buffer, size);
        if (size < 0)
            throw ScannerException("Failed to obtain the image", size);

        switch (caps.image_format) {
        case scanner_caps::scanner_image_gray_8bit:
            fingerprint->image = new FingerprintGreyscaleImage(caps.image_width, caps.image_height, buffer, size);
            break;
        default:
            throw ScannerException("Obtained unknown image format");
        }
    }

    if (caps.iso_template) {
        int size = scanner_get_iso_template(NULL, 0);

        if (size < 0)
            throw ScannerException("Zero sized template");

        unsigned char buffer[size];

        size = scanner_get_iso_template((void *)buffer, size);
        if (size < 0)
            throw ScannerException("Failed to obtain the template");

        try {
            fingerprint->minutiaeRecord = new FingerprintISOv20MinutiaeRecord(buffer, size);
        } catch (FingerprintMinutiaeRecordInvalidVersion &e) {
            fingerprint->minutiaeRecord = new FingerprintISOv030MinutiaeRecord(buffer, size);
        }
    }

    return fingerprint;
}
