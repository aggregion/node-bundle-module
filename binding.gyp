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
                "bundles/mbedtls-2.4.0/library/padlock.c"
            ],
            "cflags":         [ "-std=c++11"],
            "cflags!":        [ "-fno-exceptions" ],
            "cflags_cc!":     [ "-fno-exceptions" ],
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
            "cflags":         [ "-std=c++11" ],
            "cflags!":        [ "-fno-exceptions" ],
            "cflags_cc!":     [ "-fno-exceptions" ],

        }
    ]
}
