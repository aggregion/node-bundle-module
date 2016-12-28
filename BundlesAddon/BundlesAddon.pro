REPO_ROOT = $$PWD/../

QT     -= core gui
CONFIG -= qt

TARGET = BundlesAddon
TEMPLATE = lib

CONFIG += c++14 warn_on

INCLUDEPATH += $$REPO_ROOT/tools
INCLUDEPATH += $$REPO_ROOT/bundles/lib
INCLUDEPATH += /usr/include/nodejs/src/
INCLUDEPATH += /usr/include/nodejs/deps/v8/include/

INCLUDEPATH += $$REPO_ROOT/../node/src
INCLUDEPATH += $$REPO_ROOT/../node/deps/v8/include
INCLUDEPATH += $$REPO_ROOT/../node/deps/uv/include
INCLUDEPATH += $$REPO_ROOT/../node/test/gc/node_modules/nan

contains(QMAKE_HOST.arch, x86_64) {
   DESTDIR   = $$REPO_ROOT/bin/x64
   LIBS     += -L$$REPO_ROOT/bin/x64
} else {
   DESTDIR   = $$REPO_ROOT/bin
   LIBS     += -L$$REPO_ROOT/bin
}

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
    LIBS += -llibBundlesLibraryd
} else {
    LIBS += -llibBundlesLibrary
}

DEFINES += DONT_USE_NODE_JS
DEFINES += BUNDLES_ADDON_LIBRARY

SOURCES += \
    Bundle.cpp

HEADERS +=\
    BundlesLibNodeJS.h \
    exports.h \
    Bundle.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
