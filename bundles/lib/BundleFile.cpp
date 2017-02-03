#include <string>
#include <vector>
#include <memory.h>
#include <map>
#include <errno.h>
#include <time.h>
#include <algorithm>

#include "BundlesLibrary.h"
#include "BundleFile.h"

// Конструктор
CBundleFile::CBundleFile(std::shared_ptr<IBinaryStream>bundleStream,
                         int                           emptyHeadersCount)
  : m_bundle(bundleStream), m_initialized(false), m_created(false),
  m_AesPathContext(nullptr), m_AesBuffer(nullptr),
  m_emptyHeadersCount(emptyHeadersCount) {
#ifdef __GNUC__
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&m_locker, &attr);
#endif // ifdef __GNUC__
  m_filesDesc   = new CFilesDesc;
  m_filesIdx    = new CFilesIndex;
  m_blocksCache = new CBundleBlocks;

  // проверим
  m_created = m_bundle != nullptr && m_filesDesc != nullptr
              && m_filesIdx != nullptr
              && m_blocksCache != nullptr;

  // обнулим данные
  memset(&m_info, 0, sizeof(m_info));
}

// деструктор
CBundleFile::~CBundleFile() {
  // закроем файл
  Close();

  // обнулим данные
  memset(&m_info, 0, sizeof(m_info));

  // удалим крипто контекст
  if (m_AesPathContext != nullptr) {
    delete m_AesPathContext;
    m_AesPathContext = nullptr;
  }

  // удалим буфер
  if (m_AesBuffer != nullptr) {
    free(m_AesBuffer);
    m_AesBuffer = nullptr;
  }

  if (m_filesDesc != nullptr) {
    delete m_filesDesc;
  }

  if (m_filesIdx != nullptr) {
    delete m_filesIdx;
  }

  if (m_blocksCache != nullptr) {
    delete m_blocksCache;
  }
#ifdef __GNUC__
  pthread_mutex_destroy(&m_locker);
#endif // ifdef __GNUC__
}

// открытие бандла
errno_t CBundleFile::Open(bool openAlways) {
  // проверки параметров
  if (!m_created) {
    return ENOMEM;
  }

  // лочимся
#ifdef __GNUC__
  pthread_mutex_lock(&m_locker);
#else // ifdef __GNUC__
  std::lock_guard<std::recursive_mutex> locker(m_locker);
#endif // ifdef __GNUC__
  errno_t err = EEXIST;

  // читаем заголовок
  if (m_bundle->Seek(0, SEEK_SET)) {
    if ((m_bundle->Read(&m_info, sizeof(m_info), false) == sizeof(m_info))
        && (memcmp(m_info.bundleSign, BUNDLE_SIGNATURE, sizeof(m_info.bundleSign)) == 0)) {
      // читаем первый информационный блок
      BundleBlock *block = BlockLoad(sizeof(m_info));

      if ((block != nullptr) && (block->size >= (int64_t)sizeof(BundleFileInfo))) {
        // прочтем заголовки
        if (ReadHeaders()) {
          err = 0;
        }
      }
    }
  }

  // при необходимости создадим новый бандл
  if ((err == EEXIST) && openAlways) {
    err = CreateNewBundle();
  }

  // анлочим
#ifdef __GNUC__
  pthread_mutex_unlock(&m_locker);
#endif // ifdef __GNUC__

  // вернем результат
  return err;
}

// создание нового бандла
errno_t CBundleFile::CreateNewBundle() {
  errno_t err = 0;

  // сместимся в начало
  m_bundle->Seek(0, SEEK_SET);

  // основной хидер
  m_info.version = BUNDLE_VERSION;
  memcpy(m_info.bundleSign, BUNDLE_SIGNATURE, strlen(BUNDLE_SIGNATURE));

  if (!m_bundle->Seek(0,
                      SEEK_SET) ||
      (m_bundle->Write(&m_info, sizeof(m_info), true) != sizeof(m_info))) {
    err = errno;
  }

  if (err == 0) {
    // пишем первый информационный блок
    BundleBlock nblock;
    nblock.size = sizeof(BundleFileInfo);

    if (BlockStore(sizeof(m_info), nblock) == nullptr) {
      err = errno;
    }

    // запишем нулевой элемент
    if (err == 0) {
      BundleFileDesc descZero;

      // во избежания рекурсии сразу установим ссылку на файл 0го элемента
      descZero.info.attrsBlocks[BUNDLE_FILE_DATA] = sizeof(m_info);

      // нулевой элемент указывает на информацию об остальных элементах
      descZero.curBlock = sizeof(m_info);

      // запишем первый элемент
      if (m_bundle->Seek(sizeof(m_info) + sizeof(BundleBlock), SEEK_SET) &&
          (m_bundle->Write(&descZero.info, sizeof(descZero.info), true) != sizeof(descZero.info))) {
        err = errno;
      }

      // добавим в вектор
      if (err == 0) {
        m_filesDesc->push_back(descZero);
      }

      // запишем остальные элементы
      AddEmptyHeaders();
    }
  }

  // вернем результат
  return err;
}

// добавление блока пустых заголовков
bool CBundleFile::AddEmptyHeaders() {
  std::vector<BundleFileInfo> tmpInfos(m_emptyHeadersCount);

  // забьем мусором
#ifndef __GNUC__
  srand(_time32(nullptr));
#else // ifndef __GNUC__
  srand(time(nullptr));
#endif // ifndef __GNUC__

  for (size_t i = 0, j = tmpInfos.size(); i < j; i++) {
    // забьем мусором
    for (size_t l = 0, m = sizeof(BundleFileInfo); l < m; l++) {
      ((char *)&tmpInfos[i])[l] = rand() % 255;
    }

    // выставим флаг пустого заголовка
    tmpInfos[i].flags |= BUNDLE_FILE_FLAG_EMPTY;
  }

  // перейдем в конец
  FileSeek(0, 0, BUNDLE_FILE_ORIG_END);

  // запишем на диск
  return BundleAttributeSet(0, BUNDLE_FILE_DATA, &tmpInfos[0],
                            tmpInfos.size() * sizeof(BundleFileInfo),
                            nullptr) == (int64_t)(tmpInfos.size() * sizeof(BundleFileInfo));
}

