#ifndef NONINTERACTIVETEST_H
#define NONINTERACTIVETEST_H
#include <QTest>

class BundleTests : public QObject {
  Q_OBJECT

private Q_SLOTS:

  void BinaryFileTest();
  void BundleFileTest();
};

#endif // NONINTERACTIVETEST_H
