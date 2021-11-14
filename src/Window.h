#ifndef __VOLUME_RESTIR_WINDOW_H__
#define __VOLUME_RESTIR_WINDOW_H__

#ifdef _WIN32
#pragma comment(linker, "/subsystem:console")
#include <windows.h>
#elif defined(__linux__)
#include <xcb/xcb.h>
#endif

#endif /* __VOLUME_RESTIR_WINDOW_H__ */