// закрытие
void CBundleFile::Close() {
  if (!m_created) {
    return;
  }

  // лочимся
#ifdef __GNUC__
  pthread_mutex_lock(&m_locker);
#else // ifdef __GNUC__
  std::lock_guard<std::recursive_mutex> locker(m_locker);
#endif // ifdef __GNUC__

  // закроем файл
  m_bundle->Close();

  // обнулим данные
  memset(&m_info, 0, sizeof(m_info));
  m_initialized = false;

  // анлочим
#ifdef __GNUC__
  pthread_mutex_unlock(&m_locker);
#endif // ifdef __GNUC__
}

// Инициализация
bool CBundleFile::Initialize(const void *pathKey, int keyLen,
                             size_t cryptoBufferSize) {
  bool ret = false;

  // проверки
  if (!m_created) {
    return false;
  }

  if (pathKey == nullptr) {
    return false;
  }

  // лочимся
#ifdef __GNUC__
  pthread_mutex_lock(&m_locker);
#else // ifdef __GNUC__
  std::lock_guard<std::recursive_mutex> locker(m_locker);
#endif // ifdef __GNUC__

  // сбросим флаг
  m_initialized = false;

  // создадим буфер
  void *nbuffer = realloc(m_AesBuffer, cryptoBufferSize);

  if (nbuffer != nullptr) {
    m_AesBuffer     = nbuffer;
    m_AesBufferSize = cryptoBufferSize;

    // инициализируем воркер
    m_AesPathContext = new AesContext;

    if ((mbedtls_aes_setkey_enc(&m_AesPathContext->ctxEnc, (const unsigned char *)pathKey,
                                keyLen * 8) != 0)
        || (mbedtls_aes_setkey_dec(&m_AesPathContext->ctxDec, (const unsigned char *)pathKey,
                                   keyLen * 8) != 0)) {
      delete m_AesPathContext;
      m_AesPathContext = nullptr;
    }

    if (m_AesPathContext != nullptr) {
      // прочтем заголовки
      ReadPaths();

      // выставим флаг
      m_initialized = ret = true;
    }
  }

  // анлочим
#ifdef __GNUC__
  pthread_mutex_unlock(&m_locker);
#endif // ifdef __GNUC__

  // вернем результат
  return ret;
}

// получение атрибутов
int64_t CBundleFile::BundleAttributeGet(int idx, int type, void *dst,
                                        int64_t  *dstLen, void *cryptoContext) {
  int64_t ret = 0;

  // проверка
  if (!m_created) {
    return 0;
  }

  if (type >= BUNDLE_ATTRS_COUNT) {
    return 0;
  }

  // лочимся
#ifdef __GNUC__
  pthread_mutex_lock(&m_locker);
#else // ifdef __GNUC__
  std::lock_guard<std::recursive_mutex> locker(m_locker);
#endif // ifdef __GNUC__

  // получим значения
  if ((idx >= 0) && (idx < (int)m_filesDesc->size())) {
    if (((*m_filesDesc)[idx].info.flags & BUNDLE_FILE_FLAG_EMPTY) == 0) {
      // проверим на атрибуты или данные
      if (type != BUNDLE_FILE_DATA) {
        int64_t tmp1 = (*m_filesDesc)[idx].info.attrsBlocks[type];
        int64_t tmp2 = 0;

        if ((cryptoContext != nullptr) && (dst != nullptr) && (dstLen != nullptr)) {
          ret = CryptoContentRead(tmp1, tmp2, dst, dstLen, cryptoContext);
        } else {
          ret = ContentRead(tmp1, tmp2, dst, dstLen);
        }
      } else {
        if ((cryptoContext != nullptr) && (dst != nullptr) && (dstLen != nullptr)) {
          ret = CryptoContentRead((*m_filesDesc)[idx].curBlock,
                                  (*m_filesDesc)[idx].curBlockPos, dst, dstLen, cryptoContext);
        } else {
          ret = ContentRead((*m_filesDesc)[idx].curBlock, (*m_filesDesc)[idx].curBlockPos,
                            dst, dstLen);
        }
      }
    } else {
      if (dstLen != nullptr) {
        *dstLen = 0;
      }
    }
  }

  // анлочим
#ifdef __GNUC__
  pthread_mutex_unlock(&m_locker);
#endif // ifdef __GNUC__

  // вернем результат
  return ret;
}

