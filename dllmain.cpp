// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

// Inclusion guard
#ifndef _DLLTUT_DLL_H_
#define _DLLTUT_DLL_H_

// Make our life easier, if DLL_EXPORT is defined in a file then DECLDIR will do an export
// If it is not defined DECLDIR will do an import
#if defined DLL_EXPORT
#define DECLDIR __declspec(dllexport)
#else
#define DECLDIR __declspec(dllimport)
#endif
#endif