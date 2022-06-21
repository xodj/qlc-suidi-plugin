#define LIBUSB_DEBUG 4
#include <libusb.h>

#include <QElapsedTimer>
#include <QSettings>
#include <QDebug>
#include <cmath>

#include "suididevice.h"
#include "qlcmacros.h"

#define DMX_CHANNELS 512

#define SUIDI_SHARED_VENDOR         0x6244
#define SUIDI_SHARED_PRODUCT_00     0x0301
#define SUIDI_SHARED_PRODUCT_01     0x0302
#define SUIDI_SHARED_PRODUCT_02     0x0303
#define SUIDI_SHARED_PRODUCT_03     0x0401
#define SUIDI_SHARED_PRODUCT_04     0x0411
#define SUIDI_SHARED_PRODUCT_05     0x0531
#define SUIDI_SHARED_PRODUCT_06     0x0532
#define SUIDI_SHARED_PRODUCT_07     0x0421
#define SUIDI_SHARED_PRODUCT_08     0x0431
#define SUIDI_SHARED_PRODUCT_09     0x0441
#define SUIDI_SHARED_PRODUCT_10     0x0511
#define SUIDI_SHARED_PRODUCT_11     0x0451
#define SUIDI_SHARED_PRODUCT_12     0x0491
#define SUIDI_SHARED_PRODUCT_13     0x0591
#define SUIDI_SHARED_PRODUCT_14     0x0601
#define SUIDI_SHARED_PRODUCT_15     0x0611
#define SUIDI_SHARED_PRODUCT_16     0x0650
#define SUIDI_SHARED_PRODUCT_17     0x0651
#define SUIDI_SHARED_PRODUCT_18     0x0653
#define SUIDI_SHARED_PRODUCT_19     0x0655
#define SUIDI_SHARED_PRODUCT_20     0x0631
#define SUIDI_SHARED_PRODUCT_21     0x0471
#define SUIDI_SHARED_PRODUCT_22     0x0461
#define SUIDI_SHARED_PRODUCT_23     0x0501
#define SUIDI_SHARED_PRODUCT_24     0x0521
#define SUIDI_SHARED_PRODUCT_25     0x0481
#define SUIDI_SHARED_PRODUCT_26     0x0541
#define SUIDI_SHARED_PRODUCT_27     0x0561
#define SUIDI_SHARED_PRODUCT_28     0x0571
#define SUIDI_SHARED_PRODUCT_29     0x0581
#define SUIDI_SHARED_PRODUCT_30     0x0621

#define SUIDI_SET_CHANNEL_RANGE 0x0002 /* Command to set n channel values */

#define SETTINGS_FREQUENCY "suidi/frequency"

/****************************************************************************
 * Initialization
 ****************************************************************************/

SUIDIDevice::SUIDIDevice(struct libusb_device* device, libusb_device_descriptor *desc, QObject* parent)
    : QThread(parent)
    , m_device(device)
    , m_descriptor(desc)
    , m_handle(NULL)
    , m_running(false)
    , m_frequency(100)
    , m_granularity(Unknown)
{
    Q_ASSERT(device != NULL);

    QSettings settings("settings.ini", QSettings::IniFormat);
    QVariant var = settings.value(SETTINGS_FREQUENCY);
    if (var.isValid() == true)
        m_frequency = var.toDouble();

    extractName();
}

SUIDIDevice::~SUIDIDevice()
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setValue(SETTINGS_FREQUENCY, m_frequency);
    close();
}

/****************************************************************************
 * Device information
 ****************************************************************************/

bool SUIDIDevice::isSUIDIDevice(const struct libusb_device_descriptor* desc)
{
    if (desc == NULL)
        return false;

    if (desc->idVendor != SUIDI_SHARED_VENDOR)
            return false;

    if (
            desc->idProduct != SUIDI_SHARED_PRODUCT_00 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_01 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_02 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_03 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_04 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_05 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_06 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_07 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_08 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_09 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_10 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_11 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_12 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_13 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_14 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_15 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_16 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_17 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_18 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_19 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_20 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_21 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_22 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_23 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_24 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_25 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_26 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_27 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_28 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_29 &&
            desc->idProduct != SUIDI_SHARED_PRODUCT_30
            )
        return false;

    return true;
}

