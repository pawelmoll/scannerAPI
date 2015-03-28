#ifndef SCANNER_H
#define SCANNER_H

#include <exception>

#include "fingerprint.h"
#include "../scanner.h"



class ScannersList
{
private:
    std::vector<const char *> names;

    ScannersList();

    ScannersList(const ScannersList &);
    ScannersList& operator =(const ScannersList& other);

public:
    std::vector<const char *>::iterator begin() { return names.begin(); }
    std::vector<const char *>::iterator end() { return names.end(); }
    const char *operator[](int n) { return names[n]; }

    static ScannersList &getScannersList();
};



class ScannerException : public std::exception
{
    const char *message;
    int error;

public:
    virtual int code() { return error; }
    virtual const char* what() { return message; }

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
    struct scanner *scanner;
    void *image;
    size_t image_size;
    void *iso_template;
    size_t iso_template_size;
    struct scanner_caps caps;

public:
    Scanner(const char *name);
    ~Scanner();

    const char *getName();
    Fingerprint *getFingerprint(int timeout = -1); // ms, -1 means infinite
};

#endif // SCANNER_H
