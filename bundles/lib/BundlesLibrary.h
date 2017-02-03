#pragma once
#include <memory>
#include "streams/IBinaryStream.h"

enum BundleOpenMode {
  BMODE_READ        = 0x01,
  BMODE_WRITE       = 0x02,
  BMODE_READWRITE   = 0x03,
  BMODE_OPEN_ALWAYS = 0x04
};

enum BundleAttribute {
  BUNDLE_EXTRA_PRIVATE = 1,
  BUNDLE_EXTRA_PUBLIC  = 2,
  BUNDLE_EXTRA_SYSTEM  = 3
};
#ifndef _BUNDLES_ORIGIN_ENUM_
# define _BUNDLES_ORIGIN_ENUM_
enum BundleFileOrigin {
  BUNDLE_FILE_ORIG_SET = 0,
  BUNDLE_FILE_ORIG_CUR = 1,
  BUNDLE_FILE_ORIG_END = 2
};
#endif // ifndef _BUNDLES_ORIGIN_ENUM_
typedef void *BundlePtr;
typedef void *CryptoCtx;

// открытие и закрытие бандла
BundlePtr BundleOpen(const char *filename,
                     int         mode);
BundlePtr BundleOpenFromStream(std::shared_ptr<IBinaryStream>stream,
                               int                           mode);
void      BundleClose(BundlePtr bundle);

// инициализация. в случае успеха возвращает 0
int       BundleInitialize(BundlePtr   bundle,
                           const void *pathKey,
                           int         keyLen);

// получение информации
int64_t BundleAttributeGet(BundlePtr       bundle,
                           BundleAttribute type,
                           void           *dst,
                           int64_t         dstOffset,
                           int64_t        *dstLen);
int64_t BundleAttributeSet(BundlePtr       bundle,
                           BundleAttribute type,
                           const void     *src,
                           int64_t         srcOffset,
                           const int64_t   srcLen);
int64_t BundleFileAttributeGet(BundlePtr bundle,
                               int       idx,
                               void     *dst,
                               int64_t   dstOffset,
                               int64_t  *dstLen,
                               CryptoCtx cryptoCtx);
int64_t BundleFileAttributeSet(BundlePtr     bundle,
                               int           idx,
                               const void   *src,
                               int64_t       srcOffset,
                               const int64_t srcLen,
                               CryptoCtx     cryptoCtx);

// копирует путь в строку и возвращает следующий индекс или -1 - если больше нет
// файлов, первый раз вызывать с idx=0
int BundleFileName(BundlePtr bundle,
                   int       idx,
                   char     *filename,
                   int       len);

// работа с криптографией
CryptoCtx BundleCreateCryptoContext(const void *key,
                                    int         keyLen);
void      BundleDestroyCryptoContext(CryptoCtx cryptoCtx);

// работа с файлами. Для всех операций чтения/записи с использованием
// криптоконтекста, буфер должен быть выровнен на 16 байт (блок AES)
int     BundleFileOpen(BundlePtr   bundle,
                       const char *filename,
                       int         openAlways);
int64_t BundleFileSeek(BundlePtr        bundle,
                       int              idx,
                       int64_t          offset,
                       BundleFileOrigin origin);
int64_t BundleFileLength(BundlePtr bundle,
                         int       idx);
int64_t BundleFileRead(BundlePtr bundle,
                       int       idx,
                       void     *dst,
                       int64_t   dstOffset,
                       int64_t  *dstLen,
                       CryptoCtx cryptoCtx);
int64_t BundleFileWrite(BundlePtr     bundle,
                        int           idx,
                        const void   *src,
                        int64_t       srcOffset,
                        const int64_t srcLen,
                        CryptoCtx     cryptoCtx);
void BundleFileDelete(BundlePtr bundle,
                      int       idx);

void Defragmentation(const char *fileSrc,
                     const char *fileTmp);