void SUIDIDevice::extractName()
{
    Q_ASSERT(m_device != NULL);

    libusb_device_handle* handle = NULL;
    int r = libusb_open(m_device, &handle);
    if (r == 0)
    {
        char buf[256];
        int len = 0;

        /* Extract the name */
        len = libusb_get_string_descriptor_ascii(handle, m_descriptor->iProduct,
                                               (uchar*) &buf, sizeof(buf));
        if (len > 0)
        {
            m_name = QString(QByteArray(buf, len));
        }
        else
        {
            m_name = tr("Unknown");
            qWarning() << "Unable to get product name:" << len;
        }

         m_config = (libusb_config_descriptor*)malloc(sizeof(*m_config));
         if (m_config)
             libusb_get_active_config_descriptor(m_device, &m_config);
         endpoints.clear();
         int endp = (int)m_config->interface[0].altsetting[0].bNumEndpoints;
         for(int i = 0;i < endp;i++){
             uint8_t bEndpointAddress = m_config->interface[0].altsetting[0].endpoint[i].bEndpointAddress;
             uint8_t bDescriptorType = m_config->interface[0].altsetting[0].endpoint[i].bDescriptorType;
             if(bDescriptorType == LIBUSB_DT_ENDPOINT && bEndpointAddress < 0x80)
                 endpoints.append(bEndpointAddress);
             uint16_t wMaxPacketSize = m_config->interface[0].altsetting[0].endpoint[i].wMaxPacketSize;
             if(len > wMaxPacketSize)
                 len = wMaxPacketSize;
         }
    }
    libusb_close(handle);
}

QString SUIDIDevice::name() const
{
    return m_name;
}

QString SUIDIDevice::infoText() const
{
    QString info;
    QString gran;

    if (m_device != NULL && m_handle != NULL)
    {
        info += QString("<P>");
        info += QString("<B>%1:</B> %2").arg(tr("Device name")).arg(name());
        info += QString("<BR>");
        info += QString("<B>%1:</B> %2").arg(tr("DMX Channels")).arg(512);
        info += QString("<BR>");
        info += QString("<B>%1:</B> %2Hz").arg(tr("DMX Frame Frequency")).arg(m_frequency);
        info += QString("<BR>");
        if (m_granularity == Bad)
            gran = QString("<FONT COLOR=\"#aa0000\">%1</FONT>").arg(tr("Bad"));
        else if (m_granularity == Good)
            gran = QString("<FONT COLOR=\"#00aa00\">%1</FONT>").arg(tr("Good"));
        else
            gran = tr("Patch this device to a universe to find out.");
        info += QString("<B>%1:</B> %2").arg(tr("System Timer Accuracy")).arg(gran);
        info += QString("</P>");
    }
    else
    {
        info += QString("<P><B>%1</B></P>").arg(tr("Device not in use"));
    }

    return info;
}

/****************************************************************************
 * Open & close
 ****************************************************************************/

bool SUIDIDevice::open()
{
    if (m_device != NULL && m_handle == NULL)
    {
        qDebug() << "Open SUIDI with idProduct:" << m_descriptor->idProduct;
        int ret = libusb_open(m_device, &m_handle);
        if (ret < 0)
        {
            qWarning() << "Unable to open SUIDI with idProduct:" << m_descriptor->idProduct;
            m_handle = NULL;
        }

        /*qDebug() << "Send control request to SUIDI";
        ret = libusb_control_transfer(m_handle,
                            0xc0,
                            0x02,
                            0x0000,
                            0x0000,
                            new uchar (0x00),
                            112,
                            500);
        if (ret < 0)
        {
            qWarning() << "Unable to send control request to SUIDI";
            m_handle = NULL;
        }

        char buf[256];
        int len = 0;
        qDebug() << "Get product name SUIDI";
        len = libusb_get_string_descriptor(m_handle, 0x03, 0x409,
                                           (uchar*) &buf, sizeof(buf));
        if (len < 0)
        {
            qWarning() << "Unable to get product name:" << len;
        }

        qDebug() << "Get product name SUIDI";
        len = libusb_get_string_descriptor(m_handle, 0x03, 0x409,
                                           (uchar*) &buf, sizeof(buf));
        if (len < 0)
        {
            qWarning() << "Unable to get product name:" << len;
        }

        qDebug() << "Send control request to SUIDI";
        ret = libusb_control_transfer(m_handle,
                            0x40,
                            0x06,
                            0x0002,
                            0x0000,
                            new uchar (0x00),
                            0x01,
                            500);
        if (ret < 0)
        {
            qWarning() << "Unable to send control request to SUIDI";
            m_handle = NULL;
        }

        qDebug() << "Send control request to SUIDI";
        ret = libusb_control_transfer(m_handle,
                            0x40,
                            0x11,
                            0x0001,
                            0x0000,
                            new uchar (0x00),
                            0x01,
                            500);
        if (ret < 0)
        {
            qWarning() << "Unable to send control request to SUIDI";
            m_handle = NULL;
        }

        qDebug() << "Send control request to SUIDI";
        ret = libusb_control_transfer(m_handle,
                            0x40,
                            0x11,
                            0x0003,
                            0x0000,
                            new uchar (0x00),
                            0x01,
                            500);
        if (ret < 0)
        {
            qWarning() << "Unable to send control request to SUIDI";
            m_handle = NULL;
        }

        qDebug() << "Send control request to SUIDI";
        ret = libusb_control_transfer(m_handle,
                            0x40,
                            0x11,
                            0x0005,
                            0x0000,
                            new uchar (0x00),
                            0x01,
                            500);
        if (ret < 0)
        {
            qWarning() << "Unable to send control request to SUIDI";
            m_handle = NULL;
        }*/

        qDebug() << "Try to claim interface SUIDI";
        ret = libusb_claim_interface(m_handle, 0);
        if(ret < 0)
        {
            qWarning() << "Cannot Claim Interface";
            m_handle = NULL;
        }
        else
            qDebug() << "Claimed Interface";
    }

    if (m_handle == NULL)
        return false;

    start();

    return true;
}

