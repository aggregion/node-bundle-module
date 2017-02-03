#include <cstring>
#include <algorithm>
#ifdef _MSC_VER
# include <share.h>
#endif // ifdef _MSC_VER
#include <vector>
#include "BinaryFile.h"
std::wstring utf8_to_utf16(const std::string& utf8)
{
  std::vector<unsigned long> unicode;
  size_t i = 0;

  while (i < utf8.size())
  {
    unsigned long uni;
    size_t todo;
    unsigned char ch = utf8[i++];

    if (ch <= 0x7F)
    {
      uni  = ch;
      todo = 0;
    }
    else if (ch <= 0xBF)
    {
      throw std::logic_error("not a UTF-8 string");
    }
    else if (ch <= 0xDF)
    {
      uni  = ch & 0x1F;
      todo = 1;
    }
    else if (ch <= 0xEF)
    {
      uni  = ch & 0x0F;
      todo = 2;
    }
    else if (ch <= 0xF7)
    {
      uni  = ch & 0x07;
      todo = 3;
    }
    else
    {
      throw std::logic_error("not a UTF-8 string");
    }

    for (size_t j = 0; j < todo; ++j)
    {
      if (i == utf8.size()) throw std::logic_error("not a UTF-8 string");
      unsigned char ch = utf8[i++];

      if ((ch < 0x80) ||
          (ch > 0xBF)) throw std::logic_error("not a UTF-8 string");
      uni <<= 6;
      uni  += ch & 0x3F;
    }

    if ((uni >= 0xD800) && (uni <= 0xDFFF)) throw std::logic_error(
              "not a UTF-8 string");

    if (uni > 0x10FFFF) throw std::logic_error("not a UTF-8 string");
    unicode.push_back(uni);
  }
  std::wstring utf16;

  for (size_t i = 0; i < unicode.size(); ++i)
  {
    unsigned long uni = unicode[i];

    if (uni <= 0xFFFF)
    {
      utf16 += (wchar_t)uni;
    }
    else
    {
      uni   -= 0x10000;
      utf16 += (wchar_t)((uni >> 10) + 0xD800);
      utf16 += (wchar_t)((uni & 0x3FF) + 0xDC00);
    }
  }
  return utf16;
}

// конструктор
CBinaryFile::CBinaryFile(size_t cacheWriteSize, size_t cacheReadSize) {
  try
  {
    // создадим буферы
    if (cacheWriteSize > 0) m_writeCache = new char[cacheWriteSize];

    if (cacheReadSize > 0)  m_readCache = new char[cacheReadSize];

    // установим размеры
    m_writeCacheSize = cacheWriteSize;
    m_readCacheSize  = cacheReadSize;
#ifdef __GNUC__
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_locker, &attr);
#endif // ifdef __GNUC__
  }
  catch (...)
  {
    // удалим буферы
    if (m_readCache != nullptr)
    {
      delete[] (char *)m_readCache;
      m_readCache     = nullptr;
      m_readCacheSize = 0;
    }

    if (m_writeCache != nullptr)
    {
      delete[] (char *)m_writeCache;
      m_writeCache     = nullptr;
      m_writeCacheSize = 0;
    }
  }
}

// деструктор
CBinaryFile::~CBinaryFile(void)
{
  // закроем файл
  Close();

  // удалим буферы
  if (m_readCache != nullptr)
  {
    delete[] (char *)m_readCache;
    m_readCache     = nullptr;
    m_readPos       = 0;
    m_readSize      = 0;
    m_readCacheSize = 0;
  }

  if (m_writeCache != nullptr)
  {
    delete[] (char *)m_writeCache;
    m_writeCache     = nullptr;
    m_writePos       = 0;
    m_writeSize      = 0;
    m_writeCacheSize = 0;
  }
#ifdef __GNUC__
  pthread_mutex_destroy(&m_locker);
#endif // ifdef __GNUC__
}

