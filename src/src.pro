include(../../../config.pri)
TEMPLATE = lib
LANGUAGE = C++
TARGET   = suidi

QT += widgets
macx:QT_CONFIG -= no-pkg-config

CONFIG      += plugin
INCLUDEPATH += ../../interfaces
DEPENDPATH  += ../../interfaces

contains(QT_ARCH, i386) {
    DESTDIR = ../../../NDMXDumper-v.$$NDMXDVERSION-x32/plugins
} else {
    DESTDIR = ../../../NDMXDumper-v.$$NDMXDVERSION-x64/plugins
}

win32 {
    # Windows target
    contains(QT_ARCH, i386) {
        LIBUSBDIR    = $$PWD/../libusb-1.0.26-binaries/libusb-MinGW-Win32/
    } else {
        LIBUSBDIR    = $$PWD/../libusb-1.0.26-binaries/libusb-MinGW-x64/
    }
    LIBS        += -L$$LIBUSBDIR/lib -llibusb-1.0
    LIBS     += $$LIBUSBDIR/lib/libusb-1.0.a
    INCLUDEPATH += $$LIBUSBDIR/include/libusb-1.0/
    QMAKE_LFLAGS += -shared
} else {
    CONFIG      += link_pkgconfig
    PKGCONFIG   += libusb-1.0
}

HEADERS += ../../interfaces/qlcioplugin.h
HEADERS += suididevice.h \
           suidi.h

SOURCES += ../../interfaces/qlcioplugin.cpp
SOURCES += suididevice.cpp \
           suidi.cpp

# This must be after "TARGET = " and before target installation so that
# install_name_tool can be run before target installation
macx {
    include(../../../platforms/macos/nametool.pri)
    nametool.commands += $$pkgConfigNametool(libusb-1.0, libusb-1.0.0.dylib)
}

# Installation
target.path = $$INSTALLROOT/$$PLUGINDIR
INSTALLS   += target

# UDEV rule to make suidi USB device readable & writable for users in Linux
unix:!macx {
    udev.path  = $$UDEVRULESDIR
    udev.files = suidi.rules
    INSTALLS  += udev

    metainfo.path   = $$METAINFODIR
    metainfo.files += org.qlcplus.QLCPlus.suidi.metainfo.xml
    INSTALLS       += metainfo
}