// установка атрибутов
int64_t CBundleFile::BundleAttributeSet(int idx, int type, const void *src,
                                        const int64_t  srcLen, void *cryptoContext) {
  int64_t ret         = 0;
  int64_t firstBlock  = 0;
  bool    infoChanged = false;

  // проверка
  if (!m_created) {
    return 0;
  }

  if (type >= BUNDLE_ATTRS_COUNT) {
    return 0;
  }

  // лочимся
#ifdef __GNUC__
  pthread_mutex_lock(&m_locker);
#else // ifdef __GNUC__
  std::lock_guard<std::recursive_mutex> locker(m_locker);
#endif // ifdef __GNUC__

  // запишем значения
  if ((idx >= 0) && (idx < (int)m_filesDesc->size())
      && (((*m_filesDesc)[idx].info.flags & BUNDLE_FILE_FLAG_EMPTY) == 0)) {
    // выставим тип для файла
    infoChanged = ((*m_filesDesc)[idx].info.attrsBlocks[type] <= 0);

    // если меняется флаг шифрования
    if ((cryptoContext != nullptr) && (src != nullptr)) {
      unsigned char flag = BUNDLE_FILE_FLAG_ENC_ATTR0 << type;

      // выставим флаг шифрования
      if (((*m_filesDesc)[idx].info.flags & flag) == 0) {
        (*m_filesDesc)[idx].info.flags |= flag;
        infoChanged                     = true;
      }
    }

    // проверим на атрибуты или данные
    if (type != BUNDLE_FILE_DATA) {
      int64_t tmp1 = (*m_filesDesc)[idx].info.attrsBlocks[type];
      int64_t tmp2 = 0;

      if ((cryptoContext != nullptr) && (src != nullptr)) {
        ret = CryptoContentWrite(tmp1, tmp2, src, srcLen, &firstBlock, cryptoContext, true);
      } else {
        ret = ContentWrite(tmp1, tmp2, src, srcLen, &firstBlock);
      }

      // обрежем данные
      if (ret > 0) {
        BlockTrunk(tmp1, tmp2);
      }
    } else {
      // выставим текущий блок
      if ((*m_filesDesc)[idx].curBlock == 0) {
        (*m_filesDesc)[idx].curBlock = (*m_filesDesc)[idx].info.attrsBlocks[type];
      }

      if ((cryptoContext != nullptr) && (src != nullptr)) {
        ret = CryptoContentWrite((*m_filesDesc)[idx].curBlock,
                                 (*m_filesDesc)[idx].curBlockPos,
                                 src,
                                 srcLen,
                                 &firstBlock,
                                 cryptoContext,
                                 false);
      } else {
        ret = ContentWrite((*m_filesDesc)[idx].curBlock, (*m_filesDesc)[idx].curBlockPos,
                           src, srcLen, &firstBlock);
      }
    }

    // для первого блока запишем его значение
    if (infoChanged) {
      (*m_filesDesc)[idx].info.attrsBlocks[type] = firstBlock;
    }
  }

  // запишем инфо
  if (infoChanged && (ret > 0)
      && !InfoStore((*m_filesDesc)[idx].infoPos, (*m_filesDesc)[idx].info, (idx == 0))) {
    ret = 0;
  }

  // анлочим
#ifdef __GNUC__
  pthread_mutex_unlock(&m_locker);
#endif // ifdef __GNUC__

  // вернем результат
  return ret;
}

// открытие файла
int CBundleFile::FileOpen(const char *filename, bool openAlways) {
  int res = -1;

  // проверки
  if (!m_created) {
    return 0;
  }

  if ((filename == nullptr) || !m_initialized) {
    return 0;
  }
  std::string tmpStr(filename);

  // лочимся
#ifdef __GNUC__
  pthread_mutex_lock(&m_locker);
#else // ifdef __GNUC__
  std::lock_guard<std::recursive_mutex> locker(m_locker);
#endif // ifdef __GNUC__

  // ищем файл
  CFilesIndex::iterator it = m_filesIdx->find(tmpStr);

  // не нашли? добавим
  if ((it == m_filesIdx->end()) || (it->second >= m_filesDesc->size())) {
    // проверим, можно ли создавать
    if (openAlways) {
      BundleFileDesc desc;

      // запишем инфо
      if (InfoStore(desc.infoPos, desc.info, false)) {
        // добавим новый заголовок
        m_filesDesc->push_back(desc);
        res = (int)m_filesDesc->size() - 1;

        // добавим имя
        std::string cpy(filename);

        // сохраним на диск
        if (BundleAttributeSet(res, BUNDLE_FILE_NAME, cpy.data(), cpy.length() * sizeof(char),
                               m_AesPathContext) != (int64_t)(cpy.length() * sizeof(char))) {
          res = -1;
        } else {
          (*m_filesIdx)[cpy.c_str()] = res;
          (*m_filesDesc)[res].path   = cpy;
        }
      }
    }
  } else {
    res = (int)it->second;

    // выставим позицию
    (*m_filesDesc)[res].curBlock =
      (*m_filesDesc)[res].info.attrsBlocks[BUNDLE_FILE_DATA];
    (*m_filesDesc)[res].curBlockPos = 0;
  }

  // анлочим
#ifdef __GNUC__
  pthread_mutex_unlock(&m_locker);
#endif // ifdef __GNUC__

  // вернем результат
  return res;
}

void CBundleFile::FileDelete(int idx) {
  if (!m_created) {
    return;
  }

  // лочимся
#ifdef __GNUC__
  pthread_mutex_lock(&m_locker);
#else // ifdef __GNUC__
  std::lock_guard<std::recursive_mutex> locker(m_locker);
#endif // ifdef __GNUC__

  if ((idx > 0) && (idx < (int)m_filesDesc->size())
      && (((*m_filesDesc)[idx].info.flags & BUNDLE_FILE_FLAG_EMPTY) == 0)) {
    // выставим флаг
    (*m_filesDesc)[idx].info.flags |= BUNDLE_FILE_FLAG_EMPTY;

    CFilesIndex::iterator it = m_filesIdx->find((*m_filesDesc)[idx].path.c_str());

    if (it != m_filesIdx->end()) {
      m_filesIdx->erase(it);
    }

    // запишем
    InfoStore((*m_filesDesc)[idx].infoPos, (*m_filesDesc)[idx].info, false);
  }

  // анлочим
#ifdef __GNUC__
  pthread_mutex_unlock(&m_locker);
#endif // ifdef __GNUC__
}

