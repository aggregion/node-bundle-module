REPO_ROOT = $$PWD/..

QT     -= core gui
CONFIG -= qt

TARGET = BundlesLibrary
TEMPLATE = lib

CONFIG += staticlib
CONFIG += c++14 debug_and_release warn_on
CONFIG += build_all

contains(QMAKE_HOST.arch, x86_64) {
   DESTDIR   = $$REPO_ROOT/bin/x64
} else {
   DESTDIR   = $$REPO_ROOT/bin
}

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

INCLUDEPATH += $$REPO_ROOT/mbedtls-2.4.0/include/

SOURCES += BundlesLibrary.cpp \
    BundleFile.cpp \
    streams/BinaryFile.cpp \
    ../mbedtls-2.4.0/library/aes.c \
    ../mbedtls-2.4.0/library/padlock.c

HEADERS += BundlesLibrary.h \
    BundleFile.h \
    BundleFileHDRs.h \
    streams/BinaryFile.h \
    streams/IBinaryStream.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
