#pragma once

// This code produces too many warnings

#if defined _WIN32 || defined __CYGWIN__
  # ifdef BUNDLES_ADDON_LIBRARY
    #  ifdef __GNUC__
      #   define DLL_PUBLIC_BUNDLES_LIB __attribute__((dllexport))
    #  else // ifdef __GNUC__
      #   define DLL_PUBLIC_BUNDLES_LIB __declspec(dllexport) // Note: actually gcc seems to also
                                                              // supports this syntax.
    #  endif // ifdef __GNUC__
  # else // ifdef BUNDLES_ADDON_LIBRARY
    #  ifdef __GNUC__
      #   define DLL_PUBLIC_BUNDLES_LIB __attribute__((dllimport))
    #  else // ifdef __GNUC__
      #   define DLL_PUBLIC_BUNDLES_LIB __declspec(dllimport) // Note: actually gcc seems to also
                                                              // supports this syntax.
    #  endif // ifdef __GNUC__
  # endif // ifdef BUNDLES_ADDON_LIBRARY
  # define DLL_LOCAL
#else // if defined _WIN32 || defined __CYGWIN__
  # if __GNUC__ >= 4
    #  define DLL_PUBLIC_BUNDLES_LIB __attribute__((visibility("default")))
    #  define DLL_LOCAL_BUNDLES_LIB  __attribute__((visibility("hidden")))
  # else // if __GNUC__ >= 4
    #  define DLL_PUBLIC_BUNDLES_LIB
    #  define DLL_LOCAL_BUNDLES_LIB
  # endif // if __GNUC__ >= 4
#endif // if defined _WIN32 || defined __CYGWIN__
