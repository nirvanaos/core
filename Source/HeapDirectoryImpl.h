/*
* Nirvana Core.
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
#ifndef NIRVANA_CORE_HEAPDIRECTORYIMPL_H_
#define NIRVANA_CORE_HEAPDIRECTORYIMPL_H_
#pragma once

namespace Nirvana {
namespace Core {

/// Heap directory bitmap implementation details
/// 
/// Heap bitmaps are often sparse (filled with zeroes).
/// Some virtual memory systems, such as Linux, use single zeroed physical page for all zero memory
/// blocks and then eventually do COW (copy on write) behavior on the page when we are changing the contents.
/// So the physical page is allocated only when we write to page.
/// For the such systems we may use HeapDirectoryImpl::COMMITTED_BITMAP and it won't produce the redundant
/// memory allocations.
/// But Windows does not use this behaviour. In Windows, the physical page allocated on the first access
/// the page, read or write. So, to conserve the physical memory in Windows we use HeapDirectoryImpl::RESERVED_BITMAP_WITH_EXCEPTIONS.
/// 
/// const HeapDirectoryImpl HEAP_DIRECTORY_IMPLEMENTATION must be defined in config.h
/// 
enum class HeapDirectoryImpl
{
	//! This implementation used for systems without memory protection.
	//! All bitmap memory must be initially committed and zero-filled.
	//! `HeapInfo` parameter is unused and must be `NULL`. `Memory` functions never called.
	PLAIN_MEMORY,

	//! All bitmap memory must be initially committed and zero-filled.
	//! If `HeapInfo` pointer is `NULL` the `Memory` functions will never be called as for PLAIN_MEMORY.
	//! This implementation provides the best performance but can't save physical pages for bitmap.
	COMMITTED_BITMAP,

	//! Bitmap memory must be initially reserved. HeapDirectory will commit it as needed.
	//! HeapDirectory uses `is_readable()` method to detect uncommitted pages.
	//! Extremely slow.
	RESERVED_BITMAP,

	//! Bitmap memory must be initially reserved. HeapDirectory will commit it as needed.
	//! HeapDirectory catches GP exception to detect uncommitted pages.
	//! This provides better performance than `RESERVED_BITMAP`, but maybe not for all platforms.
	RESERVED_BITMAP_WITH_EXCEPTIONS
};

}
}

#endif