// получение следующего имени файла
int CBundleFile::FileName(int idx, char *filename, int len) {
  int ret = -1;

  if (!m_created) {
    return 0;
  }

  // лочимся
#ifdef __GNUC__
  pthread_mutex_lock(&m_locker);
#else // ifdef __GNUC__
  std::lock_guard<std::recursive_mutex> locker(m_locker);
#endif // ifdef __GNUC__

  // сразу сместимся
  idx++;

  // ищем свободный элемент
  while (idx >= 0 && idx < (int)m_filesDesc->size()
         && ((*m_filesDesc)[idx].info.flags & BUNDLE_FILE_FLAG_EMPTY) ==
         BUNDLE_FILE_FLAG_EMPTY) {
    idx++;
  }

  // нашли что-нибудь?
  if ((idx >= 0) && (idx < (int)m_filesDesc->size()) && (filename != nullptr) && (len > 0)) {
    // скопируем имя
#ifndef _MSC_VER
    strcpy(filename, (*m_filesDesc)[idx].path.c_str());
#else // ifndef _MSC_VER
    strcpy_s(filename, len, (*m_filesDesc)[idx].path.c_str());
#endif // ifndef _MSC_VER

    // запомним результат
    ret = idx;
  }

  // анлочим
#ifdef __GNUC__
  pthread_mutex_unlock(&m_locker);
#endif // ifdef __GNUC__

  // вернем результат
  return ret;
}

// получение размера файла
int64_t CBundleFile::FileSize(int idx) {
  int64_t res = 0;

  if (!m_created) {
    return 0;
  }

  // лочимся
#ifdef __GNUC__
  pthread_mutex_lock(&m_locker);
#else // ifdef __GNUC__
  std::lock_guard<std::recursive_mutex> locker(m_locker);
#endif // ifdef __GNUC__

  if ((idx >= 0) && (idx < (int)m_filesDesc->size())
      && (((*m_filesDesc)[idx].info.flags & BUNDLE_FILE_FLAG_EMPTY) == 0)) {
    // посчитаем размер
    CalculateSize((*m_filesDesc)[idx].info.attrsBlocks[BUNDLE_FILE_DATA], 0, &res);
  }

  // анлочим
#ifdef __GNUC__
  pthread_mutex_unlock(&m_locker);
#endif // ifdef __GNUC__

  // вернем результат
  return res;
}

// установка новой позиции в файле. возвращает полное смещение в бандле от начала
int64_t CBundleFile::FileSeek(int idx, int64_t offset, BundleFileOrigin origin) {
  std::vector<int64_t> blocks;
  int64_t ret = 0;

  // проверки
  if (!m_created) {
    return 0;
  }

  if ((origin < BUNDLE_FILE_ORIG_SET) || (origin > BUNDLE_FILE_ORIG_END)) {
    return -1;
  }

  // лочимся
#ifdef __GNUC__
  pthread_mutex_lock(&m_locker);
#else // ifdef __GNUC__
  std::lock_guard<std::recursive_mutex> locker(m_locker);
#endif // ifdef __GNUC__

  // запишем значения
  if ((idx >= 0) && (idx < (int)m_filesDesc->size())
      && (((*m_filesDesc)[idx].info.flags & BUNDLE_FILE_FLAG_EMPTY) == 0)) {
    // скорректируем смещение
    if (origin == BUNDLE_FILE_ORIG_END) {
      (*m_filesDesc)[idx].curBlock    = 0;
      (*m_filesDesc)[idx].curBlockPos = 0;

      if (offset > 0) {
        offset = 0;
      }
    }

    // при необходимости соберем список блоков
    if ((origin == BUNDLE_FILE_ORIG_END) || (origin == BUNDLE_FILE_ORIG_CUR)) {
      ret = CollectBlocks((*m_filesDesc)[idx].info.attrsBlocks[BUNDLE_FILE_DATA],
                          (*m_filesDesc)[idx].curBlock, blocks);
    }

    // определим метод прохода
    if ((origin == BUNDLE_FILE_ORIG_END) || ((origin == BUNDLE_FILE_ORIG_CUR)
                                             && (offset < 0))) {
      // смещаемся
      ret += FileSeekBackward((*m_filesDesc)[idx].curBlock,
                              (*m_filesDesc)[idx].curBlockPos, offset, blocks);
    } else {
      // выставим начало
      if ((origin != BUNDLE_FILE_ORIG_CUR) || ((*m_filesDesc)[idx].curBlock <= 0)) {
        (*m_filesDesc)[idx].curBlock =
          (*m_filesDesc)[idx].info.attrsBlocks[BUNDLE_FILE_DATA];
        (*m_filesDesc)[idx].curBlockPos = 0;
        ret                             = 0;
      }

      // двигаемся вперед
      if (offset > 0) {
        ret += FileSeekForward((*m_filesDesc)[idx].curBlock,
                               (*m_filesDesc)[idx].curBlockPos, offset);
      }
    }
  }

  // анлочим
#ifdef __GNUC__
  pthread_mutex_unlock(&m_locker);
#endif // ifdef __GNUC__

  // вернем результат
  return ret;
}

// сбор стэка блоков
int64_t CBundleFile::CollectBlocks(int64_t start, int64_t end,
                                   std::vector<int64_t>& dest) {
  int64_t ret           = 0;
  BundleBlock *tmpBlock = nullptr;

  while (start != end && (tmpBlock = BlockLoad(start)) != nullptr) {
    // добавим в стэк
    dest.push_back(start);
    ret += tmpBlock->size;

    // сместимся
    start = tmpBlock->nextBlock;
  }

  // вернем результат
  return ret;
}

