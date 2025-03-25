#include <libusb.h>

#include <QMessageBox>
#include <QString>
#include <QDebug>

#include "suididevice.h"
#include "suidi.h"

SUIDI::~SUIDI()
{
}

void SUIDI::init()
{
    m_ctx = NULL;

    if (libusb_init(&m_ctx) != 0)
        qWarning() << "Unable to initialize libusb context!";

    rescanDevices();
}

QString SUIDI::name()
{
    return QString("SUIDI");
}

int SUIDI::capabilities() const
{
    return QLCIOPlugin::Output;
}

/*****************************************************************************
 * Outputs
 *****************************************************************************/

bool SUIDI::openOutput(quint32 output, quint32 universe)
{
    if (output < quint32(m_deviceOutputs.size()))
    {
        addToMap(universe, output, Output);
        return m_deviceOutputs.at(output)->device->
                open(m_deviceOutputs.at(output)->outputUniverse);
    }
    return false;
}

void SUIDI::closeOutput(quint32 output, quint32 universe)
{
    if (output < quint32(m_deviceOutputs.size()))
    {
        removeFromMap(output, universe, Output);
        m_deviceOutputs.at(output)->device->
                close(m_deviceOutputs.at(output)->outputUniverse);
    }
}

QStringList SUIDI::outputs()
{
    /* Clear m_deviceOutputs list */
    while (m_deviceOutputs.count()){
        DeviceOutputs *m_deviceOutput = m_deviceOutputs.first();
        m_deviceOutputs.removeFirst();
        delete m_deviceOutput;
    }
    /* Make new m_deviceOutputs list */
    QStringList list;
    QListIterator <SUIDIDevice*> it(m_devices);
    quint32 itNumber = 0;
    while (it.hasNext() == true){
        SUIDIDevice *device = it.next();
        quint32 outputs = static_cast<quint32>(device->outpust());
        if(outputs > 1){
            for(quint32 uNumber = 0;uNumber < outputs;uNumber++, itNumber++){
                list << device->name() + " U" + QString::number(uNumber + 1);
                DeviceOutputs *m_deviceOutput =
                        new DeviceOutputs { itNumber, device, uNumber };
                m_deviceOutputs.append(m_deviceOutput);
            }
        } else {
            list << device->name();
        }
    }

    return list;
}

QString SUIDI::pluginInfo()
{
    QString str;

    str += QString("<HTML>");
    str += QString("<HEAD>");
    str += QString("<TITLE>%1</TITLE>").arg(name());
    str += QString("</HEAD>");
    str += QString("<BODY>");

    str += QString("<P>");
    str += QString("<H3>%1</H3>").arg(name());
    str += tr("This plugin provides DMX output support for SUIDI devices.");
    str += QString("</P>");

    return str;
}

QString SUIDI::outputInfo(quint32 output)
{
    QString str;

    if (output != QLCIOPlugin::invalidLine() && output < quint32(m_devices.size()))
    {
        str += m_devices.at(output)->infoText();
    }

    str += QString("</BODY>");
    str += QString("</HTML>");

    return str;
}

void SUIDI::writeUniverse(quint32 universe, quint32 output, const QByteArray &data, bool dataChanged)
{
    Q_UNUSED(universe)
    Q_UNUSED(dataChanged)
    if (output < quint32(m_deviceOutputs.size()))
        m_deviceOutputs.at(output)->device->
                outputDMX(m_deviceOutputs.at(output)->outputUniverse, data);
}

void SUIDI::rescanDevices()
{
    /* Treat all devices as dead first, until we find them again. Those
       that aren't found, get destroyed at the end of this function. */
    QList <SUIDIDevice*> destroyList(m_devices);
    int devCount = m_devices.count();

    libusb_device** devices = NULL;
    ssize_t count = libusb_get_device_list(m_ctx, &devices);
    for (ssize_t i = 0; i < count; i++)
    {
        libusb_device* dev = devices[i];
        Q_ASSERT(dev != NULL);

        libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0)
        {
            qWarning() << "Unable to get device descriptor:" << r;
            continue;
        }
        SUIDIDevice* udev = device(dev);
        if (udev != NULL)
        {
            /* We already have this device and it's still
               there. Remove from the destroy list and
               continue iterating. */
            destroyList.removeAll(udev);
            continue;
        }
        else if (SUIDIDevice::isSUIDIDevice(&desc) == true)
        {
            /* This is a new device. Create and append. */
            udev = new SUIDIDevice(dev, &desc, this);
            m_devices.append(udev);
        }
    }

    /* Destroy those devices that were no longer found. */
    while (destroyList.isEmpty() == false)
    {
        SUIDIDevice* udev = destroyList.takeFirst();
        m_devices.removeAll(udev);
        delete udev;
    }

    if (m_devices.count() != devCount)
        emit configurationChanged();
}

SUIDIDevice* SUIDI::device(struct libusb_device* usbdev)
{
    QListIterator <SUIDIDevice*> it(m_devices);
    while (it.hasNext() == true)
    {
        SUIDIDevice* udev = it.next();
        if (udev->device() == usbdev)
            return udev;
    }

    return NULL;
}

/*****************************************************************************
 * Configuration
 *****************************************************************************/

void SUIDI::configure()
{
    int r = QMessageBox::question(NULL, name(),
                                  tr("Do you wish to re-scan your hardware?"),
                                  QMessageBox::Yes, QMessageBox::No);
    if (r == QMessageBox::Yes)
        rescanDevices();
}

bool SUIDI::canConfigure()
{
    return true;
}
