TEMPLATE = subdirs

SUBDIRS += \
    BundlesAddon \
    UnitTests

UnitTests.depends  = BundlesAddon
