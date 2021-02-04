{
    "target_defaults": {
        "default_configuration": "Release",
        "configurations":        {
            "Debug":   {
                "defines":  [ "DEBUG" ],
                "defines!": [ "NDEBUG" ]
            },
            "Release": {
                'msvs_settings': {
                    'VCCLCompilerTool': {
                        'ExceptionHandling': 1,
                    }
                },
                "defines": [ "NDEBUG" ]
            }
        }
    }
}
