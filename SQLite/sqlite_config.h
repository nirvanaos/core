/*
* Nirvana SQLite driver.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#ifndef SQLITE_CONFIG_H_
#define SQLITE_CONFIG_H_
#pragma once

#define SQLITE_OS_OTHER 1
#define SQLITE_WITHOUT_MSIZE
#define SQLITE_DEFAULT_MEMSTATUS 0
#define SQLITE_LIKE_DOESNT_MATCH_BLOBS
#define SQLITE_OMIT_DEPRECATED
#define SQLITE_OMIT_PROGRESS_CALLBACK
#define SQLITE_OMIT_SHARED_CACHE
#define SQLITE_USE_ALLOCA
#define SQLITE_OMIT_AUTOINIT
#define SQLITE_STRICT_SUBTYPE 1
#define SQLITE_DEFAULT_MMAP_SIZE 0
#define SQLITE_ENABLE_COLUMN_METADATA
#define SQLITE_DEFAULT_FOREIGN_KEYS 1
#define SQLITE_USE_ALLOCA
#define SQLITE_USE_URI 1

#define HAVE_LOCALTIME_R (1)
#define HAVE_GMTIME_R (1)

#ifdef NDEBUG
#define SQLITE_THREADSAFE (0)
#define SQLITE_MUTEX_OMIT
#else
#define SQLITE_MUTEX_APPDEF (1)
#endif

#define SQLITE_OMIT_SEH

// Declare for CLang
#if defined (_MSC_VER) && defined (__clang__)
unsigned short _byteswap_ushort (unsigned short);
unsigned long _byteswap_ulong (unsigned long);
unsigned __int64 __cdecl _byteswap_uint64 (unsigned __int64);
#endif

#endif
