REPO_ROOT = $$PWD/..
contains(QMAKE_HOST.arch, x86_64) {
   DESTDIR   = $$REPO_ROOT/bin/x64/tests
} else {
   DESTDIR   = $$REPO_ROOT/bin/tests
}
TARGET    = BundleUnitTests

QT       += core testlib

CONFIG   -= app_bundle
CONFIG   += console c++14 debug_and_release warn_on build_all

INCLUDEPATH += $$REPO_ROOT/mbedtls-2.4.0/include/

contains(QMAKE_HOST.arch, x86_64) {
LIBS +=  -L$$REPO_ROOT/bin/x64
} else {
LIBS +=  -L$$REPO_ROOT/bin
}

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
    LIBS +=  -lBundlesLibraryd
} else {
    LIBS +=  -lBundlesLibrary
}

TEMPLATE = app

SOURCES += \
    main.cpp \
    BundleTests.cpp \

HEADERS += \
    BundleTests.h \