// установка позиции в прямом порядке
int64_t CBundleFile::FileSeekForward(int64_t& curBlock, int64_t& curBlockPos,
                                     int64_t offset) {
  int64_t pos = 0;

  // добавим смещения
  offset     += curBlockPos;
  curBlockPos = 0;

  // пройдем в цикле
  for (BundleBlock *bb = BlockLoad(curBlock); bb != nullptr;) {
    if (offset >= bb->size) {
      offset -= bb->size;
      pos    += bb->size;
    } else {
      curBlockPos = offset;
      pos        += offset;
      break;
    }

    // сместимся к следующему блоку
    if (bb->nextBlock > 0) {
      curBlock = bb->nextBlock;
      bb       = BlockLoad(curBlock);
    } else {
      break;
    }
  }

  // вернем результат
  return pos;
}

// установка позиции в обратном порядке
int64_t CBundleFile::FileSeekBackward(int64_t& curBlock, int64_t& curBlockPos,
                                      int64_t offset, std::vector<int64_t>& blocks) {
  int64_t pos = 0;

  // сменим знак на положительный
  offset = -offset;

  // обработаем текущий блок (если есть)
  if (curBlock > 0) {
    // укладывается ли смещение в текущий блок?
    if (curBlockPos >= offset) {
      curBlockPos -= offset;
      return curBlockPos;
    } else {
      offset     -= curBlockPos;
      curBlockPos = 0;
    }
  } else {
    curBlockPos = 0;
  }

  // пройдем в цикле
  for (size_t i = blocks.size(); i > 0; i--) {
    BundleBlock *bb = BlockLoad(blocks[i - 1]);

    if (bb == nullptr) {
      break;
    }

    // запомним блок
    curBlock = blocks[i - 1];

    // укладывается ли смещение в текущий блок?
    if (bb->size >= offset) {
      curBlockPos = bb->size - offset;
      pos        -= offset;
      break;
    } else {
      offset -= bb->size;
      pos    -= bb->size;
    }
  }

  // вернем результат
  return pos;
}

// обрезание файла по заданному размеру
void CBundleFile::FileTrunk(int idx, int64_t newSize) {
  // проверки
  if (!m_created) {
    return;
  }

  if (newSize < 0) {
    return;
  }

  // лочимся
#ifdef __GNUC__
  pthread_mutex_lock(&m_locker);
#else // ifdef __GNUC__
  std::lock_guard<std::recursive_mutex> locker(m_locker);
#endif // ifdef __GNUC__

  // обрежем
  if ((idx >= 0) && (idx < (int)m_filesDesc->size())
      && (((*m_filesDesc)[idx].info.flags & BUNDLE_FILE_FLAG_EMPTY) == 0)) {
    // сместимся
    FileSeek(idx, newSize, BUNDLE_FILE_ORIG_SET);

    // обрежем
    BlockTrunk((*m_filesDesc)[idx].curBlock, (*m_filesDesc)[idx].curBlockPos);
  }

  // анлочим
#ifdef __GNUC__
  pthread_mutex_unlock(&m_locker);
#endif // ifdef __GNUC__
}

// обрезание файла по заданному размеру
void CBundleFile::BlockTrunk(int64_t blockPos, int64_t newSize) {
  // получим блок
  BundleBlock *bb = BlockLoad(blockPos);

  if ((bb != nullptr) && (bb->size > newSize)) {
    // обрежем
    bb->size      = newSize;
    bb->nextBlock = 0;

    // сохраним блок
    BlockStore(blockPos, *bb);
  }
}

// чтение заголовков файлов
bool CBundleFile::ReadHeaders() {
  int64_t totalSize  = 0;
  int64_t countSize  = 0;
  int64_t mainPos    = sizeof(BundleInfo);
  int64_t mainOffset = 0;
  std::vector<BundleFileInfo> tmpVect;
  BundleFileDesc desc;

  // получим общую длину заголовков
  CalculateSize(mainPos, mainOffset, &totalSize);
  countSize = totalSize / sizeof(BundleFileInfo);

  // проверка на пустой бандл
  if (countSize > 0) {
    // реаллоцируем
    tmpVect.resize((size_t)countSize);

    if (countSize != (int64_t)tmpVect.size()) {
      return false;
    }

    // сместимся на первый элемент
    FileSeek(0, 0, BUNDLE_FILE_ORIG_SET);

    // читаем заголовки
    if (totalSize != ContentRead(mainPos, mainOffset, &tmpVect[0], &totalSize)) {
      return false;
    }

    // добавим заголовки
    for (int64_t i = 0; i < countSize; i++) {
      desc.info     = tmpVect[(size_t)i];
      desc.curBlock = desc.curBlockPos = 0;
      desc.infoPos  = sizeof(BundleFileInfo) * i;

      // добавим в вектор только значимые заголовки
      if ((desc.info.flags & BUNDLE_FILE_FLAG_EMPTY) == 0) {
        m_filesDesc->push_back(desc);
      }
    }
  }

  // все ок
  return true;
}

// расшифровка путей файлов
bool CBundleFile::ReadPaths() {
  int64_t totalSize = 0;
  int64_t readSize  = 0;
  std::string tmpStr(2048, '\0');

  // пройдем по списку заголовков
  for (size_t i = 1, j = m_filesDesc->size(); i < j; i++) {
    // загрузим путь
    totalSize = tmpStr.size() * sizeof(tmpStr[0]);
    readSize  = BundleAttributeGet((int)i, BUNDLE_FILE_NAME, &tmpStr[0], &totalSize,
                                   m_AesPathContext);

    // занулим
    if ((readSize >= 0) && (readSize <= totalSize)) {
      tmpStr[(size_t)readSize / sizeof(tmpStr[0])] = '\0';
    }

    // добавим в индекс по пути
    if (!tmpStr.empty()) {
      (*m_filesIdx)[tmpStr.c_str()] = i;
      (*m_filesDesc)[i].path        = tmpStr;
    }
  }

  // все ок
  return true;
}

