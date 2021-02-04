#include <string>
#include <vector>
#include <map>
#include "BundlesLibrary.h"
#include "BundleFile.h"

#include "streams/BinaryFile.h"


// открытие бандла
BundlePtr BundleOpen(const char *filename, int mode) {
  std::string  om     = "";
  CBundleFile *bundle = nullptr;
  std::shared_ptr<CBinaryFile> stream;

  // проверки параметров
  if (filename == nullptr) {
    return nullptr;
  }

  // выделяем объекты
  try {
    stream = std::make_shared<CBinaryFile>(0, 0);
    bundle = new CBundleFile(stream);
  } catch (...) {
    if (bundle != nullptr) {
      delete bundle;
    }
    return nullptr;
  }
  // выставим режим
  if ((mode & BMODE_READWRITE) == BMODE_READWRITE) {
    om += "r+b";
  } else {
    if ((mode & BMODE_READ) == BMODE_READ) {
      om += "rb";
    } else if ((mode & BMODE_WRITE) == BMODE_WRITE) {
      om += "wb";
    }
  }

  // открываем поток
  errno_t err = stream->Open(filename, om.data());

  if ((err == ENOENT) && ((mode & BMODE_OPEN_ALWAYS) == BMODE_OPEN_ALWAYS)) {
    err = stream->Open(filename, "w+b");
  }

  // инициализиуруем бандл
  if ((err != 0) || (bundle->Open((mode & BMODE_OPEN_ALWAYS) == BMODE_OPEN_ALWAYS) != 0)) {
    delete bundle;
    bundle = nullptr;
  }

  // вернем результат
  return bundle;
}

BundlePtr BundleOpenFromStream(std::shared_ptr<IBinaryStream>
                               stream, int mode) {
  CBundleFile *bundle = nullptr;

  // проверки параметров
  if (stream.get() == nullptr) {
    return nullptr;
  }

  // выделяем объекты
  try {
    bundle = new CBundleFile(stream);
  } catch (...) {
    if (bundle != nullptr) {
      delete bundle;
    }
    return nullptr;
  }
  // инициализиуруем бандл
  if (bundle->Open((mode & BMODE_OPEN_ALWAYS) == BMODE_OPEN_ALWAYS) != 0) {
    delete bundle;
    bundle = nullptr;
  }

  // вернем результат
  return bundle;
}

// закрытие бандла
void BundleClose(BundlePtr bundle) {
  CBundleFile *bf = (CBundleFile *)bundle;

  // удалим
  if (bf != nullptr) {
    delete bf;
  }
}

// инициализация
int BundleInitialize(BundlePtr bundle, const void *pathKey,
                     int keyLen) {
  CBundleFile *bf = (CBundleFile *)bundle;

  return bf != nullptr
         && bf->Initialize(pathKey, keyLen, BUNDLE_CACHE_SIZE) ? 1 : 0;
}

// получение атрибутов бандла
int64_t BundleAttributeGet(BundlePtr bundle, BundleAttribute type,
                           void *dst, int64_t dstOffset, int64_t *dstLen) {
  CBundleFile *bf = (CBundleFile *)bundle;

  return bf != nullptr ? bf->BundleAttributeGet(0, type, (char *)dst + dstOffset, dstLen,
                                                nullptr) : 0;
}

// установка атрибутов бандла
int64_t BundleAttributeSet(BundlePtr bundle, BundleAttribute type,
                           const void *src, int64_t srcOffset, const int64_t srcLen) {
  CBundleFile *bf = (CBundleFile *)bundle;

  return bf != nullptr ? bf->BundleAttributeSet(0, type, (char *)src + srcOffset,
                                                srcLen, nullptr) : 0;
}

// получение файловых атрибутов
int64_t BundleFileAttributeGet(BundlePtr bundle, int idx,
                               void *dst,
                               int64_t dstOffset, int64_t *dstLen, CryptoCtx cryptoCtx) {
  CBundleFile *bf = (CBundleFile *)bundle;

  return bf != nullptr
         && idx > 0 ? bf->BundleAttributeGet(idx, BUNDLE_FILE_ATTRS, (char *)dst + dstOffset,
                                             dstLen, cryptoCtx) : 0;
}

