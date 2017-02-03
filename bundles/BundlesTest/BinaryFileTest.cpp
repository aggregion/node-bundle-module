#include "CppUnitTest.h"
#include "..\..\include\BinaryFile.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#define TEST_BUFFER_SIZE 1024*1024

namespace BundlesTest
{
	TEST_CLASS(BinaryFileTest)
	{
	public:
	
		TEST_METHOD(WriteTest)
		{
         CBinaryFile file(1200,1200);
         char *bufferRead =NULL;
         char *bufferWrite=NULL;
         bool  pass =false;
         bool  error=false;
         // инициализируем генератор случайных чисел
         srand(_time32(NULL));
         // выделим блоки памяти
         bufferRead =(char*)malloc(TEST_BUFFER_SIZE);
         bufferWrite=(char*)malloc(TEST_BUFFER_SIZE);
         if(bufferRead!=NULL && bufferWrite!=NULL)
         {
            // получим пусть к файлу
            auto tmp = Windows::Storage::ApplicationData::Current->TemporaryFolder;
            Platform::String ^str = tmp->Path;
            char str_a[_MAX_PATH] = {0};
            size_t cnv = 0;
            str+="\\file.test";
            wcstombs_s(&cnv,str_a,str->Data(),sizeof(str_a)-1);
            // создадим файл
            errno_t res=file.Open(str_a,"a+b");
            if(res==0)
            {
               // заполним буфер случайными данными
               for(size_t i=0;i<TEST_BUFFER_SIZE;i++)
                  bufferWrite[i]=rand()%256;
               // пишем в файл через кэш
               for(int i=0;!error && i<1024;i++)
                  if(file.Write(bufferWrite+(i*TEST_BUFFER_SIZE)/1024,TEST_BUFFER_SIZE/1024,(i*TEST_BUFFER_SIZE)/1024,false)!=TEST_BUFFER_SIZE/1024)
                     error=true;
               // флашим
               file.Flush();
               // пишем в файл без кеша
               for(int i=0;!error && i<1024;i++)
                  if(file.Write(bufferWrite+(i*TEST_BUFFER_SIZE)/1024,TEST_BUFFER_SIZE/1024,(i*TEST_BUFFER_SIZE)/1024,true)!=TEST_BUFFER_SIZE/1024)
                     error=true;
               // теперь читаем из файла с кешем
               for(int i=0;!error && i<TEST_BUFFER_SIZE;i++)
                  if(file.Read(bufferRead+i,1,i,false)!=1)
                     error=true;
               // проверим содержимое
               if(!error && memcmp(bufferWrite,bufferRead,TEST_BUFFER_SIZE)!=0)
                  error=true;
               // теперь читаем из файла без кеша
               if(!error && file.Read(bufferRead,TEST_BUFFER_SIZE,0,false)!=TEST_BUFFER_SIZE)
                  error=true;
               // проверим содержимое
               if(!error && memcmp(bufferWrite,bufferRead,TEST_BUFFER_SIZE)!=0)
                  error=true;
               // теперь читаем из файла большим блоком с кешем
               if(file.Read(bufferRead,TEST_BUFFER_SIZE,TEST_BUFFER_SIZE,true)!=TEST_BUFFER_SIZE)
                  error=true;
               // проверим содержимое
               if(!error && memcmp(bufferWrite,bufferRead,TEST_BUFFER_SIZE)==0)
                  pass=true;
               // закроем файл
               file.Close();
            }
         }
         // освободим память
         if(bufferRead!=NULL)  free(bufferRead);
         if(bufferWrite!=NULL) free(bufferWrite);
         // прошли ли тест?
         if(!pass) throw std::exception();
		}

	};
}