void SUIDIDevice::close()
{
    stop();

    if (m_device != NULL && m_handle != NULL)
        libusb_close(m_handle);

    m_handle = NULL;
}

const struct libusb_device* SUIDIDevice::device() const
{
    return m_device;
}

/****************************************************************************
 * Thread
 ****************************************************************************/

void SUIDIDevice::outputDMX(const QByteArray& universe)
{
    /* Create SUIDI request */
    int i = 0,z = 0;
    for(int x = 0;x < 9;x++){
        m_universe[z] = uchar(x);
        z++;
        for(int y = 0;y < 57;y++, i++){
            if(i < 512){
                m_universe[z] = universe.at(i);
                z++;
            }
        }
        m_universe[z] = uchar(0x00);
        z++;
        m_universe[z] = uchar(0x00);
        z++;
        m_universe[z] = uchar(0x00);
        z++;
        m_universe[z] = uchar(0x00);
        z++;
        m_universe[z] = uchar(0x00);
        z++;
        m_universe[z] = uchar(0x00);
        z++;
    }
    m_universe[SUIDI_PACKET_SIZE - 1] = uchar(0xFF);
}

void SUIDIDevice::stop()
{
    while (isRunning() == true)
    {
        // This may occur before the thread sets m_running,
        // so timeout and try again if necessary
        m_running = false;
        wait(100);
    }
}
void SUIDIDevice::run()
{
    // One "official" DMX frame can take (1s/44Hz) = 23ms
    int frameTime = (int) floor(((double)1000 / m_frequency) + (double)0.5);
    int r = 0;

    // Wait for device to settle in case the device was opened just recently
    // Also measure, whether timer granularity is OK
    QElapsedTimer time;
    time.start();
    usleep(1000);
    if (time.elapsed() > 3)
        m_granularity = Bad;
    else
        m_granularity = Good;

    m_running = true;
    while (m_running == true)
    {
        if (m_handle == NULL)
            goto framesleep;

        time.restart();

        /* Write all 512 channels */
        for(qsizetype i = 0;i < endpoints.count();i++){
            r = libusb_bulk_transfer(m_handle,
                                     endpoints.at(i),
                                     m_universe,
                                     SUIDI_PACKET_SIZE,
                                     &len,
                                     0);
            if (r < 0)
                qWarning() << "SUIDI: unable to write universe:" << libusb_strerror(libusb_error(r));
        }

        r = libusb_control_transfer(m_handle,
                            0xc0,
                            0x08,
                            0x0000,
                            0x0000,
                            new uchar (0x00),
                            0x02,
                            10);
        if (r < 0)
            qWarning() << "SUIDI: unable to write universe:" << libusb_strerror(libusb_error(r));

framesleep:
        // Sleep for the remainder of the DMX frame time
        if (m_granularity == Good)
            while (time.elapsed() < frameTime) { usleep(1000); }
        else
            while (time.elapsed() < frameTime) { /* Busy sleep */ }
    }
}