// устновка файловых атрибутов
int64_t BundleFileAttributeSet(BundlePtr     bundle,
                               int           idx,
                               const void   *src,
                               int64_t       srcOffset,
                               const int64_t srcLen,
                               CryptoCtx     cryptoCtx) {
  CBundleFile *bf = (CBundleFile *)bundle;

  return bf != nullptr
         && idx > 0 ? bf->BundleAttributeSet(idx, BUNDLE_FILE_ATTRS, (char *)src + srcOffset,
                                             srcLen, cryptoCtx) : 0;
}

int BundleFileName(BundlePtr bundle, int idx, char *filename,
                   int len) {
  CBundleFile *bf = (CBundleFile *)bundle;

  return bf != nullptr ? bf->FileName(idx, filename, len) : -1;
}

// создание крипто контекста
CryptoCtx BundleCreateCryptoContext(const void *key, int keyLen) {
  AesContext *ret = new AesContext;

  if ((mbedtls_aes_setkey_enc(&ret->ctxEnc, (const unsigned char *)key, keyLen * 8) != 0)
      || (mbedtls_aes_setkey_dec(&ret->ctxDec, (const unsigned char *)key, keyLen * 8) != 0)) {
    delete ret;
    ret = nullptr;
  }
  return ret;
}

// удаление крипто контекста
void BundleDestroyCryptoContext(CryptoCtx cryptoCtx) {
  delete (AesContext *)cryptoCtx;
}

// открытие файла
int BundleFileOpen(BundlePtr bundle, const char *filename,
                   int openAlways) {
  CBundleFile *bf = (CBundleFile *)bundle;

  return bf != nullptr ? bf->FileOpen(filename, openAlways != 0) : -1;
}

// установка заданной позиции
int64_t BundleFileSeek(BundlePtr bundle, int idx, int64_t offset,
                       BundleFileOrigin origin) {
  CBundleFile *bf = (CBundleFile *)bundle;

  return bf != nullptr && idx > 0 ? bf->FileSeek(idx, offset, origin) : 0;
}

int64_t BundleFileLength(BundlePtr bundle, int idx) {
  CBundleFile *bf = (CBundleFile *)bundle;

  return bf != nullptr && idx > 0 ? bf->FileSize(idx) : 0;
}

// обрезание файла
void BundleFileTrunk(BundlePtr bundle, int idx, int64_t newSize) {
  CBundleFile *bf = (CBundleFile *)bundle;

  if ((bf != nullptr) && (idx > 0)) {
    bf->FileTrunk(idx, newSize);
  }
}

// чтение из файла
int64_t BundleFileRead(BundlePtr bundle, int idx, void *dst,
                       int64_t dstOffset, int64_t *dstLen, CryptoCtx cryptoCtx) {
  CBundleFile *bf = (CBundleFile *)bundle;

  return bf != nullptr
         && idx > 0 ? bf->BundleAttributeGet(idx, BUNDLE_FILE_DATA, (char *)dst + dstOffset,
                                             dstLen, cryptoCtx) : 0;
}

// запись в файл
int64_t BundleFileWrite(BundlePtr bundle, int idx, const void *src,
                        int64_t srcOffset, const int64_t srcLen, CryptoCtx cryptoCtx) {
  CBundleFile *bf = (CBundleFile *)bundle;

  return bf != nullptr
         && idx > 0 ? bf->BundleAttributeSet(idx, BUNDLE_FILE_DATA, (char *)src + srcOffset,
                                             srcLen, cryptoCtx) : 0;
}

// удаление файла
void BundleFileDelete(BundlePtr bundle, int idx) {
  CBundleFile *bf = (CBundleFile *)bundle;

  if ((bf != nullptr) && (idx > 0)) {
    bf->FileDelete(idx);
  }
}

// дефрагментация
void Defragmentation(const char *fileSrc, const char *fileTmp) {
  auto streamSrc = std::make_shared<CBinaryFile>(0, 1024 * 1024);
  auto streamDst = std::make_shared<CBinaryFile>(0, 1024 * 1024);

  // проверки
  if ((fileSrc == nullptr) || (fileTmp == nullptr)) {
    return;
  }

  // откроем
  if ((streamSrc->Open(fileSrc, "rb") == 0) && (streamDst->Open(fileTmp, "r+b") == 0)) {
    if (CBundleFile::Defragmentation(streamSrc, streamDst)) {
      // удалим старый бандл
      if (remove(fileSrc)) {
        // переименуем бандл
        rename(fileTmp, fileSrc);
      }
    }
  }

  // закроем
  streamSrc->Close();
  streamDst->Close();
}
