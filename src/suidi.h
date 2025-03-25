#ifndef SUIDI_H
#define SUIDI_H

#include <QStringList>
#include <QList>

#include "qlcioplugin.h"

struct libusb_device;
class SUIDIDevice;

typedef struct {
    quint32 output;
    SUIDIDevice *device;
    quint32 outputUniverse;

} DeviceOutputs;

class SUIDI : public QLCIOPlugin
{
    Q_OBJECT
    Q_INTERFACES(QLCIOPlugin)
    Q_PLUGIN_METADATA(IID QLCIOPlugin_iid)

    /*********************************************************************
     * Initialization
     *********************************************************************/
public:
    /** @reimp */
    virtual ~SUIDI();

    /** @reimp */
    void init();

    /** @reimp */
    QString name();

    /** @reimp */
    int capabilities() const;

    /** @reimp */
    QString pluginInfo();

    /*********************************************************************
     * Outputs
     *********************************************************************/
public:
    /** @reimp */
    bool openOutput(quint32 output, quint32 universe);

    /** @reimp */
    void closeOutput(quint32 output, quint32 universe);

    /** @reimp */
    QStringList outputs();

    /** @reimp */
    QString outputInfo(quint32 output);

    /** @reimp */
    void writeUniverse(quint32 universe, quint32 output, const QByteArray& data, bool dataChanged);

private:
    /** Attempt to find all SUIDI devices */
    void rescanDevices();

    /** Get a SUIDIDevice entry by its usbdev struct */
    SUIDIDevice* device(libusb_device *usbdev);

private:
    struct libusb_context* m_ctx;

    /** List of available devices */
    QList <SUIDIDevice*> m_devices;
    QList <DeviceOutputs*> m_deviceOutputs;

    /*********************************************************************
     * Configuration
     *********************************************************************/
public:
    /** @reimp */
    void configure();

    /** @reimp */
    bool canConfigure();
};

#endif