// чтение данных с последующей расшифровкой
int64_t CBundleFile::CryptoContentRead(int64_t& blockPos, int64_t& blockOffset,
                                       void *dst, int64_t *dstLen, void *cryptoContext) {
  // проверки
  if ((cryptoContext == nullptr) || (dst == nullptr) || (dstLen == nullptr)
      || (m_AesBuffer == nullptr)
      || (m_AesBufferSize == 0)) {
    return 0;
  }

  if ((*dstLen % AES_BLOCK_SIZE) != 0) {
    return 0;
  }

  // читаем данные в буфер
  int64_t remain    = *dstLen;
  int64_t totalRead = 0;

  while (remain > 0) {
    int64_t toRead = std::min(remain, (int64_t)m_AesBufferSize);
    int64_t read   = ContentRead(blockPos, blockOffset, m_AesBuffer, &toRead);

    // проверим, есть ли выровненные данные?
    if (((read % AES_BLOCK_SIZE) != 0) || (read <= 0)) {
      break;
    }

    // расшифруем
    for (size_t pos = 0; pos < static_cast<size_t>(read); pos += AES_BLOCK_SIZE) {
      mbedtls_aes_crypt_ecb(&((AesContext *)cryptoContext)->ctxDec, MBEDTLS_AES_DECRYPT,
                            (unsigned char *)m_AesBuffer + pos, (unsigned char *)m_AesBuffer + pos);
    }

    // скопируем
    memcpy((char *)dst + totalRead, m_AesBuffer, (rsize_t)read);

    // сдвинем счетчики
    totalRead += read;
    remain    -= read;
  }

  // вернем результат
  return totalRead;
}

// запись данных с шифрованием
int64_t CBundleFile::CryptoContentWrite(int64_t   & blockPos,
                                        int64_t   & blockOffset,
                                        const void *src,
                                        int64_t     srcLen,
                                        int64_t    *firstBlock,
                                        void       *cryptoContext,
                                        bool        padding) {
  // проверки
  if ((cryptoContext == nullptr) || (src == nullptr) || (m_AesBuffer == nullptr)
      || (m_AesBufferSize == 0)) {
    return 0;
  }

  if (((srcLen % AES_BLOCK_SIZE) != 0) && !padding) {
    return 0;
  }

  // читаем данные в буфер
  int64_t remain     = srcLen;
  int64_t totalWrote = 0;

  while (remain > 0) {
    int64_t toRead = std::min(remain, (int64_t)m_AesBufferSize);

    if (toRead <= 0) {
      break;
    }

    // скопируем исходные данные
    memcpy(m_AesBuffer, (char *)src + totalWrote, (rsize_t)toRead);

    // вычислим размер для шифрования
    int64_t toWrite = (toRead % AES_BLOCK_SIZE) == 0 ? toRead : toRead +
                      AES_BLOCK_SIZE - toRead % AES_BLOCK_SIZE;

    // добьем нулями
    if (toWrite > toRead) {
      memset((char *)m_AesBuffer + toRead, 0, (size_t)(toWrite - toRead));
    }

    // зашифруем
    for (size_t pos = 0; pos < static_cast<size_t>(toWrite); pos += AES_BLOCK_SIZE) {
      mbedtls_aes_crypt_ecb(&((AesContext *)cryptoContext)->ctxEnc, MBEDTLS_AES_ENCRYPT,
                            (unsigned char *)m_AesBuffer + pos, (unsigned char *)m_AesBuffer + pos);
    }

    // запишем на диск
    int64_t wrote = ContentWrite(blockPos, blockOffset, m_AesBuffer, toWrite,
                                 firstBlock);

    // проверим
    if (wrote != toWrite) {
      break;
    }

    // сдвинем счетчики
    totalWrote += toRead;
    remain     -= toRead;
  }

  // вернем результат
  return totalWrote;
}

// чтение контента
int64_t CBundleFile::ContentRead(int64_t& blockPos, int64_t& blockOffset,
                                 void *dst, int64_t  *dstLen) {
  int64_t res    = 0;
  int64_t remain = 0;

  // проверки
  if (dstLen == nullptr) {
    return 0;
  }

  // проверим на операцию запроса длины
  if (dst == nullptr) {
    // посчитаем размер
    CalculateSize(blockPos, blockOffset, dstLen);
    return 0;
  }

  // читаем данные
  remain = *dstLen;

  for (BundleBlock *bb = BlockLoad(blockPos); bb != nullptr && remain > 0;) {
    int64_t toRead = std::min(bb->size - blockOffset, remain);

    if (toRead < 0) {
      break;
    }

    if (toRead > 0) {
      // читаем из файла
      m_bundle->Seek(blockPos + blockOffset + sizeof(BundleBlock), SEEK_SET);
      size_t read = m_bundle->Read((char *)dst + res, (size_t)toRead, false);

      // сместим счетчики
      res         += read;
      remain      -= read;
      blockOffset += read;

      // тут выход в случае ошибки
      if (read != (size_t)toRead) {
        return res + read;
      }
    }

    // если нужно - смещаемся к следующему блоку
    if ((remain > 0) && (bb->nextBlock > 0)) {
      blockPos    = bb->nextBlock;
      blockOffset = 0;
      bb          = BlockLoad(blockPos);
    } else {
      bb = nullptr;
    }
  }

  // вернем результат
  return res;
}

