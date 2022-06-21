#ifndef SUIDIDEVICE_H
#define SUIDIDEVICE_H

#include <QThread>

#define SUIDI_PACKET_SIZE 576

struct libusb_device;
struct libusb_device_handle;
struct libusb_device_descriptor;

class SUIDIDevice : public QThread
{
    Q_OBJECT

    /********************************************************************
     * Initialization
     ********************************************************************/
public:
    SUIDIDevice(libusb_device *device, libusb_device_descriptor *desc, QObject* parent = 0);
    virtual ~SUIDIDevice();

    /** Find out, whether the given USB device is a SUIDI device */
    static bool isSUIDIDevice(const libusb_device_descriptor *desc);

    /********************************************************************
     * Device information
     ********************************************************************/
public:
    QString name() const;
    QString infoText() const;

private:
    void extractName();

private:
    QString m_name;

    /********************************************************************
     * Open & close
     ********************************************************************/
public:
    bool open();
    void close();

    const libusb_device *device() const;

private:
    struct libusb_device* m_device;
    struct libusb_device_descriptor *m_descriptor;
    struct libusb_device_handle* m_handle;
    struct libusb_config_descriptor *m_config;
    int len = 64;
    QList<uint8_t> endpoints;

    /********************************************************************
     * Thread
     ********************************************************************/
public:
    void outputDMX(const QByteArray& universe);

private:
    enum TimerGranularity { Unknown, Good, Bad };

    /** Stop the writer thread */
    void stop();

    /** DMX writer thread worker method */
    void run();

private:
    bool m_running;
    uchar m_universe[SUIDI_PACKET_SIZE];
    double m_frequency;
    TimerGranularity m_granularity;
};

#endif
