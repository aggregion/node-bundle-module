#pragma once
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <string.h>
//
#ifndef _MSC_VER
# ifndef _time32
#  define _time32 time
# endif // ifndef _time32
# ifndef errno_t
typedef int errno_t;
#  define NO_ERROR 0
# endif // ifndef errno_t
#endif  // ifndef _MSC_VER

#ifndef rsize_t
# define rsize_t size_t
#endif // ifndef rsize_t

// константы
#define BUNDLE_VERSION 1
#define BUNDLE_CACHE_SIZE (4 * 1024 * 1024)
#define BUNDLE_SIGNATURE "AZBUKA"
#define BUNDLE_ATTRS_COUNT 4
#define BUNDLE_BLOCK_HDRS_CNT 128

#pragma pack(push,1)

#ifndef _BUNDLES_ORIGIN_ENUM_
# define _BUNDLES_ORIGIN_ENUM_
enum BundleFileOrigin
{
  BUNDLE_FILE_ORIG_SET = 0,
  BUNDLE_FILE_ORIG_CUR = 1,
  BUNDLE_FILE_ORIG_END = 2
};
#endif // ifndef _BUNDLES_ORIGIN_ENUM_
enum BundleFileFlags
{
  BUNDLE_FILE_FLAG_EMPTY = 0x001,     // флаг пустого заголовка (файл не
                                      // существует)
  BUNDLE_FILE_FLAG_ENC_ATTR0 = 0x002, // зашифрованный атрибут 0
  BUNDLE_FILE_FLAG_ENC_ATTR1 = 0x004, // зашифрованный атрибут 1
  BUNDLE_FILE_FLAG_ENC_ATTR2 = 0x008, // зашифрованный атрибут 2
  BUNDLE_FILE_FLAG_ENC_ATTR3 = 0x010  // зашифрованный атрибут 3
};
enum BundleFileAttribute
{
  BUNDLE_FILE_DATA  = 0,
  BUNDLE_FILE_NAME  = 1,
  BUNDLE_FILE_ATTRS = 2
};

// основная информация бандла
typedef struct
{
  char bundleSign[6]; // "AZBUKA"
  int  version;       // версия бандла
} BundleInfo;

// блок информации
typedef struct BundleBlock
{
  int64_t size      = 0; // размер блока
  int64_t nextBlock = 0; // смещение от начала бандла к к следующему блоку (0 -
                         // если блок последний)
} BundleBlock;

// информация о файле внутри бандла
typedef struct BundleFileInfo
{
  unsigned char flags = 0;                 // флаги файла
                                           // (BundleFileFlags)
  int64_t attrsBlocks[BUNDLE_ATTRS_COUNT]; // смещения на блоки атрибутов файла
  char    reserved[15];                    // зарезервировано для экстра-данных
  BundleFileInfo() {
    memset(attrsBlocks, 0, sizeof(attrsBlocks));
    memset(reserved,    0, sizeof(reserved));
  }
} BundleFileInfo;
#pragma pack(pop)

// определим структуру для хранения
typedef struct BundleFileDesc
{
  BundleFileInfo info;        // информация о файле
  int64_t        infoPos = 0; // смещение инфо от начала
  // информация для содержимого файла
  int64_t curBlock    = 0;    // смещение текущего блока от начала бандла
  int64_t curBlockPos = 0;    // позиция в текущем блоке
  // путь
  std::string path = "";      // путь к файлу
} BundleFileDesc;

#ifndef AES_BLOCK_SIZE
# define AES_BLOCK_SIZE 16
#endif // ifndef AES_BLOCK_SIZE
typedef struct
{
  mbedtls_aes_context ctxDec;
  mbedtls_aes_context ctxEnc;
} AesContext;
typedef std::vector<BundleFileDesc>   CFilesDesc;
typedef std::map<std::string, size_t> CFilesIndex;
typedef std::map<int64_t, BundleBlock>CBundleBlocks;
