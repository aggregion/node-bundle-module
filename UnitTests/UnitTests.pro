REPO_ROOT = $$PWD/../

QT       += testlib core

TARGET = tst_utpfileworker
CONFIG   += c++14 console
CONFIG   -= app_bundle

TEMPLATE = app

contains(QMAKE_HOST.arch, x86_64) {
   DESTDIR   = $$REPO_ROOT/bin/x64
   LIBS     += -L$$REPO_ROOT/bin/x64
} else {
   DESTDIR   = $$REPO_ROOT/bin
   LIBS     += -L$$REPO_ROOT/bin
}

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
    LIBS += -lBundlesAddond
} else {
    LIBS += -lBundlesAddon
}

target.path = $$DESTDIR

SOURCES += \
    testBundlesLibWrapper.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"
