{
  "targets": [
    {
      "target_name": "bundledlibrary",
      "sources": [ "bundles/lib/streams/BinaryFile.cpp", "bundles/lib/BundleFile.cpp", "bundles/lib/BundlesLibrary.cpp", "bundles/mbedtls-2.4.0/library/aes.c" ],
      "cflags!": [ '-fno-exceptions' ],
      "cflags_cc!": [ '-fno-exceptions' ]
    }
  ]
}
