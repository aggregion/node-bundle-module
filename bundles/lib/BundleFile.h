#pragma once
#ifdef __GNUC__
# include <pthread.h>
#else // ifdef __GNUC__
# include <mutex>
#endif // ifdef __GNUC__
#include <memory>
#include "mbedtls/aes.h"
#include "BundleFileHDRs.h"
#include "streams/IBinaryStream.h"

// класс, работающий с бандлом
class CBundleFile {
private:

  std::shared_ptr<IBinaryStream> m_bundle; // поток бандла
  bool m_initialized;                      // флаг инициализации
  bool m_created;                          // флаг создания
#ifdef __GNUC__
  pthread_mutex_t m_locker;                // лок
#else // ifdef __GNUC__
  std::recursive_mutex m_locker;           // лок
#endif // ifdef __GNUC__

  // криптография
  AesContext *m_AesPathContext;
  void       *m_AesBuffer;      // буфер для криптографии
  size_t      m_AesBufferSize;  // размер буфера
  // информация о бандле
  BundleInfo  m_info;           // инфо бандла
  CFilesDesc *m_filesDesc;      // файлы бандла
  CFilesIndex
  *m_filesIdx;                  // индекс для поиска по пути
  CBundleBlocks *m_blocksCache; // кэш блоков
  int
    m_emptyHeadersCount;        // число пустых заголовков для превыделения

public:

  CBundleFile(std::shared_ptr<IBinaryStream>bundleStream,
              int                           emptyHeadersCount = BUNDLE_BLOCK_HDRS_CNT);
  ~CBundleFile();

  // открытие/закрытие
  errno_t Open(bool openAlways);
  void    Close();

  // инициализация
  bool    Initialize(const void *pathKey,
                     int         keyLen,
                     size_t      cryptoBufferSize);

  // получение информации (атрибуты, данные и т.д.)
  int64_t BundleAttributeGet(int      idx,
                             int      type,
                             void    *dst,
                             int64_t *dstLen,
                             void    *cryptoContext);
  int64_t BundleAttributeSet(int           idx,
                             int           type,
                             const void   *src,
                             const int64_t srcLen,
                             void         *cryptoContext);

  // работа с файлами
  int     FileOpen(const char *filename,
                   bool        openAlways);
  int64_t FileSeek(int              idx,
                   int64_t          offset,
                   BundleFileOrigin origin);
  void    FileTrunk(int     idx,
                    int64_t newSize);
  int64_t FileSize(int idx);
  void    FileDelete(int idx);
  int     FileName(int   idx,
                   char *filename,
                   int   len);

  // служебная функция
  static bool Defragmentation(std::shared_ptr<IBinaryStream>src,
                              std::shared_ptr<IBinaryStream>dst);

private:

  errno_t BundleOpen(int mode,
                     int emptyHeadersCount);
  errno_t CreateNewBundle();
  bool    AddEmptyHeaders();
  bool    ReadHeaders();
  bool    ReadPaths();
  int64_t CryptoContentRead(int64_t& blockPos,
                            int64_t& blockOffset,
                            void    *dst,
                            int64_t *dstLen,
                            void    *cryptoContext);
  int64_t CryptoContentWrite(int64_t   & blockPos,
                             int64_t   & blockOffset,
                             const void *src,
                             int64_t     srcLen,
                             int64_t    *firstBlock,
                             void       *cryptoContext,
                             bool        padding);
  int64_t ContentRead(int64_t& blockPos,
                      int64_t& blockOffset,
                      void    *dst,
                      int64_t *dstLen);
  int64_t ContentWrite(int64_t   & blockPos,
                       int64_t   & blockOffset,
                       const void *src,
                       int64_t     srcLen,
                       int64_t    *firstBlock = nullptr);
  void            CalculateSize(int64_t  blockPos,
                                int64_t  blockOffset,
                                int64_t *dstLen);
  BundleBlock   * BlockLoad(int64_t blockPos);
  BundleBlock   * BlockStore(int64_t      blockPos,
                             BundleBlock& block);
  void            BlockTrunk(int64_t blockPos,
                             int64_t newSize);
  BundleFileInfo* InfoLoad(int64_t infoPos);
  bool            InfoStore(int64_t       & infoPos,
                            BundleFileInfo& info,
                            bool            infoSystem);
  int64_t         CollectBlocks(int64_t               start,
                                int64_t               end,
                                std::vector<int64_t>& dest);
  int64_t         FileSeekForward(int64_t& curBlock,
                                  int64_t& curBlockPos,
                                  int64_t  offset);
  int64_t         FileSeekBackward(int64_t             & curBlock,
                                   int64_t             & curBlockPos,
                                   int64_t               offset,
                                   std::vector<int64_t>& blocks);

  // служебная функция
  static bool AttributeCopy(CBundleFile& srcBundle,
                            CBundleFile& dstBundle,
                            int          idxSrc,
                            int          idxDst,
                            int          type,
                            void        *buffer,
                            size_t       bufferSize);
};
