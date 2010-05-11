#ifndef BEATRIX_INCLUDE_BEATRIX_STDAFX_H
#define BEATRIX_INCLUDE_BEATRIX_STDAFX_H

#pragma once

//don't warn about unreferenced parameters
#pragma warning(disable : 4100)

//turn off hundreds of warning from ATL
#define _CRT_SECURE_NO_WARNINGS 1
//need this for xp-style common controls
#define ISOLATION_AWARE_ENABLED 1

#ifdef BEATRIX_EXPORTS
#define BEATRIX_API __declspec(dllexport)
#else
#define BEATRIX_API __declspec(dllimport)
#endif //toolbar api

namespace beatrix {
} //~namespace beatrix;

#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#define _WIN32_IE 0x0510
#define NTDDI_VERSION NTDDI_WIN2k

#include<somnia/stdafx.h>
#include<somnia/stdatl.h>
#include<somnia/stdwtl.h>
#include<vector>

#endif //BEATRIX_INCLUDE_BEATRIX_STDAFX_H
