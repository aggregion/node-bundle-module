#include <time.h>
#include "CppUnitTest.h"
#include "..\BundlesLibrary\BundlesLibrary.h"
#include "windows.h"
#include "ppltasks.h"
#include "TestHelper.h"
using namespace concurrency;
using namespace HiloTests;

#define FILE_SIZE 10*1024LL

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#ifdef _WIN64
   #ifdef BUNDLES_WINRT
      #ifdef _DEBUG
        #pragma comment(lib,"..\\x64\\Windows RT Debug\\BundlesLibrary\\BundlesLibrary.lib")
      #else
        #pragma comment(lib,"..\\x64\\Windows RT Release\\BundlesLibrary\\BundlesLibrary.lib")
      #endif
   #else
      #ifdef _DEBUG
        #pragma comment(lib,"..\\x64\\Debug\\BundlesLibrary.lib")
      #else
        #pragma comment(lib,"..\\x64\\Release\\BundlesLibrary.lib")
      #endif
   #endif
#else
   #ifdef BUNDLES_WINRT
      #ifdef _DEBUG
        #pragma comment(lib,"..\\Windows RT Debug\\BundlesLibrary\\BundlesLibraryStore.lib")
      #else
        #pragma comment(lib,"..\\Windows RT Release\\BundlesLibrary\\BundlesLibraryStore.lib")
      #endif
   #else
      #ifdef _DEBUG
        #pragma comment(lib,"..\\Debug\\BundlesLibrary.lib")
      #else
        #pragma comment(lib,"..\\Release\\BundlesLibrary.lib")
      #endif
   #endif
