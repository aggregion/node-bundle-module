{
    "targets":  [
	{
	    "target_name":    "BundlesLibrary",
	    "type":           "static_library",
            "include_dirs":	[
        	"bundles/mbedtls-2.4.0/include/"
            ],
            "sources":		[
        	"bundles/lib/BundleFile.cpp",
                "bundles/lib/BundlesLibrary.cpp",
                "bundles/lib/streams/BinaryFile.cpp",
                "bundles/mbedtls-2.4.0/library/aes.c",
                "bundles/mbedtls-2.4.0/library/aesni.c",
                "bundles/mbedtls-2.4.0/library/padlock.c"
            ],
            "cflags_cxx":     [ "-std=c++11"],
            "cflags!":        [ "-fno-exceptions" ],
            "cflags_cc!":     [ "-fno-exceptions" ],
            'conditions': [
                ['OS=="mac"', {
                  'xcode_settings': {
                    'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
                    'OTHER_CPLUSPLUSFLAGS': ['-stdlib=libc++']
                  }
                }]
            ]
        },
        {
            "target_name":    "BundlesAddon",
            "sources":        [
                "BundlesAddon/Bundle.cpp"
            ],
            "include_dirs":   [
		"bundles/lib",
                "<!(node -e \"require('nan')\")"
                "<!(node -e \"require('v8')\")"
                "<!(node -e \"require('uv')\")"
            ],
            "dependencies":   [
                "BundlesLibrary"
            ],
            "cflags_cxx":     [ "-std=c++11 --with-gcc-toolchain" ],
            "cflags!":        [ "-fno-exceptions" ],
            "cflags_cc!":     [ "-fno-exceptions" ],
            'conditions': [
                ['OS=="mac"', {
                  'xcode_settings': {
                    'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
                    'OTHER_CPLUSPLUSFLAGS': ['-stdlib=libc++']
                  }
                }]
            ]
        }
    ]
}
