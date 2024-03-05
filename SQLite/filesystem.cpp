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
#include "pch.h"
#include "filesystem.h"
#include "Global.h"
#include <Nirvana/File.h>
#include <Nirvana/System.h>
#include <Nirvana/c_heap_dbg.h>
#include <Nirvana/posix.h>
#include <time.h>
#include <unordered_map>

extern "C" int sqlite3_os_init ()
{
	return SQLITE_OK;
}

extern "C" int sqlite3_os_end ()
{
	return SQLITE_OK;
}

namespace SQLite {

extern const sqlite3_io_methods io_methods;

class File : public sqlite3_file
{
public:
	File (Nirvana::AccessDirect::_ref_type fa) noexcept :
		access_ (std::move (fa))
	{
		pMethods = &io_methods;
	}

	int close () noexcept
	{
		try {
			access_->close ();
			access_ = nullptr;
		} catch (...) {
		}
		return SQLITE_OK;
	}

	int read (void* p, int cb, sqlite3_int64 off) const noexcept
	{
		int ret = SQLITE_OK;
		try {
			NDBC::Blob data;
			access_->read (Nirvana::FileLock (), off, cb, Nirvana::LockType::LOCK_NONE, false, data);
			memcpy (p, data.data (), data.size ());
			if ((int)data.size () < cb) {
				// If xRead () returns SQLITE_IOERR_SHORT_READ it must also fill in the unread portions of the
				// buffer with zeros.A VFS that fails to zero - fill short reads might seem to work.
				// However, failure to zero - fill short reads will eventually lead to database corruption.
				// https://www.sqlite.org/c3ref/io_methods.html
				memset ((char*)p + data.size (), 0, (size_t)cb - data.size ());
				ret = SQLITE_IOERR_SHORT_READ;
			}
		} catch (...) {
			ret = SQLITE_IOERR_READ;
		}
		return ret;
	}

	int write (const void* p, int cb, sqlite3_int64 off) const noexcept
	{
		int ret = SQLITE_OK;
		try {
			access_->write (off, NDBC::Blob ((const CORBA::Octet*)p, (const CORBA::Octet*)p + cb),
				Nirvana::FileLock (), false);
		} catch (const CORBA::SystemException& ex) {
			if (ENOSPC == Nirvana::get_minor_errno (ex.minor ()))
				ret = SQLITE_FULL;
			else
				ret = SQLITE_IOERR_WRITE;
		} catch (...) {
			ret = SQLITE_IOERR_WRITE;
		}
		return ret;
	}

	int truncate (sqlite3_int64 size) const noexcept
	{
		int ret = SQLITE_OK;
		try {
			access_->size (size);
		} catch (...) {
			ret = SQLITE_IOERR_TRUNCATE;
		}
		return ret;
	}

	int sync () const noexcept
	{
		int ret = SQLITE_OK;
		try {
			access_->flush ();
		} catch (...) {
			ret = SQLITE_IOERR_FSYNC;
		}
		return ret;
	}

	int size (sqlite3_int64& cb) const noexcept
	{
		int ret = SQLITE_OK;
		try {
			cb = access_->size ();
		} catch (...) {
			ret = SQLITE_IOERR_FSTAT;
		}
		return ret;
	}

	int sector_size () const noexcept
	{
		int ret = 512;
		try {
			ret = (int)access_->block_size ();
		} catch (...) {
		}
		return ret;
	}

	int fetch (sqlite3_int64 off, int cb, void** pp) noexcept
	{
		int ret = SQLITE_OK;
		*pp = nullptr;
		assert (cb);
		assert (cache_.find (off) == cache_.end ());
		try {
			NDBC::Blob data;
			access_->read (Nirvana::FileLock (), off, cb, Nirvana::LockType::LOCK_NONE, false, data);
			if ((int)data.size () == cb)
				*pp = cache_.emplace (off, std::move (data)).first->second.data ();
		} catch (const CORBA::NO_MEMORY&) {
			ret = SQLITE_IOERR_NOMEM;
		} catch (...) {
			ret = SQLITE_IOERR_READ;
		}
		return ret;
	}

