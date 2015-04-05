#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include <list>
#include <QImage>

#include "iso_fmr/v20.h"
#include "iso_fmr/v030.h"


class FingerprintMinutia
{
public:
    int x, y; // pixels
    int angle; // degrees/100000

    FingerprintMinutia(int x, int y, int angle) : x(x), y(y), angle(angle) {}
};



class FingerprintMinutiaeRecordException : public std::exception
{
    const char *message;
    int error;

public:
    virtual int code() { return error; }
    virtual const char* what() { return message; }

    FingerprintMinutiaeRecordException(const char *message, int error = 0) : message(message), error(error) {}
};

class FingerprintMinutiaeRecordInvalidVersion : public FingerprintMinutiaeRecordException
{
public:
    FingerprintMinutiaeRecordInvalidVersion() : FingerprintMinutiaeRecordException("Invalid version", 0) {}
};



class FingerprintMinutiaeRecord
{
public:
    std::list<FingerprintMinutia> minutiae;

    virtual ~FingerprintMinutiaeRecord() {}

    virtual QString getRecord() = 0;
};

class FingerprintISOv20MinutiaeRecord : public FingerprintMinutiaeRecord
{
private:
    iso_fmr_v20 *record;

public:
    FingerprintISOv20MinutiaeRecord(void *binary, size_t size);
    ~FingerprintISOv20MinutiaeRecord();

    QString getRecord();
};

class FingerprintISOv030MinutiaeRecord : public FingerprintMinutiaeRecord
{
private:
    struct iso_fmr_v030 *record;

public:
    FingerprintISOv030MinutiaeRecord(void *binary, size_t size);
    ~FingerprintISOv030MinutiaeRecord();

    QString getRecord();
};



class FingerprintImage
{
protected:
    int width, height;
public:
    FingerprintImage(int width, int height) : width(width), height(height) {}
    virtual ~FingerprintImage() {}

    virtual QImage *getImage() = 0;
};

class FingerprintGreyscaleImage : public FingerprintImage
{
private:
    std::vector<unsigned char> buffer;
    QVector<QRgb> grayScale;

public:
    FingerprintGreyscaleImage(int width, int height, void *image, size_t size);
    ~FingerprintGreyscaleImage();
    QImage *getImage();
};



class Fingerprint
{
public:
    enum Hand { hand_unknown, left, right } hand;
    enum Finger { finger_unknown, thumb, index, middle, ring, little } finger;

    FingerprintImage *image;
    FingerprintMinutiaeRecord *minutiaeRecord;

    Fingerprint(Hand hand = hand_unknown, Finger finger = finger_unknown) : hand(hand), finger(finger), image(NULL), minutiaeRecord(NULL) {}
    ~Fingerprint() { delete image; delete minutiaeRecord; }
};

#endif
