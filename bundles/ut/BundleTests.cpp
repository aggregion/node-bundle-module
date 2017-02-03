#include "BundleTests.h"
#include <QDir>
#include <time.h>
#include "../lib/streams/BinaryFile.h"
#include "../lib/BundlesLibrary.h"
#include <QDebug>

#define TEST_BUFFER_SIZE 16 * 1024 * 1024LL
#define FILE_SIZE 10 * 1024LL

using namespace std;

void BundleTests::BinaryFileTest() {
  CBinaryFile file(1200, 1200);

  // инициализируем генератор случайных чисел
  srand(_time32(nullptr));

  // выделим блоки памяти
  auto bufferRead  = new char[TEST_BUFFER_SIZE];
  auto bufferWrite = new char[TEST_BUFFER_SIZE];

  if ((bufferRead != nullptr) && (bufferWrite != nullptr))
  {
    // получим пусть к файлу
    auto str = QDir::tempPath().toStdString();
    str += "\\file.test";

    // создадим файл
    errno_t res = file.Open(str.c_str(), "w+b");

    if (res == 0)
    {
      // заполним буфер случайными данными
      for (size_t i = 0; i < TEST_BUFFER_SIZE; i++) {
        bufferWrite[i] = rand() % 256;
      }

      // пишем в файл через кэш
      for (int i = 0; i < 1024; i++) {
        QVERIFY2(file.Write(bufferWrite + (i * TEST_BUFFER_SIZE) / 1024, TEST_BUFFER_SIZE / 1024,
                            false) == TEST_BUFFER_SIZE / 1024, "Failed to write data");
      }

      // флашим
      file.Flush();

      // пишем в файл без кеша
      for (int i = 0; i < 1024; i++) {
        QVERIFY2(file.Write(bufferWrite + (i * TEST_BUFFER_SIZE) / 1024,
                            TEST_BUFFER_SIZE / 1024,
                            true) == TEST_BUFFER_SIZE / 1024, "Failed to write data");
      }

      // теперь читаем из файла с кешем
      file.Seek(0, SEEK_SET);

      for (int i = 0; i < TEST_BUFFER_SIZE; i++) {
        QVERIFY2(file.Read(bufferRead + i, 1, false) == 1, "Failed to read data");
      }

      // проверим содержимое
      QVERIFY2(memcmp(bufferWrite, bufferRead, TEST_BUFFER_SIZE) == 0, "Read data is invalid");

      // теперь читаем из файла без кеша
      file.Seek(0, SEEK_SET);

      QVERIFY2(file.Read(bufferRead, TEST_BUFFER_SIZE, false) == TEST_BUFFER_SIZE,
               "Failed to read data");

      // проверим содержимое
      QVERIFY2(memcmp(bufferWrite, bufferRead, TEST_BUFFER_SIZE) == 0, "Read data is invalid");

      // теперь читаем из файла большим блоком с кешем
      file.Seek(0, SEEK_SET);

      QVERIFY2(file.Read(bufferRead, TEST_BUFFER_SIZE, true) == TEST_BUFFER_SIZE,
               "Failed to read data");

      // проверим содержимое
      QVERIFY2(memcmp(bufferWrite, bufferRead, TEST_BUFFER_SIZE) == 0, "Read data is invalid");

      // закроем файл
      file.Close();
    }
  }

  // освободим память
  delete[] bufferRead;
  delete[] bufferWrite;
}

