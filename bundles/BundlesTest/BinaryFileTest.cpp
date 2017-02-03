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
         // �������������� ��������� ��������� �����
         srand(_time32(NULL));
         // ������� ����� ������
         bufferRead =(char*)malloc(TEST_BUFFER_SIZE);
         bufferWrite=(char*)malloc(TEST_BUFFER_SIZE);
         if(bufferRead!=NULL && bufferWrite!=NULL)
         {
            // ������� ����� � �����
            auto tmp = Windows::Storage::ApplicationData::Current->TemporaryFolder;
            Platform::String ^str = tmp->Path;
            char str_a[_MAX_PATH] = {0};
            size_t cnv = 0;
            str+="\\file.test";
            wcstombs_s(&cnv,str_a,str->Data(),sizeof(str_a)-1);
            // �������� ����
            errno_t res=file.Open(str_a,"a+b");
            if(res==0)
            {
               // �������� ����� ���������� �������
               for(size_t i=0;i<TEST_BUFFER_SIZE;i++)
                  bufferWrite[i]=rand()%256;
               // ����� � ���� ����� ���
               for(int i=0;!error && i<1024;i++)
                  if(file.Write(bufferWrite+(i*TEST_BUFFER_SIZE)/1024,TEST_BUFFER_SIZE/1024,(i*TEST_BUFFER_SIZE)/1024,false)!=TEST_BUFFER_SIZE/1024)
                     error=true;
               // ������
               file.Flush();
               // ����� � ���� ��� ����
               for(int i=0;!error && i<1024;i++)
                  if(file.Write(bufferWrite+(i*TEST_BUFFER_SIZE)/1024,TEST_BUFFER_SIZE/1024,(i*TEST_BUFFER_SIZE)/1024,true)!=TEST_BUFFER_SIZE/1024)
                     error=true;
               // ������ ������ �� ����� � �����
               for(int i=0;!error && i<TEST_BUFFER_SIZE;i++)
                  if(file.Read(bufferRead+i,1,i,false)!=1)
                     error=true;
               // �������� ����������
               if(!error && memcmp(bufferWrite,bufferRead,TEST_BUFFER_SIZE)!=0)
                  error=true;
               // ������ ������ �� ����� ��� ����
               if(!error && file.Read(bufferRead,TEST_BUFFER_SIZE,0,false)!=TEST_BUFFER_SIZE)
                  error=true;
               // �������� ����������
               if(!error && memcmp(bufferWrite,bufferRead,TEST_BUFFER_SIZE)!=0)
                  error=true;
               // ������ ������ �� ����� ������� ������ � �����
               if(file.Read(bufferRead,TEST_BUFFER_SIZE,TEST_BUFFER_SIZE,true)!=TEST_BUFFER_SIZE)
                  error=true;
               // �������� ����������
               if(!error && memcmp(bufferWrite,bufferRead,TEST_BUFFER_SIZE)==0)
                  pass=true;
               // ������� ����
               file.Close();
            }
         }
         // ��������� ������
         if(bufferRead!=NULL)  free(bufferRead);
         if(bufferWrite!=NULL) free(bufferWrite);
         // ������ �� ����?
         if(!pass) throw std::exception();
		}

	};
}