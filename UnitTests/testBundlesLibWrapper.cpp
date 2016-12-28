#include <QString>
#include <QtTest>
#include "../BundlesAddon/BundlesLibNodeJS.h"

using namespace std;
class UTBundleLibWrapper : public QObject {
  Q_OBJECT

public:

  UTBundleLibWrapper();

private Q_SLOTS:

  void BundleFileTest();
};

UTBundleLibWrapper::UTBundleLibWrapper()
{}

void UTBundleLibWrapper::BundleFileTest()
{
}

QTEST_APPLESS_MAIN(UTBundleLibWrapper)

#include "testBundlesLibWrapper.moc"