void BundleTests::BundleFileTest() {
  void *bundle;

  unsigned char key[] =
  { 0x4a, 0x12, 0x45, 0x6a, 0x2a, 0x4d, 0x27, 0xb8, 0xa5, 0x31, 0xd5, 0xb6, 0xfb, 0x68, 0x8a,
    0x11 };
  unsigned char fileKey[] =
  { 0xb4, 0x9b, 0xf9, 0x93, 0xdb, 0xff, 0x09, 0x23, 0xa5, 0x31, 0x8a, 0x3c, 0x82, 0x50, 0xb5,
    0x3f };
  char dataPrivate[] = { "Private data here!" };
  char dataPublic[]  = { "Public data here!" };
  char dataSystem[]  = { "System data here!" };
  bool error         = false;

  char buffer[1024] = { 0 };
  int64_t len       = sizeof(buffer);

  bundle = nullptr;

  srand(_time32(nullptr));
  auto f1 = new char[FILE_SIZE];
  auto f2 = new char[FILE_SIZE];
  auto r  = new char[FILE_SIZE];

  // ИНИЦИАЛИЗАЦИЯ
  auto str = QDir::currentPath().toStdString() + "\\test.bundle";

  // иницализируем содержимое
  for (size_t i = 0; i < FILE_SIZE; i++)
  {
    f1[i] = rand() % 256;
    f2[i] = rand() % 256;
  }
  bundle = BundleOpen(str.c_str(), BMODE_READWRITE | BMODE_OPEN_ALWAYS);

  if (bundle)
  {
    // ТЕСТ №1. АТТРИБУТЫ
    BundleAttributeSet(bundle, BUNDLE_EXTRA_PRIVATE, dataPrivate, 0, sizeof(dataPrivate) / 2);
    BundleAttributeSet(bundle, BUNDLE_EXTRA_PUBLIC,  dataPublic,  0, sizeof(dataPublic));
    BundleAttributeSet(bundle, BUNDLE_EXTRA_SYSTEM,  dataSystem,  0, sizeof(dataSystem));

    // создаем сегментированный аттрибут
    BundleAttributeSet(bundle, BUNDLE_EXTRA_PRIVATE, dataPrivate, 0, sizeof(dataPrivate));

    // проверим аттрибуты
    QVERIFY2((BundleAttributeGet(bundle, BUNDLE_EXTRA_PRIVATE, nullptr, 0,
                                 &len) == 0) && (len == sizeof(dataPrivate)),
             "Failed to get attribute");
    QVERIFY2((BundleAttributeGet(bundle, BUNDLE_EXTRA_PRIVATE, buffer, 0,
                                 &len) == sizeof(dataPrivate)) &&
             (memcmp(buffer, dataPrivate, sizeof(dataPrivate)) == 0),
             "Failed to get attribute");

    QVERIFY2((BundleAttributeGet(bundle, BUNDLE_EXTRA_PUBLIC, nullptr, 0,
                                 &len) == 0) && (len == sizeof(dataPublic)),
             "Failed to get attribute");
    QVERIFY2((BundleAttributeGet(bundle, BUNDLE_EXTRA_PUBLIC, buffer, 0,
                                 &len) == sizeof(dataPublic)) &&
             (memcmp(buffer, dataPublic, sizeof(dataPublic)) == 0),
             "Failed to get attribute");

    QVERIFY2((BundleAttributeGet(bundle, BUNDLE_EXTRA_SYSTEM, nullptr, 0,
                                 &len) == 0) && (len == sizeof(dataSystem)),
             "Failed to get attribute");
    QVERIFY2((BundleAttributeGet(bundle, BUNDLE_EXTRA_SYSTEM, buffer, 0,
                                 &len) == sizeof(dataSystem)) &&
             (memcmp(buffer, dataSystem, sizeof(dataSystem)) == 0),
             "Failed to get attribute");

    // ТЕСТ №2. ФАЙЛЫ
    // открываем несуществующий файл
    QVERIFY2(BundleFileOpen(bundle, "TestPath\\testfile.test", false) == 0,
             "Failed to open bundle file");

    // выставим ключ
    QVERIFY2(BundleInitialize(bundle, key, sizeof(key)), "Failed to initialize bundle");

    // Создаем новый файл
    int idx1 = BundleFileOpen(bundle, "TestPath\\testfile1.test", true);
    int idx2 = BundleFileOpen(bundle, "TestPath\\testfile2.test", true);
    int idx3 = BundleFileOpen(bundle, "TestPath\\testfile3.test", true);

    QVERIFY2(idx1 > 0 && idx2 > 0 && idx3 > 0, "File not found");

    // тест с Seek
    int64_t rSize = FILE_SIZE;
    auto    wrote = BundleFileWrite(bundle, idx3, f2, 0, 256, nullptr);
    auto    pos   = BundleFileSeek(bundle, idx3, -10, BundleFileOrigin::BUNDLE_FILE_ORIG_END);
    wrote = BundleFileWrite(bundle, idx3, f2, 0, 10, nullptr);
    pos   = BundleFileSeek(bundle, idx3, -10, BundleFileOrigin::BUNDLE_FILE_ORIG_CUR);
    auto read = BundleFileRead(bundle, idx3, r, 0, &rSize, nullptr);
    pos = BundleFileSeek(bundle, idx3, 0, BundleFileOrigin::BUNDLE_FILE_ORIG_END);
    pos = BundleFileSeek(bundle, idx3, 0, BundleFileOrigin::BUNDLE_FILE_ORIG_END);

    // создадим новый контекст
    void *cryptoContext = BundleCreateCryptoContext(fileKey, sizeof(fileKey));

    if (cryptoContext != nullptr)
    {
      for (int i = 0; i < 10 && !error; i++)
      {
        // пишем данные
        if (BundleFileWrite(bundle, idx1, f1 + i * FILE_SIZE / 10, 0, FILE_SIZE / 10,
                            nullptr) != FILE_SIZE / 10) {
          error = true;
        }

        // пишем шифрованные данные
        if (!error &&
            (BundleFileWrite(bundle, idx2, f2 + i * FILE_SIZE / 10, 0, FILE_SIZE / 10,
                             cryptoContext) != FILE_SIZE / 10)) {
          error = true;
        }
      }

      // проверим чтение
      int64_t rSize = FILE_SIZE;
      BundleFileSeek(bundle, idx1, 0, BUNDLE_FILE_ORIG_SET);
      BundleFileSeek(bundle, idx2, 0, BUNDLE_FILE_ORIG_SET);

      if ((BundleFileRead(bundle, idx1, r, 0, &rSize,
                          nullptr) != FILE_SIZE) || (memcmp(r, f1, FILE_SIZE) != 0)) {
        error = true;
      }

      if (!error &&
          ((BundleFileRead(bundle, idx2, r, 0, &rSize,
                           cryptoContext) != FILE_SIZE) ||
           (memcmp(r, f2, FILE_SIZE) != 0))) {
        error = true;
      }

      // проверим Seek
      int64_t pos = 0;
      rSize = 100;

      // 0 - 0
      pos = BundleFileSeek(bundle, idx1, -100, BUNDLE_FILE_ORIG_SET);

      if (pos != 0) {
        error = true;
      }

      if ((BundleFileRead(bundle, idx1, r, 0, &rSize,
                          nullptr) != 100) || (memcmp(r, f1, 100) != 0)) {
        error = true;
      }

      // 100 - 200
      pos = BundleFileSeek(bundle, idx1, 100, BUNDLE_FILE_ORIG_SET);

      if (pos != 100) {
        error = true;
      }

      if ((BundleFileRead(bundle, idx1, r, 0, &rSize,
                          nullptr) != 100) ||
          (memcmp(r, f1 + 100, 100) != 0)) {
        error = true;
      }

      // 1200 - 1300
      pos = BundleFileSeek(bundle, idx1, 1200, BUNDLE_FILE_ORIG_SET);

      if (pos != 1200) {
        error = true;
      }

      if ((BundleFileRead(bundle, idx1, r, 0, &rSize,
                          nullptr) != 100) ||
          (memcmp(r, f1 + 1200, 100) != 0)) {
        error = true;
      }

      // 1000 - 1100
      pos = BundleFileSeek(bundle, idx1, -300, BUNDLE_FILE_ORIG_CUR);

      if (pos != 1000) {
        error = true;
      }

      if ((BundleFileRead(bundle, idx1, r, 0, &rSize,
                          nullptr) != 100) ||
          (memcmp(r, f1 + 1000, 100) != 0)) {
        error = true;
      }

      // 2000 - 2100
      pos = BundleFileSeek(bundle, idx1, 900, BUNDLE_FILE_ORIG_CUR);

      if (pos != 2000) {
        error = true;
      }

      if ((BundleFileRead(bundle, idx1, r, 0, &rSize,
                          nullptr) != 100) ||
          (memcmp(r, f1 + 2000, 100) != 0)) {
        error = true;
      }

      // 10240 - 10240
      pos = BundleFileSeek(bundle, idx1, 0, BUNDLE_FILE_ORIG_END);

      if (pos != 10240) {
        error = true;
      }

      if (BundleFileRead(bundle, idx1, r, 0, &rSize, nullptr) != 0) {
        error = true;
      }

      // 10240 - 10240
      pos = BundleFileSeek(bundle, idx1, 100, BUNDLE_FILE_ORIG_END);

      if (pos != 10240) {
        error = true;
      }

      if (BundleFileRead(bundle, idx1, r, 0, &rSize, nullptr) != 0) {
        error = true;
      }

      // 10140 - 10240
      pos = BundleFileSeek(bundle, idx1, -100, BUNDLE_FILE_ORIG_END);

      if (pos != 10140) {
        error = true;
      }

      if ((BundleFileRead(bundle, idx1, r, 0, &rSize,
                          nullptr) != 100) ||
          (memcmp(r, f1 + 10140, 100) != 0)) {
        error = true;
      }
    }

    // освободим крипто контекст
    BundleDestroyCryptoContext(cryptoContext);
    cryptoContext = nullptr;
  }
  else {
    error = true;
  }

  //
  BundleClose(bundle);
  bundle = nullptr;

  // ОТКРЫТИЕ СУЩЕСТВУЮЩЕГО БАНДЛА (второй раз после дефрагментации)
  for (int i = 0; i < 2 && !error; i++)
  {
    bundle = BundleOpen(str.c_str(), BMODE_READWRITE);

    if (bundle)
    {
      // ТЕСТ №3. АТТРИБУТЫ
      if ((BundleAttributeGet(bundle, BUNDLE_EXTRA_PRIVATE, nullptr, 0,
                              &len) != 0) || (len != sizeof(dataPrivate))) {
        error = true;
      }
      else
      {
        if ((BundleAttributeGet(bundle, BUNDLE_EXTRA_PRIVATE, buffer, 0,
                                &len) != sizeof(dataPrivate)) ||
            (memcmp(buffer, dataPrivate, sizeof(dataPrivate)) != 0)) {
          error = true;
        }
      }

      if (!error)
      {
        if ((BundleAttributeGet(bundle, BUNDLE_EXTRA_PUBLIC, nullptr, 0,
                                &len) != 0) || (len != sizeof(dataPublic))) {
          error = true;
        }
        else
        if ((BundleAttributeGet(bundle, BUNDLE_EXTRA_PUBLIC, buffer, 0,
                                &len) != sizeof(dataPublic)) ||
            (memcmp(buffer, dataPublic, sizeof(dataPublic)) != 0)) {
          error = true;
        }
      }

      if (!error)
      {
        if ((BundleAttributeGet(bundle, BUNDLE_EXTRA_SYSTEM, nullptr, 0,
                                &len) != 0) || (len != sizeof(dataSystem))) {
          error = true;
        }
        else
        if ((BundleAttributeGet(bundle, BUNDLE_EXTRA_SYSTEM, buffer, 0,
                                &len) != sizeof(dataSystem)) ||
            (memcmp(buffer, dataSystem, sizeof(dataSystem)) != 0)) {
          error = true;
        }
      }

      // ТЕСТ №2. ФАЙЛЫ
      if (!error)
      {
        // выставим ключ
        if (!BundleInitialize(bundle, key, sizeof(key))) {
          error = true;
        }

        // Создаем новый файл
        int idx1 = BundleFileOpen(bundle, "TestPath\\testfile1.test", false);
        int idx2 = BundleFileOpen(bundle, "TestPath\\testfile2.test", false);

        if (((idx1 <= 0) && (i == 0)) || (idx2 <= 0)) {
          error = true;
        }

        // создадим новый контекст
        void *cryptoContext = BundleCreateCryptoContext(fileKey, sizeof(fileKey));

        if (cryptoContext != nullptr)
        {
          // проверим чтение
          if (!error)
          {
            int64_t rSize = FILE_SIZE;

            if (i == 0) BundleFileSeek(bundle, idx1, 0, BUNDLE_FILE_ORIG_SET);
            BundleFileSeek(bundle, idx2, 0, BUNDLE_FILE_ORIG_SET);

            if (i == 0)
            {
              if ((BundleFileRead(bundle, idx1, r, 0, &rSize,
                                  nullptr) != FILE_SIZE) ||
                  (memcmp(r, f1, FILE_SIZE) != 0)) {
                error = true;
              }
            }

            if (!error &&
                ((BundleFileRead(bundle, idx2, r, 0, &rSize,
                                 cryptoContext) != FILE_SIZE) ||
                 (memcmp(r, f2, FILE_SIZE) != 0))) {
              error = true;
            }
          }

          // освободим крипто контекст
          BundleDestroyCryptoContext(cryptoContext);
          cryptoContext = nullptr;
        }
        else {
          error = true;
        }

        // удалим файл
        BundleFileDelete(bundle, idx1);

        idx1 = BundleFileOpen(bundle, "TestPath\\testfile1.test", false);

        if (idx1 >= 0) {
          error = true;
        }
      }

      //
      BundleClose(bundle);
      bundle = nullptr;
    }
    else {
      error = true;
    }

    // ДЕФРАГМЕНТАЦИЯ
    if (i == 0)
    {
      std::string strTmp = str + ".tmp";
      Defragmentation(str.c_str(), strTmp.c_str());
    }
  }

  // ЗАВЕРШЕНИЕ
  // освободим память
  if (f1 != nullptr) free(f1);

  if (f2 != nullptr) free(f2);

  if (r != nullptr)  free(r);

  // прошли ли тест?

  if (error) throw std::exception();
}
