/*
* This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef CMANGOS_GLOBALDEF_H
#define CMANGOS_GLOBALDEF_H

#include "revision.h"
#include "Platform/Define.h"

#define _PACKAGENAME "CMaNGOS"

// Define the default name of config file and the minimal version
#define CONFIG_DEFAULT_FILENAME "AuthServer.conf"
// Format is YYYYMMDDRR where RR is the change in the ".conf" file for the day
#define AUTH_CONFIG_MINIMAL_REVISION 2018040101
#define REVISION_DB_AUTHSERVER "required_00001_01_authserver"
#define REQUIRED_SQL_SUFFIX "required_"

#define DEFAULT_PLAYER_LIMIT 100
#define DEFAULT_WORLDSERVER_PORT 8085                       //8129
#define DEFAULT_REALMSERVER_PORT 3724

#if MANGOS_ENDIAN == MANGOS_BIGENDIAN
# define _ENDIAN_STRING "big-endian"
#else
# define _ENDIAN_STRING "little-endian"
#endif

#if defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86)
# define ARCHITECTURE "x32"
#elif defined(__amd64) || defined(__amd64__) || defined(__x86_64) || defined(_M_X64)
# define ARCHITECTURE "x64"
#elif defined(__ia64)  || defined(__IA64__)  || defined(_M_IA64)
# define ARCHITECTURE "IA64"
#else
# define ARCHITECTURE "x32"
#endif

#if PLATFORM == PLATFORM_WINDOWS
# ifdef _WIN64
#  define _ENDIAN_PLATFORM "Win64 (" _ENDIAN_STRING ")"
# else
#  define _ENDIAN_PLATFORM "Win32 (" _ENDIAN_STRING ")"
# endif
#else
# if defined  (__FreeBSD__)
#  define _ENDIAN_PLATFORM "FreeBSD_" ARCHITECTURE " (" _ENDIAN_STRING ")"
# elif defined(__NetBSD__)
#  define _ENDIAN_PLATFORM "NetBSD_" ARCHITECTURE " (" _ENDIAN_STRING ")"
# elif defined(__OpenBSD__)
#  define _ENDIAN_PLATFORM "OpenBSD_" ARCHITECTURE " (" _ENDIAN_STRING ")"
# elif defined(__DragonFly__)
#  define _ENDIAN_PLATFORM "DragonFlyBSD_" ARCHITECTURE " (" _ENDIAN_STRING ")"
# elif defined(__APPLE__)
#  define _ENDIAN_PLATFORM "MacOSX_" ARCHITECTURE " (" _ENDIAN_STRING ")"
# elif defined(__linux) || defined(__linux__)
#  define _ENDIAN_PLATFORM "Linux_" ARCHITECTURE " (" _ENDIAN_STRING ")"
# else
#  define _ENDIAN_PLATFORM "Unix_" ARCHITECTURE " (" _ENDIAN_STRING ")"
# endif
#endif

#endif
