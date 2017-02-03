#pragma once
#ifdef __GNUC__
# include <pthread.h>
#else // ifdef __GNUC__
# include <mutex>
#endif // ifdef __GNUC__
#include <errno.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <stdexcept>
#include "IBinaryStream.h"

//
#define _FILE_OFFSET_BITS 64

#ifndef _MSC_VER
# ifndef _fflush_nolock
#  define _fflush_nolock fflush
# endif // ifndef _fflush_nolock

# ifndef _fwrite_nolock
#  define _fwrite_nolock fwrite
# endif // ifndef _fwrite_nolock

# ifndef _fread_nolock
#  define _fread_nolock fread
# endif // ifndef _fread_nolock

# ifndef _fseeki64
#  define _fseeki64 fseeko
# endif // ifndef _fseeki64

# ifndef errno_t
#  define errno_t int
# endif // ifndef errno_t

# ifndef _ftelli64
#  define _ftelli64 ftello
# endif // ifndef _ftelli64
#endif  // ifndef _MSC_VER

// класс для работы с бинарным файлом с использованием кэша
// Класс является потоково-безопасным
class CBinaryFile : public IBinaryStream {
private:

  FILE *m_handle = nullptr; // хэндл открытого файла

#ifdef __GNUC__
  pthread_mutex_t m_locker;
#else // ifdef __GNUC__
  std::recursive_mutex m_locker;  // локер
#endif // ifdef __GNUC__
  std::string m_path;             // путь к файлу
  bool m_read = false;            // флаг того, что последняя операция
                                  // чтения
  bool m_canRead   = false;       // флаг возможности чтения
  bool m_canWrite  = false;       // флаг возможности записи
  int64_t m_curPos = 0;           // текущая позиция в файле
  // кэш чтения
  void   *m_readCache = nullptr;  // буфер кэша чтения
  int64_t m_readPos   = 0;        // позиция в файле, с которой прочитали
                                  // данные
  size_t m_readSize      = 0;     // размер прочитанных в кэш данных
  size_t m_readCacheSize = 0;     // размер кэша
  // кэш записи
  void   *m_writeCache = nullptr; // буфер кэша записи
  int64_t m_writePos   = 0;       // позиция в файле, с которой начали
                                  // запись
  size_t m_writeSize      = 0;    // размер записанных в кэш данных
  size_t m_writeCacheSize = 0;    // размер кэша

public:

  CBinaryFile(size_t cacheWriteSize,
              size_t cacheReadSize);
  ~CBinaryFile(void);

  void Path(std::string& path);
  bool IsReadable() {
    return m_canRead;
  }

  bool IsWritable() {
    return m_canWrite;
  }

  bool EndOfFile()  {
    return Size() == _ftelli64(m_handle);
  }

  // открытие/закрытие
  errno_t Open(const char *filename,
               const char *mode);
  void    Close();

  // чтение/запись. возвращают число прочитанных/записанных байт
  size_t  Read(void  *buffer,
               size_t size,
               bool   ignoreCache);
  size_t  Write(void  *buffer,
                size_t size,
                bool   ignoreCache);

  // флаш данных на диск
  bool    Flush();

  // работа с позицией и размером
  int64_t Size();
  bool    Seek(int64_t pos,
               int     origin);
};