// открытие файла
errno_t CBinaryFile::Open(const char *filename, const char *mode)
{
  // закроем предыдущий файл
  Close();

  // проверки параметров
  if ((filename == nullptr) || (mode == nullptr)) return EINVAL;

  // откроем файл
  try
  {
#ifndef _MSC_VER
    m_handle = fopen(filename, mode);
#else // ifndef _MSC_VER

    // сконвертируем в UNICODE
    std::wstring filenameW = utf8_to_utf16(filename);
    std::wstring modeW     = utf8_to_utf16(mode);

    // откроем
    m_handle = _wfsopen(filenameW.c_str(), modeW.c_str(), _SH_DENYWR);
#endif // ifndef _MSC_VER

    if (m_handle == nullptr) return errno;

    // запомним путь
    m_path = filename;
  }
  catch (...)
  {
    return errno;
  }
  // запомним режим
  m_canWrite = (strstr(mode, "r+") != nullptr || strstr(mode, "w")  != nullptr);
  m_canRead  = (strstr(mode, "r")  != nullptr || strstr(mode, "w+") != nullptr);

  // все ок
  return 0;
}

// закрытие
void CBinaryFile::Close()
{
  // сбросим данные
  Flush();

  // закроем файл
  if (m_handle != 0)
  {
    fclose(m_handle);
    m_handle = 0;
  }

  // очистим путь
  m_path.clear();
}

// получение пути к файлу
void CBinaryFile::Path(std::string& path)
{
  path = m_path;
}

// чтение из файла. возвращает число прочитанных байт
size_t CBinaryFile::Read(void  *buffer,
                         size_t size,
                         bool   ignoreCache)
{
  size_t readNeed  = 0;
  size_t read      = 0;
  size_t readTotal = 0;
  char  *dst       = (char *)buffer;

  // проверки
  if (m_handle == nullptr) return 0;

  if (!ignoreCache &&
      ((m_readCache == nullptr) || (m_readCacheSize == 0))) ignoreCache = true;

  if ((buffer == nullptr) &&
      (ignoreCache || (m_readCache == nullptr) ||
       (m_readCacheSize == 0))) return 0;

  // лочимся
#ifdef __GNUC__
  pthread_mutex_lock(&m_locker);
#else // ifdef __GNUC__
  m_locker.lock();
#endif // ifdef __GNUC__

  // флашим
  if (!m_read) Flush();

  // читаем и копируем
  while (size > 0)
  {
    // проверим на кэш-попадание
    if (!ignoreCache && (m_readCache != nullptr) && (m_readCacheSize > 0) &&
        (m_readPos <= m_curPos) && ((int64_t)(m_readPos + m_readSize) > m_curPos))
    {
      // скопируем из кэша
      readNeed =
        std::min(size,
                 static_cast<size_t>((int64_t)(m_readPos + m_readSize) -
                                     m_curPos));
      memcpy(dst, (char *)m_readCache + m_curPos - m_readPos, readNeed);

      // изменим счетчики
      size      -= readNeed;
      readTotal += readNeed;
      m_curPos  += readNeed;
      dst       += readNeed;

      // проверим, нужно ли еще считывать?
      if (size == 0) break;
    }

    // флашим, если была запись
    if (!m_read) _fflush_nolock(m_handle);

    // установим позицию в файле
    if (_fseeki64(m_handle, m_curPos, SEEK_SET) != 0) break;

    // выясним, сколько нужно читать
    readNeed = ignoreCache ? size : m_readCacheSize;

    // читаем
    read = _fread_nolock(ignoreCache ? dst : m_readCache, 1, readNeed, m_handle);

    // выставим флаг чтения
    m_read = true;

    if (read == 0) break;

    // изменим счетчики
    if (ignoreCache)
    {
      size      -= read;
      readTotal += read;
      m_curPos  += readNeed;
      dst       += readNeed;
    }
    else
    {
      m_readSize = read;
      m_readPos  = m_curPos;
    }
  }

  // анлочимся
#ifdef __GNUC__
  pthread_mutex_unlock(&m_locker);
#else // ifdef __GNUC__
  m_locker.unlock();
#endif // ifdef __GNUC__

  // вернем результат
  return readTotal;
}

