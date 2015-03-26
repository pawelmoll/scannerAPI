#ifndef SCANNER_H
#define SCANNER_H

#include <exception>

#include "fingerprint.h"
#include "../scanner.h"

class ScannerException : public std::exception
{
    const char *message;
    int error;

public:
    virtual int code()
    {
        return error;
    }

    virtual const char* what()
    {
        return message;
    }

    ScannerException(const char *message, int error = 0) : message(message), error(error) {}
};

class ScannerTimeout : public ScannerException
{
public:
    ScannerTimeout() : ScannerException("Timeout", 0) {}
};



class Scanner
{
private:
    void *image;
    size_t image_size;
    void *iso_template;
    size_t iso_template_size;
    struct scanner_caps caps;

public:
    Scanner();
    ~Scanner();

    const char *getName();
    Fingerprint *getFingerprint(int timeout = -1); // ms, -1 means infinite
};

#endif // SCANNER_H
