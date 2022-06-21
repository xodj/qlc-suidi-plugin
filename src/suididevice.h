#ifndef SUIDIDEVICE_H
#define SUIDIDEVICE_H

#include <QThread>

#define SUIDI_PACKET_SIZE 576
#define SUIDI_MAX_UNIVERSES 4
#define SUIDI_DEFAULT_FREQUENCY 44

struct libusb_device;
struct libusb_device_handle;
struct libusb_device_descriptor;

typedef struct {
    uint8_t endpoint;
    bool opened;

} UniverseEndpoint;

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
    qsizetype outpust() const {
        return endpoints.count();
    }

private:
    void extractNameEndpoints();

private:
    QString m_name;

    /********************************************************************
     * Open & close
     ********************************************************************/
public:
    bool open(quint32 universe);
    void close(quint32 universe);

    const libusb_device *device() const;

private:
    struct libusb_device* m_device;
    struct libusb_device_descriptor *m_descriptor;
    struct libusb_device_handle* m_handle;
    struct libusb_config_descriptor *m_config;
    int len = 64;
    QList<UniverseEndpoint*> endpoints;

    /********************************************************************
     * Thread
     ********************************************************************/
public:
    void outputDMX(quint32 universeNumber, const QByteArray& universe);

private:
    enum TimerGranularity { Unknown, Good, Bad };

    /** Stop the writer thread */
    void stop();

    /** DMX writer thread worker method */
    void run();

private:
    bool m_running;
    uchar m_universe[SUIDI_MAX_UNIVERSES][SUIDI_PACKET_SIZE];
    double m_frequency;
    TimerGranularity m_granularity;
};

#endif