// запись в файл. возвращает число записанных байт
size_t CBinaryFile::Write(void  *buffer,
                          size_t size,
                          bool   ignoreCache)
{
  size_t writeNeed  = 0;
  size_t wrote      = 0;
  size_t wroteTotal = 0;
  char  *dst        = (char *)buffer;

  // проверки
  if (m_handle == nullptr) return 0;

  if (!ignoreCache &&
      ((m_writeCache == nullptr) || (m_writeCacheSize == 0))) ignoreCache = true;

  if ((buffer == nullptr) &&
      (ignoreCache || (m_writeCache == nullptr) ||
       (m_writeCacheSize == 0))) return 0;


  // лочимся
#ifdef __GNUC__
  pthread_mutex_lock(&m_locker);
#else // ifdef __GNUC__
  m_locker.lock();
#endif // ifdef __GNUC__

  // пишем и копируем
  while (size > 0)
  {
    // работа с кешем?
    if (!ignoreCache)
    {
      // проверим, нужно ли скинуть данные
      if (((int64_t)(m_writePos + m_writeSize) != m_curPos) ||
          (m_writeSize == m_writeCacheSize))
      {
        if (!Flush()) break;

        // установим новую позицию
        m_writePos = m_curPos;
      }

      // запишем в буфер
      writeNeed = std::min(size, m_writeCacheSize - m_writeSize);
      memcpy((char *)m_writeCache + m_writeSize, dst, writeNeed);

      // сместим счетчики
      wroteTotal  += writeNeed;
      m_writeSize += writeNeed;
      dst         += writeNeed;
      m_curPos    += writeNeed;
      size        -= writeNeed;
    }
    else
    {
      // флашим, если было чтение
      if (m_read) _fflush_nolock(m_handle);

      // установим позицию в файле
      if (_fseeki64(m_handle, m_curPos, SEEK_SET) != 0) break;

      // пишем
      wrote = _fwrite_nolock(dst, 1, size, m_handle);

      // выставим флаг записи
      m_read = false;

      if (wrote == 0) break;

      // сместим счетчики
      wroteTotal += wrote;
      dst        += wrote;
      m_curPos   += wrote;
      size       -= wrote;

      // обнулим кэш чтения
      if (wrote > 0) m_readSize = 0;
    }
  }

  // анлочимся
#ifdef __GNUC__
  pthread_mutex_unlock(&m_locker);
#else // ifdef __GNUC__
  m_locker.unlock();
#endif // ifdef __GNUC__

  // вернем результат
  return wroteTotal;
}

// сброс данных на диск
bool CBinaryFile::Flush()
{
  if ((m_writeCache != nullptr) && (m_writeSize > 0))
  {
    // запишем данные
    if (Seek(m_writePos, SEEK_SET) &&
        (Write(m_writeCache, m_writeSize, true) != m_writeSize)) {
      return false;
    }

    // сбросим
    m_writeSize = 0;
    m_writePos  = 0;

    // обнулим кэш чтения
    m_readSize = 0;
  }

  // флашим
  _fflush_nolock(m_handle);

  // все ок
  return true;
}

// получение размера файла
int64_t CBinaryFile::Size()
{
  if (_fseeki64(m_handle, 0, SEEK_END) != 0) return 0;

  return _ftelli64(m_handle);
}

// установка новой позиции
bool CBinaryFile::Seek(int64_t pos, int origin)
{
  if (_fseeki64(m_handle, pos, origin) == 0) {
    m_curPos = _ftelli64(m_handle);
    return true;
  }
  return false;
}
