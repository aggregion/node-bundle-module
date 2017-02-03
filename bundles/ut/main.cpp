#include "BundleTests.h"

using namespace std;

int main(int argc, char *argv[])
{
  int res = QTest::qExec(new BundleTests(), argc, argv);

  return res;
}