#endif
namespace BundlesTest
{
   TEST_CLASS(BundleFileTest)
   {
      void *m_bundle;

      void MakeTest()
      {
         unsigned char key[] = { 0x4a, 0x12, 0x45, 0x6a, 0x2a, 0x4d, 0x27, 0xb8, 0xa5, 0x31, 0xd5, 0xb6, 0xfb, 0x68, 0x8a, 0x11 };
         unsigned char fileKey[] = { 0xb4, 0x9b, 0xf9, 0x93, 0xdb, 0xff, 0x09, 0x23, 0xa5, 0x31, 0x8a, 0x3c, 0x82, 0x50, 0xb5, 0x3f };
         char dataPrivate[] = { "Private data here!" };
         char dataPublic[]  = { "Public data here!" };
         char dataSystem[]  = { "System data here!" };
         bool error=false;

         char  buffer[1024] = {0};
         int64_t len = sizeof(buffer);
         m_bundle = NULL;

         srand(_time32(NULL));
         void *f1 = malloc(FILE_SIZE);
         void *f2 = malloc(FILE_SIZE);
         void *r  = malloc(FILE_SIZE);

         // ИНИЦИАЛИЗАЦИЯ
#ifdef BUNDLES_WINRT
         auto tmp = Windows::Storage::ApplicationData::Current->LocalFolder;
#else
         auto str = Windows::Storage::ApplicationData::Current->LocalFolder->Path + "\\test.bundle";
         std::wstring strW = str->Begin();
         std::string strA(strW.begin(),strW.end());
#endif

         if(f1!=NULL && f2!=NULL && r!=NULL)
         {
            // иницализируем содержимое
            for(size_t i=0;i<FILE_SIZE;i++)
            {
               ((char*)f1)[i] = rand() % 256;
               ((char*)f2)[i] = rand() % 256;
            }
#ifdef BUNDLES_WINRT
            auto myFile = tmp->CreateFileAsync("test.bundle",Windows::Storage::CreationCollisionOption::ReplaceExisting);
            auto file = create_task(myFile).get();
            m_bundle = BundleOpen(file,BMODE_READWRITE | BMODE_OPEN_ALWAYS);
#else
            m_bundle = BundleOpen(strA.c_str(),BMODE_READWRITE | BMODE_OPEN_ALWAYS);
#endif
         }
         else error = true;

         if (m_bundle)
         {
            // ТЕСТ №1. АТТРИБУТЫ
            BundleAttributeSet(m_bundle,BUNDLE_EXTRA_PRIVATE,dataPrivate,0,sizeof(dataPrivate)/2);
            BundleAttributeSet(m_bundle,BUNDLE_EXTRA_PUBLIC,dataPublic,0,sizeof(dataPublic));
            BundleAttributeSet(m_bundle,BUNDLE_EXTRA_SYSTEM,dataSystem,0,sizeof(dataSystem));
            // создаем сегментированный аттрибут
            BundleAttributeSet(m_bundle,BUNDLE_EXTRA_PRIVATE,dataPrivate,0,sizeof(dataPrivate));
            // проверим аттрибуты
            if(BundleAttributeGet(m_bundle,BUNDLE_EXTRA_PRIVATE,NULL,0,&len)!=0 || len!=sizeof(dataPrivate))
               error = true;
            else
            {
               if(BundleAttributeGet(m_bundle,BUNDLE_EXTRA_PRIVATE,buffer,0,&len)!=sizeof(dataPrivate) ||
                  memcmp(buffer,dataPrivate,sizeof(dataPrivate))!=0) error = true;
            }
            if(!error)
            {
               if(BundleAttributeGet(m_bundle,BUNDLE_EXTRA_PUBLIC,NULL,0,&len)!=0 || len!=sizeof(dataPublic))
                  error = true;
               else
                  if(BundleAttributeGet(m_bundle,BUNDLE_EXTRA_PUBLIC,buffer,0,&len)!=sizeof(dataPublic) ||
                     memcmp(buffer,dataPublic,sizeof(dataPublic))!=0) error = true;
            }
            if(!error)
            {
               if(BundleAttributeGet(m_bundle,BUNDLE_EXTRA_SYSTEM,NULL,0,&len)!=0 || len!=sizeof(dataSystem))
                  error = true;
               else
                  if(BundleAttributeGet(m_bundle,BUNDLE_EXTRA_SYSTEM,buffer,0,&len)!=sizeof(dataSystem) ||
                     memcmp(buffer,dataSystem,sizeof(dataSystem))!=0) error = true;
            }
            // ТЕСТ №2. ФАЙЛЫ
            if(!error)
            {
               // открываем несуществующий файл
               if(BundleFileOpen(m_bundle,"TestPath\\testfile.test",false)!=0)
                  error = true;
               // выставим ключ
               if(!BundleInitialize(m_bundle,key,sizeof(key))) error=true;
               // Создаем новый файл
               int idx1 = BundleFileOpen(m_bundle,"TestPath\\testfile1.test",true);
               int idx2 = BundleFileOpen(m_bundle,"TestPath\\testfile2.test",true);
               int idx3 = BundleFileOpen(m_bundle,"TestPath\\testfile3.test",true);
               if(idx1 <= 0 || idx2 <= 0 || idx3 <= 0) error = true;
               // тест с Seek
               if(!error)
               {
                  int64_t rSize = FILE_SIZE;
                  auto wrote = BundleFileWrite(m_bundle,idx3,(char*)f2,0,256,NULL);
                  auto pos = BundleFileSeek(m_bundle,idx3,-10,BundleFileOrigin::BUNDLE_FILE_ORIG_END);
                  wrote = BundleFileWrite(m_bundle,idx3,(char*)f2,0,10,NULL);
                  pos = BundleFileSeek(m_bundle,idx3,-10,BundleFileOrigin::BUNDLE_FILE_ORIG_CUR);
                  auto read = BundleFileRead(m_bundle,idx3,r,0,&rSize,NULL);
                  pos = BundleFileSeek(m_bundle,idx3,0,BundleFileOrigin::BUNDLE_FILE_ORIG_END);
                  pos = BundleFileSeek(m_bundle,idx3,0,BundleFileOrigin::BUNDLE_FILE_ORIG_END);
               }
               // создадим новый контекст
               void *cryptoContext = BundleCreateCryptoContext(fileKey,sizeof(fileKey));
               if(cryptoContext!=NULL)
               {
                  for(int i=0;i<10 && !error;i++)
                  {
                     // пишем данные
                     if(BundleFileWrite(m_bundle,idx1,(char*)f1+i*FILE_SIZE/10,0,FILE_SIZE/10,NULL)!=FILE_SIZE/10)
                        error=true;
                     // пишем шифрованные данные
                     if(!error && BundleFileWrite(m_bundle,idx2,(char*)f2+i*FILE_SIZE/10,0,FILE_SIZE/10,cryptoContext)!=FILE_SIZE/10)
                        error=true;
                  }
                  // проверим чтение
                  if(!error)
                  {
                     int64_t rSize = FILE_SIZE;
                     BundleFileSeek(m_bundle,idx1,0,BUNDLE_FILE_ORIG_SET);
                     BundleFileSeek(m_bundle,idx2,0,BUNDLE_FILE_ORIG_SET);
                     if(BundleFileRead(m_bundle,idx1,r,0,&rSize,NULL)!=FILE_SIZE || memcmp(r,f1,FILE_SIZE)!=0)
                        error=true;
                     if(!error && (BundleFileRead(m_bundle,idx2,r,0,&rSize,cryptoContext)!=FILE_SIZE || memcmp(r,f2,FILE_SIZE)!=0))
                        error=true;

                  }
                  // проверим Seek
                  if(!error)
                  {
                     int64_t pos=0;
                     int64_t rSize=100;
                     // 0 - 0
                     pos = BundleFileSeek(m_bundle,idx1,-100,BUNDLE_FILE_ORIG_SET);
                     if(pos != 0) error = true;
                     if(BundleFileRead(m_bundle,idx1,r,0,&rSize,NULL)!=100 || memcmp(r,f1,100)!=0)
                        error=true;
                     // 100 - 200
                     pos = BundleFileSeek(m_bundle,idx1,100,BUNDLE_FILE_ORIG_SET);
                     if(pos != 100) error = true;
                     if(BundleFileRead(m_bundle,idx1,r,0,&rSize,NULL)!=100 || memcmp(r,(char*)f1+100,100)!=0)
                        error=true;
                     // 1200 - 1300
                     pos = BundleFileSeek(m_bundle,idx1,1200,BUNDLE_FILE_ORIG_SET);
                     if(pos != 1200) error = true;
                     if(BundleFileRead(m_bundle,idx1,r,0,&rSize,NULL)!=100 || memcmp(r,(char*)f1+1200,100)!=0)
                        error=true;
                     // 1000 - 1100
                     pos = BundleFileSeek(m_bundle,idx1,-300,BUNDLE_FILE_ORIG_CUR);
                     if(pos != 1000) error = true;
                     if(BundleFileRead(m_bundle,idx1,r,0,&rSize,NULL)!=100 || memcmp(r,(char*)f1+1000,100)!=0)
                        error=true;
                     // 2000 - 2100
                     pos = BundleFileSeek(m_bundle,idx1,900,BUNDLE_FILE_ORIG_CUR);
                     if(pos != 2000) error = true;
                     if(BundleFileRead(m_bundle,idx1,r,0,&rSize,NULL)!=100 || memcmp(r,(char*)f1+2000,100)!=0)
                        error=true;
                     // 10240 - 10240
                     pos = BundleFileSeek(m_bundle,idx1,0,BUNDLE_FILE_ORIG_END);
                     if(pos != 10240) error = true;
                     if(BundleFileRead(m_bundle,idx1,r,0,&rSize,NULL)!=0)
                        error=true;
                     // 10240 - 10240
                     pos = BundleFileSeek(m_bundle,idx1,100,BUNDLE_FILE_ORIG_END);
                     if(pos != 10240) error = true;
                     if(BundleFileRead(m_bundle,idx1,r,0,&rSize,NULL)!=0)
                        error=true;
                     // 10140 - 10240
                     pos = BundleFileSeek(m_bundle,idx1,-100,BUNDLE_FILE_ORIG_END);
                     if(pos != 10140) error = true;
                     if(BundleFileRead(m_bundle,idx1,r,0,&rSize,NULL)!=100 || memcmp(r,(char*)f1+10140,100)!=0)
                        error=true;
                  }
                  // освободим крипто контекст
                  BundleDestroyCryptoContext(cryptoContext);
                  cryptoContext=NULL;
               }
               else error=true;
            }
            //
            BundleClose(m_bundle);
            m_bundle = NULL;
         }
         else
            error = true;
         // ОТКРЫТИЕ СУЩЕСТВУЮЩЕГО БАНДЛА (второй раз после дефрагментации)
         for(int i=0;i<2 && !error;i++)
         {
#ifdef BUNDLES_WINRT
            auto myFile = tmp->CreateFileAsync("test.bundle",Windows::Storage::CreationCollisionOption::OpenIfExists);
            auto file = create_task(myFile).get();
            m_bundle = BundleOpen(file,BMODE_READWRITE);
#else
            m_bundle = BundleOpen(strA.c_str(),BMODE_READWRITE);
#endif

            if (m_bundle)
            {
               // ТЕСТ №3. АТТРИБУТЫ
               if(BundleAttributeGet(m_bundle,BUNDLE_EXTRA_PRIVATE,NULL,0,&len)!=0 || len!=sizeof(dataPrivate))
                  error = true;
               else
               {
                  if(BundleAttributeGet(m_bundle,BUNDLE_EXTRA_PRIVATE,buffer,0,&len)!=sizeof(dataPrivate) ||
                     memcmp(buffer,dataPrivate,sizeof(dataPrivate))!=0) error = true;
               }
               if(!error)
               {
                  if(BundleAttributeGet(m_bundle,BUNDLE_EXTRA_PUBLIC,NULL,0,&len)!=0 || len!=sizeof(dataPublic))
                     error = true;
                  else
                     if(BundleAttributeGet(m_bundle,BUNDLE_EXTRA_PUBLIC,buffer,0,&len)!=sizeof(dataPublic) ||
                        memcmp(buffer,dataPublic,sizeof(dataPublic))!=0) error = true;
               }
               if(!error)
               {
                  if(BundleAttributeGet(m_bundle,BUNDLE_EXTRA_SYSTEM,NULL,0,&len)!=0 || len!=sizeof(dataSystem))
                     error = true;
                  else
                     if(BundleAttributeGet(m_bundle,BUNDLE_EXTRA_SYSTEM,buffer,0,&len)!=sizeof(dataSystem) ||
                        memcmp(buffer,dataSystem,sizeof(dataSystem))!=0) error = true;
               }
               // ТЕСТ №2. ФАЙЛЫ
               if(!error)
               {
                  // выставим ключ
                  if(!BundleInitialize(m_bundle,key,sizeof(key))) error=true;
                  // Создаем новый файл
                  int idx1 = BundleFileOpen(m_bundle,"TestPath\\testfile1.test",false);
                  int idx2 = BundleFileOpen(m_bundle,"TestPath\\testfile2.test",false);
                  if((idx1 <= 0 && i==0) || idx2 <= 0) error = true;
                  // создадим новый контекст
                  void *cryptoContext = BundleCreateCryptoContext(fileKey,sizeof(fileKey));
                  if(cryptoContext!=NULL)
                  {
                     // проверим чтение
                     if(!error)
                     {
                        int64_t rSize = FILE_SIZE;
                        if(i==0) BundleFileSeek(m_bundle,idx1,0,BUNDLE_FILE_ORIG_SET);
                        BundleFileSeek(m_bundle,idx2,0,BUNDLE_FILE_ORIG_SET);
                        if(i==0)
                        {
                           if(BundleFileRead(m_bundle,idx1,r,0,&rSize,NULL)!=FILE_SIZE || memcmp(r,f1,FILE_SIZE)!=0)
                              error=true;
                        }
                        if(!error && (BundleFileRead(m_bundle,idx2,r,0,&rSize,cryptoContext)!=FILE_SIZE || memcmp(r,f2,FILE_SIZE)!=0))
                           error=true;

                     }
                     // освободим крипто контекст
                     BundleDestroyCryptoContext(cryptoContext);
                     cryptoContext=NULL;
                  }
                  else error=true;
                  // удалим файл
                  BundleFileDelete(m_bundle,idx1);
               }
               //
               BundleClose(m_bundle);
               m_bundle = NULL;
            }
            else
               error = true;
            // ДЕФРАГМЕНТАЦИЯ
            if(i==0)
            {
#ifdef BUNDLES_WINRT
               auto myFileTmp = tmp->CreateFileAsync("test.m_bundle.tmp",Windows::Storage::CreationCollisionOption::ReplaceExisting);
               auto fileTmp = create_task(myFileTmp).get();
               Defragmentation(file,fileTmp);
#else
               std::string strTmp = strA + ".tmp";
               Defragmentation(strA.c_str(),strTmp.c_str());
#endif

            }
         }

         // ЗАВЕРШЕНИЕ
         // освободим память
         if(f1!=NULL) free(f1);
         if(f2!=NULL) free(f2);
         if(r!=NULL)  free(r);
         // прошли ли тест?
         if(error) throw std::exception();
      }

   public:
      TEST_METHOD(CreationMethod)
      {
         task_status st;
         auto tsk = create_task([this]()
         {
            MakeTest();
            int i=0;
         });
         TestHelper::RunSynced(tsk,st,false);
      }
   };
}