	int unfetch (sqlite3_int64 off, void* p) noexcept
	{
		NIRVANA_VERIFY (cache_.erase (off));
		return SQLITE_OK;
	}

private:
	Nirvana::AccessDirect::_ref_type access_;

	typedef std::unordered_map <sqlite3_int64, NDBC::Blob> Cache;
	Cache cache_;
};

extern "C" int xClose (sqlite3_file * f) noexcept
{
	return static_cast <File&> (*f).close ();
}

extern "C" int xRead (sqlite3_file* f, void* p, int iAmt, sqlite3_int64 off)
{
	return static_cast <File&> (*f).read (p, iAmt, off);
}

extern "C" int xWrite (sqlite3_file* f, const void* p, int iAmt, sqlite3_int64 off)
{
	return static_cast <File&> (*f).write (p, iAmt, off);
}

extern "C" int xTruncate (sqlite3_file* f, sqlite3_int64 size)
{
	return static_cast <File&> (*f).truncate (size);
}

extern "C" int xSync (sqlite3_file* f, int)
{
	return static_cast <File&> (*f).sync ();
}

extern "C" int xFileSize (sqlite3_file* f, sqlite3_int64* pSize)
{
	return static_cast <File&> (*f).size (*pSize);
}

extern "C" int xLock (sqlite3_file* f, int)
{
	return SQLITE_OK;
}

extern "C" int xUnlock (sqlite3_file* f, int)
{
	return SQLITE_OK;
}

extern "C" int xCheckReservedLock (sqlite3_file* f, int* pResOut)
{
	*pResOut = 0;
	return SQLITE_OK;
}

extern "C" int xSectorSize (sqlite3_file* f)
{
	return static_cast <File&> (*f).sector_size ();
}

int xDeviceCharacteristics (sqlite3_file*)
{
	return
		SQLITE_IOCAP_UNDELETABLE_WHEN_OPEN |
//?		SQLITE_IOCAP_POWERSAFE_OVERWRITE |
		SQLITE_IOCAP_ATOMIC;
}

extern "C" int xFetch (sqlite3_file* f, sqlite3_int64 iOfst, int iAmt, void** pp)
{
	return static_cast <File&> (*f).fetch (iOfst, iAmt, pp);
}

extern "C" int xUnfetch (sqlite3_file* f, sqlite3_int64 iOfst, void* p)
{
	return static_cast <File&> (*f).unfetch (iOfst, p);
}

extern "C" int xFileControl (sqlite3_file * f, int op, void* pArg)
{
	return SQLITE_NOTFOUND;
}

const sqlite3_io_methods io_methods = {
	3,
	xClose,
	xRead,
	xWrite,
	xTruncate,
	xSync,
	xFileSize,
	xLock,
	xUnlock,
	xCheckReservedLock,
	xFileControl,
	xSectorSize,
	xDeviceCharacteristics,
	nullptr, // xShmMap
	nullptr, // xShmLock
	nullptr, // xShmBarrier
	nullptr, // xShmUnmap
	xFetch,
	xUnfetch
};

int xOpen (sqlite3_vfs*, sqlite3_filename zName, sqlite3_file* file,
	int flags, int* pOutFlags) noexcept
{
	Nirvana::Access::_ref_type fa;
	try {
		uint_fast16_t open_flags = O_DIRECT;
		if (flags & SQLITE_OPEN_READWRITE)
			open_flags |= O_RDWR;
		if (flags & SQLITE_OPEN_CREATE)
			open_flags |= O_CREAT;
		if (flags & SQLITE_OPEN_EXCLUSIVE)
			open_flags |= O_EXCL;

		fa = global.open_file (zName, open_flags);

	} catch (CORBA::NO_MEMORY ()) {
		return SQLITE_NOMEM;
	} catch (...) {
		return SQLITE_CANTOPEN;
	}
	new (file) File (Nirvana::AccessDirect::_narrow (fa->_to_object ()));
	return SQLITE_OK;
}

extern "C" int xDelete (sqlite3_vfs*, sqlite3_filename zName, int syncDir)
{
	try {
		// Get full path name
		CosNaming::Name name;
		Nirvana::system->append_path (name, zName, true);
		// Remove root name
		name.erase (name.begin ());
		// Delete
		global.file_system ()->unbind (name);
	} catch (...) {
		return SQLITE_ERROR;
	}
	return SQLITE_OK;
}

extern "C" int xAccess (sqlite3_vfs*, const char* zName, int flags, int* pResOut)
{
	*pResOut = 0;
	try {
		// Get full path name
		CosNaming::Name name;
		Nirvana::system->append_path (name, zName, true);
		// Remove root name
		name.erase (name.begin ());
		// Resolve name
		Nirvana::DirItem::_ref_type item;
		try {
			item = Nirvana::DirItem::_narrow (global.file_system ()->resolve (name));
		} catch (CosNaming::NamingContext::NotFound&) {
		}
		if (item) {
			unsigned mask = 0;
			switch (flags) {
			case SQLITE_ACCESS_EXISTS:
				mask = F_OK;
				break;
			case SQLITE_ACCESS_READWRITE:
				mask = R_OK | W_OK;
				break;
			case SQLITE_ACCESS_READ:
				mask = R_OK;
				break;
			}
			*pResOut = (item->access () & mask) == mask;
		}
	} catch (...) {
		return SQLITE_ERROR;
	}
	return SQLITE_OK;
}

extern "C" int xFullPathname (sqlite3_vfs*, const char* zName, int nOut, char* zOut)
{
	try {
		// Get full path name
		CosNaming::Name name;
		Nirvana::system->append_path (name, zName, true);
		auto s = Nirvana::system->to_string (name);
		if ((int)s.size () >= nOut)
			return SQLITE_CANTOPEN;
		const char* n = s.c_str ();
		std::copy (n, n + s.size () + 1, zOut);
	} catch (...) {
		return SQLITE_CANTOPEN;
	}
	return SQLITE_OK;
}

extern "C" int xSleep (sqlite3_vfs*, int microseconds)
{
	Nirvana::system->sleep ((TimeBase::TimeT)microseconds * TimeBase::MICROSECOND);
	return SQLITE_OK;
}

extern "C" int xCurrentTimeInt64 (sqlite3_vfs*, sqlite3_int64* time)
{
	TimeBase::UtcT t = Nirvana::system->system_clock ();
	*time = (t.time () + t.tdf () * 600000000LL) / TimeBase::MILLISECOND + TimeBase::JULIAN_MS;
	return SQLITE_OK;
}

extern "C" int xRandomness (sqlite3_vfs*, int nByte, char* zOut)
{
#if (RAND_MAX >= 0xFFFFFFFF)
	for (; nByte >= 4; zOut += 4, nByte -= 4) {
		*(uint32_t*)zOut = (uint32_t)Nirvana::system->rand ();
	}
#endif

#if (RAND_MAX >= 0xFFFF)
	for (; nByte >= 2; zOut += 2, nByte -= 2) {
		*(uint16_t*)zOut = (uint16_t)Nirvana::system->rand ();
	}
#endif

	for (; nByte > 0; ++zOut, --nByte) {
		*zOut = (char)Nirvana::system->rand ();
	}
	return nByte;
}

const size_t HEAP_BLOCK_OVERHEAD = sizeof (Nirvana::HeapBlockHdrType) - Nirvana::HeapBlockHdrType::TRAILER_SIZE;

struct sqlite3_vfs vfs = {
	3,                   /* Structure version number (currently 3) */
	(int)sizeof (File),  /* Size of subclassed sqlite3_file */
  512 - HEAP_BLOCK_OVERHEAD, /* Maximum file pathname length */
	nullptr,             /* Next registered VFS */
	VFS_NAME,            /* Name of this virtual file system */
	nullptr,             /* Pointer to application-specific data */
	xOpen,
	xDelete,
	xAccess,
	xFullPathname,
	nullptr, // xDlOpen
	nullptr, // xDlError
	nullptr, // xDlSym
	nullptr, // xDlClose
	xRandomness,
	xSleep,
	nullptr, // xCurrentTime
	nullptr, // xGetLastError
	xCurrentTimeInt64
};

}
