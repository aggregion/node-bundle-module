#pragma once
#include <stdint.h>
#include <cstddef>

// интерфейс, работающий с бинарным потоком
class IBinaryStream {
public:

  virtual ~IBinaryStream() {}

  virtual bool    IsReadable() = 0;
  virtual bool    IsWritable() = 0;
  virtual bool    EndOfFile()  = 0;
  virtual size_t  Read(void  *buffer,
                       size_t size,
                       bool   ignoreCache) = 0;
  virtual size_t  Write(void  *buffer,
                        size_t size,
                        bool   ignoreCache) = 0;
  virtual bool    Flush()                   = 0;
  virtual int64_t Size()                    = 0;
  virtual bool    Seek(int64_t pos,
                       int     origin) = 0;
  virtual void    Close()              = 0;
};