// запись контента
int64_t CBundleFile::ContentWrite(int64_t& blockPos, int64_t& blockOffset,
                                  const void *src, int64_t srcLen, int64_t *firstBlock) {
  int64_t res     = 0;
  int64_t remain  = srcLen;
  BundleBlock *bb = nullptr;

  // проверки
  if (src == nullptr) {
    return 0;
  }

  // пишем данные
  for (bb = BlockLoad(blockPos); bb != nullptr && remain > 0;) {
    int64_t toWrite = std::min(bb->size - blockOffset, remain);

    if (toWrite < 0) {
      break;
    }

    if (toWrite > 0) {
      // пишем в файл
      m_bundle->Seek(blockPos + blockOffset + sizeof(BundleBlock), SEEK_SET);
      size_t wrote = m_bundle->Write((char *)src + res, (size_t)toWrite, false);

      // сместим счетчики
      res         += wrote;
      remain      -= wrote;
      blockOffset += wrote;

      // тут выход в случае ошибки
      if (wrote != (size_t)toWrite) {
        return res + wrote;
      }
    }

    // если нужно - смещаемся к следующему блоку
    if (bb->nextBlock > 0) {
      if (remain > 0) {
        blockPos    = bb->nextBlock;
        blockOffset = 0;
        bb          = BlockLoad(blockPos);

        // проверим на ошибку
        if (bb == nullptr) {
          remain = 0;
        }
      }
    } else {
      break;
    }
  }

  // проверим, хватило ли блоков?
  if (remain > 0) {
    BundleBlock bbn;
    bbn.size = remain;

    // выясним, куда вставлять
    int64_t pos = m_bundle->Size();

    // проверим, можно ли расширить блок?
    if ((bb != nullptr) && (pos == blockPos + bb->size + (int64_t)sizeof(BundleBlock))) {
      // запишем данные
      m_bundle->Seek(pos, SEEK_SET);

      if (m_bundle->Write((char *)src + res, (size_t)remain, false) == (size_t)remain) {
        // установим новый размер блока
        bb->size += remain;
        BlockStore(blockPos, *bb);

        // сместим счетчики
        res        += remain;
        blockOffset = bb->size;
      }
    } else

    // вставим новый блок
    if (BlockStore(pos, bbn) != nullptr) {
      // запишем данные
      if (m_bundle->Seek(pos + sizeof(BundleBlock), SEEK_SET) &&
          (m_bundle->Write((char *)src + res, (size_t)remain, false) == (size_t)remain)) {
        // запомним первый блок
        // вставим новый блок в список
        if (bb != nullptr) {
          bb->nextBlock = pos;
          BlockStore(blockPos, *bb);
        } else {
          // запомним позицию первого блока
          if (firstBlock != nullptr) {
            *firstBlock = pos;
          }
        }

        // сместим счетчики
        res        += remain;
        blockOffset = remain;
        blockPos    = pos;
      }
    }
  }

  // вернем результат
  return res;
}

// вычисление оставшегося размера
void CBundleFile::CalculateSize(int64_t blockPos, int64_t blockOffset,
                                int64_t *dstLen) {
  int64_t res = 0;

  // проверки
  if ((blockPos < (int64_t)sizeof(m_info)) || (dstLen == nullptr)) {
    return;
  }

  // считаем по размеру блоков
  for (BundleBlock *bl = BlockLoad(blockPos); bl != nullptr;
       bl = BlockLoad(bl->nextBlock)) {
    res += bl->size;
  }

  // отнимем смещение в первом блоке
  res -= res > blockOffset ? blockOffset : res;

  // выставим значение
  *dstLen = res;
}

// загрузка блока
BundleBlock * CBundleFile::BlockLoad(int64_t blockPos) {
  BundleBlock *res = nullptr;

  // проверки
  if (blockPos < (int64_t)sizeof(m_info)) {
    return nullptr;
  }

  // проверим, есть ли блок в кэше?
  CBundleBlocks::iterator it = m_blocksCache->find(blockPos);

  if (it == m_blocksCache->end()) {
    BundleBlock block;
    m_bundle->Seek(blockPos, SEEK_SET);

    if (m_bundle->Read(&block, sizeof(block), false) == sizeof(block)) {
      (*m_blocksCache)[blockPos] = block;
      res                        = &(*m_blocksCache)[blockPos];
    }
  } else {
    res = &it->second;
  }

  // вернем результат
  return res;
}

// сохранение блока
BundleBlock * CBundleFile::BlockStore(int64_t blockPos, BundleBlock& block) {
  BundleBlock *res = nullptr;

  // проверки
  if (blockPos < (int64_t)sizeof(m_info)) {
    return nullptr;
  }

  // сохраним
  m_bundle->Seek(blockPos, SEEK_SET);

  if (m_bundle->Write(&block, sizeof(block), true) == sizeof(block)) {
    (*m_blocksCache)[blockPos] = block;
    res                        = &(*m_blocksCache)[blockPos];
  }

  // вернем результат
  return res;
}

// загрузка информации о файле
BundleFileInfo * CBundleFile::InfoLoad(int64_t /*infoPos*/) {
  return nullptr;
}

// сохранение информации о файле
bool CBundleFile::InfoStore(int64_t& infoPos, BundleFileInfo& info,
                            bool infoSystem) {
  // установим позицию
  if (!infoSystem) {
    if (infoPos <= 0) {
      infoPos = m_filesDesc->size() * sizeof(BundleFileInfo);

      // проверим, нужно ли добавлять заголовки
      if (FileSize(0) == infoPos) {
        AddEmptyHeaders();
      }
    }
    FileSeek(0, infoPos, BUNDLE_FILE_ORIG_SET);
  } else {
    FileSeek(0, 0, BUNDLE_FILE_ORIG_SET);
  }

  // сохраним
  return BundleAttributeSet(0, BUNDLE_FILE_DATA, &info, sizeof(info),
                            nullptr) == sizeof(info);
}

