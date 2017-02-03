TEMPLATE = subdirs
DEFINES += QTFRAMEWORK


SUBDIRS += \
    lib \
    ut

ut.depends  = lib
