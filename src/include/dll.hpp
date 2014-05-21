#ifndef DLL_HPP
#define DLL_HPP

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the LIBODB_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// LIBODB_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WIN32
#ifdef LIBODB_EXPORTS
#define LIBODB_API __declspec(dllexport)
#else
#define LIBODB_API __declspec(dllimport)
#endif
#else
#define LIBODB_API
#endif

#endif
