# Makefile for CPP Bundles Library

all: libbundleslibrary.so

libbundleslibrary.so:	BundlesLibrary.o
	g++ -fPIC -shared -Wl,--no-undefined -o ./bin/libbundleslibrary.so -L./../libs/ -pthread  ./bin/BundlesLibrary.o ./bin/BundleFile.o ./bin/BinaryFile.o ./bin/aes.o
    
BundlesLibrary.o:	BundlesLibrary.cpp	BundleFile.o	BinaryFile.o
	g++ -Wall -fPIC -std=c++11 -c -I../mbedtls-2.4.0/include -DBUNDLELIB_DONT_USE_INTEGRATED_CRYPTO BundlesLibrary.cpp -o ./bin/BundlesLibrary.o -Ofast -L./../libs -I./../../cppcryptolib

BundleFile.o:	BundleFile.cpp	BinaryFile.o	aes.o
	g++ -Wall -fPIC -std=c++11 -c -I../mbedtls-2.4.0/include -DBUNDLELIB_DONT_USE_INTEGRATED_CRYPTO BundleFile.cpp -o ./bin/BundleFile.o -Ofast -I./../../cppcryptolib
    
aes.o:	./../mbedtls-2.4.0/library/aes.cpp
	mkdir -p ./bin
	g++ -Wall -fPIC -std=c++11 -I./../mbedtls-2.4.0/include -c ./../mbedtls-2.4.0/library/aes.cpp -o ./bin/aes.o -Ofast
    
BinaryFile.o:	./streams/BinaryFile.cpp
	mkdir -p ./bin
	g++ -Wall -fPIC -std=c++11 -c ./../include/BinaryFile.cpp -o ./bin/BinaryFile.o -Ofast
    
clean:
	rm -f ./bin/*.o ./bin/*.a ./bin/binary
