#include "scanner.h"

#include <exception>



ScannersList::ScannersList()
{
    int err = scanner_init();
    if (err)
        throw ScannerException("Failed to initialise scanner API", err);

    int number;
    const char **list = scanner_list(&number);
    for (int i = 0; i < number; i++)
        names.push_back(list[i]);
}

ScannersList &ScannersList::getScannersList()
{
    static ScannersList list;

    return list;
}



Scanner::Scanner(const char *name)
{
    int err;

    scanner = scanner_get(name);
    if (!scanner)
        throw ScannerException("Failed to get a scanner");

    err = scanner_on(scanner);
    if (err)
        throw ScannerException("Failed to turn on the scanner", err);

    err = scanner_get_caps(scanner, &caps);
    if (err)
        throw ScannerException("Failed to obtaing caps", err);
}

Scanner::~Scanner()
{
    scanner_off(scanner);
    scanner_put(scanner);
}

const char *Scanner::getName()
{
    return caps.name;
}

Fingerprint *Scanner::getFingerprint(int timeout)
{
    int err = scanner_scan(scanner, timeout);

    if (err == -1)
        throw ScannerTimeout();

    if (err)
        throw ScannerException("Failed to scan", err);

    Fingerprint *fingerprint = new Fingerprint();

    if (caps.image) {
        int size = scanner_get_image(scanner, NULL, 0);

        if (size < 0)
            throw ScannerException("Zero sized image");

        unsigned char buffer[size];

        size = scanner_get_image(scanner, (void *)buffer, size);
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
        int size = scanner_get_iso_template(scanner, NULL, 0);

        if (size < 0)
            throw ScannerException("Zero sized template");

        unsigned char buffer[size];

        size = scanner_get_iso_template(scanner, (void *)buffer, size);
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