// дефрагментация. жадная - создает новый временный файл для копии
bool CBundleFile::Defragmentation(std::shared_ptr<IBinaryStream>src,
                                  std::shared_ptr<IBinaryStream>dst) {
  // проверки
  if ((src == nullptr) || (dst == nullptr) || !src->IsReadable() || !dst->IsWritable()) {
    return false;
  }

  // создадим бандлы
  CBundleFile srcBundle(src);
  CBundleFile dstBundle(dst);
  bool ret    = true;
  int  cnt    = 0;
  int64_t pos = 0;
  BundleFileInfo emptyInfo;
  BundleFileDesc emptyDesc;
  void *buffer = malloc(BUNDLE_CACHE_SIZE);

  if (buffer == nullptr) {
    return false;
  }

  // откроем бандлы
  if ((srcBundle.Open(false) == 0) && (dstBundle.Open(true) == 0)) {
    // посчитаем число не удаленных файлов
    for (size_t i = 0, j = srcBundle.m_filesDesc->size(); i < j; i++)
      if (((*srcBundle.m_filesDesc)[i].info.flags & BUNDLE_FILE_FLAG_EMPTY) == 0) {
        cnt++;
      }
    cnt = 0;

    // в цикле скопируем свойства
    for (size_t i = 0, j = srcBundle.m_filesDesc->size(); i < j && ret; i++) {
      if (((*srcBundle.m_filesDesc)[i].info.flags & BUNDLE_FILE_FLAG_EMPTY) ==
          BUNDLE_FILE_FLAG_EMPTY) {
        continue;
      }

      // создаем новый инфо
      if (cnt > 0) {
        // добавим новый заголовок
        dstBundle.m_filesDesc->push_back(emptyDesc);
        pos = cnt * sizeof(BundleFileInfo);
        dstBundle.InfoStore(pos, emptyInfo, false);
      }

      // берем все свойства кроме файловых
      for (int l = 0; l < BUNDLE_ATTRS_COUNT && ret; l++) {
        if ((l == BUNDLE_FILE_DATA)
            || ((*srcBundle.m_filesDesc)[i].info.attrsBlocks[l] <= 0)) {
          continue;
        }

        // копируем
        if (!AttributeCopy(srcBundle, dstBundle, (int)i, cnt, l, buffer, BUNDLE_CACHE_SIZE)) {
          ret = false;
          continue;
        }
      }

      //
      cnt++;
    }

    // для копирования файлов основной заголовок не нужен
    cnt = 1;

    // теперь копируем файлы
    for (size_t i = 1, j = srcBundle.m_filesDesc->size(); i < j && ret; i++) {
      if (((*srcBundle.m_filesDesc)[i].info.flags & BUNDLE_FILE_FLAG_EMPTY) ==
          BUNDLE_FILE_FLAG_EMPTY) {
        continue;
      }

      // берем только файлы
      for (int l = 0; l < BUNDLE_ATTRS_COUNT && ret; l++) {
        if ((l != BUNDLE_FILE_DATA)
            || ((*srcBundle.m_filesDesc)[i].info.attrsBlocks[l] <= 0)) {
          continue;
        }

        // копируем
        if (!AttributeCopy(srcBundle, dstBundle, (int)i, cnt, l, buffer, BUNDLE_CACHE_SIZE)) {
          ret = false;
          continue;
        }
      }

      //
      cnt++;
    }

    // сохраним все инфо на диск
    for (size_t i = 0, j = dstBundle.m_filesDesc->size(); i < j && ret; i++) {
      pos = i * sizeof(BundleFileInfo);
      dstBundle.InfoStore(pos, (*dstBundle.m_filesDesc)[i].info, i == 0);
    }
  } else {
    ret = false;
  }

  // закроем бандлы
  srcBundle.Close();
  dstBundle.Close();

  // освободим буфер
  free(buffer);

  // вернем результат
  return ret;
}

// копирование атрибутов. функция внутренняя и служебная, никаких проверок!
bool CBundleFile::AttributeCopy(CBundleFile& srcBundle, CBundleFile& dstBundle,
                                int idxSrc, int idxDst, int type, void *buffer, size_t bufferSize) {
  int64_t remain           = 0;
  int64_t blockPosRead     = (*srcBundle.m_filesDesc)[idxSrc].info.attrsBlocks[type];
  int64_t blockOffsetRead  = 0;
  int64_t blockPosWrite    = 0;
  int64_t blockOffsetWrite = 0;
  int64_t firstBlock       = 0;

  srcBundle.CalculateSize(blockPosRead, blockOffsetRead, &remain);

  while (remain > 0) {
    int64_t toRead = std::min(remain, (int64_t)bufferSize);

    // читаем очередной блок
    if (srcBundle.ContentRead(blockPosRead, blockOffsetRead, buffer, &toRead) != toRead) {
      return false;
    }

    // пишем
    if (dstBundle.ContentWrite(blockPosWrite, blockOffsetWrite, buffer, toRead,
                               &firstBlock) != toRead) {
      return false;
    }

    // сместим счетчики
    remain -= toRead;
  }

  // сохраним инфо
  (*dstBundle.m_filesDesc)[idxDst].info.attrsBlocks[type] = firstBlock;
  (*dstBundle.m_filesDesc)[idxDst].info.flags            |=
    (*srcBundle.m_filesDesc)[idxDst].info.flags & (BUNDLE_FILE_FLAG_ENC_ATTR0 <<
                                                   type);

  // все ок
  return true;
}
