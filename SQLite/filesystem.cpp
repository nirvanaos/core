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
#include <Nirvana/File.h>
#include <Nirvana/System.h>
#include "Global.h"
#include "sqlite/sqlite3.h"
#include <fnctl.h>
#include <unordered_map>
#include <time.h>
#include <Nirvana/md5.h>

extern "C" int sqlite3_os_init ()
{
	return SQLITE_OK;
}

extern "C" int sqlite3_os_end ()
{
	return SQLITE_OK;
}

using namespace Nirvana;
using namespace CORBA;

namespace SQLite {

const char LOG_DIR [] = "/var/tmp";

const char* is_id (const char* file) noexcept
{
	const char* prefix_end = file + std::size (OBJID_PREFIX) - 1;
	if (std::equal (file, prefix_end, OBJID_PREFIX))
		return prefix_end;
	return nullptr;
}

inline
Nirvana::DirItemId string_to_id (const char* begin, const char* end)
{
	return base64::decode_into <DirItemId> (begin, end);
}

inline char to_hex (unsigned d) noexcept
{
	return d < 10 ? '0' + d : 'A' + d - 10;
}

std::string get_journal_path (const char* id_begin, const char* end)
{
	std::string name;
	const char* id_end = end;
	for (;;) {
		if (id_begin >= --id_end)
			break;
		char c = *id_end;
		if ('-' == c || '=' == c)
			break;
	}
	if ('-' == *id_end) {

		// Generate journal file name

		Nirvana::DirItemId id = string_to_id (id_begin, id_end);
		MD5_CTX md5ctx;
		MD5Init (&md5ctx);
		MD5Update (&md5ctx, id.data (), (unsigned int)id.size ());
		unsigned char md5 [16];
		MD5Final (md5, &md5ctx);

		name.reserve (sizeof (LOG_DIR) - 1 + 32 + (end - id_end));
		name = "/var/tmp/";
		for (unsigned d : md5) {
			name.push_back (to_hex (d >> 4));
			name.push_back (to_hex (d & 0xf));
		}
		name.append (id_end, end);
	}
	return name;
}

extern const sqlite3_io_methods io_methods;

class File : public sqlite3_file
{
public:
	File (AccessDirect::_ref_type fa) noexcept :
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
			access_->read (FileLock (), off, cb, LockType::LOCK_NONE, false, data);
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
			access_->write (off, NDBC::Blob ((const Octet*)p, (const Octet*)p + cb), FileLock (), false);
		} catch (const SystemException& ex) {
			if (ENOSPC == get_minor_errno (ex.minor ()))
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
		try {
			NDBC::Blob data;
			access_->read (FileLock (), off, cb, LockType::LOCK_NONE, false, data);
			if ((int)data.size () < cb)
				ret = SQLITE_IOERR_SHORT_READ;
			*pp = cache_.emplace (data.data (), std::move (data)).first->first;
		} catch (const NO_MEMORY&) {
			ret = SQLITE_IOERR_NOMEM;
		} catch (...) {
			ret = SQLITE_IOERR_READ;
		}
		return ret;
	}

	int unfetch (sqlite3_int64 off, void* p) noexcept
	{
		verify (cache_.erase (p));
		return SQLITE_OK;
	}

private:
	Nirvana::AccessDirect::_ref_type access_;

	typedef std::unordered_map <void*, NDBC::Blob> Cache;
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
		SQLITE_IOCAP_POWERSAFE_OVERWRITE |
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
	Access::_ref_type fa;
	try {
		uint_fast16_t open_flags = O_DIRECT;
		if (flags & SQLITE_OPEN_READWRITE)
			open_flags |= O_RDWR;
		if (flags & SQLITE_OPEN_CREATE)
			open_flags |= O_CREAT;
		if (flags & SQLITE_OPEN_EXCLUSIVE)
			open_flags |= O_EXCL;

		const char* id_begin = is_id (zName);
		if (id_begin) {
			const char* end = id_begin + strlen (id_begin);

			std::string journal = get_journal_path (id_begin, end);
			if (!journal.empty ())
				fa = global.open_file (journal, open_flags);
			else {
				;
				Nirvana::File::_ref_type file = Nirvana::File::_narrow (
					global.file_system ()->get_item (string_to_id (id_begin, end)));
				if (!file)
					return SQLITE_CANTOPEN;
				fa = file->open (open_flags & ~(O_CREAT | O_EXCL), 0);
			}
		} else
			fa = global.open_file (zName, open_flags);

	} catch (CORBA::NO_MEMORY ()) {
		return SQLITE_NOMEM;
	} catch (...) {
		return SQLITE_CANTOPEN;
	}
	new (file) File (AccessDirect::_narrow (fa->_to_object ()));
	return SQLITE_OK;
}

extern "C" int xDelete (sqlite3_vfs*, sqlite3_filename zName, int syncDir)
{
	try {
		std::string path;
		const char* id_begin = is_id (zName);
		if (id_begin) {
			const char* end = id_begin + strlen (id_begin);

			path = get_journal_path (id_begin, end);
			assert (!path.empty ());
			if (path.empty ())
				return SQLITE_ERROR;

		} else
			path = zName;

		// Get full path name
		CosNaming::Name name;
		g_system->append_path (name, path, true);
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
		const char* id_begin = is_id (zName);
		DirItem::_ref_type item;
		std::string path;
		if (id_begin) {
			const char* end = id_begin + strlen (id_begin);
			path = get_journal_path (id_begin, end);
			if (path.empty ())
				item = global.file_system ()->get_item (string_to_id (id_begin, end));
		} else
			path = zName;

		if (!path.empty ()) {
			// Get full path name
			CosNaming::Name name;
			g_system->append_path (name, path, true);
			// Remove root name
			name.erase (name.begin ());
			// Resolve name
			try {
				item = DirItem::_narrow (global.file_system ()->resolve (name));
			} catch (CosNaming::NamingContext::NotFound&) {
			}
		}
		// TODO: Improve implementation
		if (item)
			*pResOut = 1;
	} catch (...) {
		return SQLITE_ERROR;
	}
	return SQLITE_OK;
}

extern "C" int xFullPathname (sqlite3_vfs*, const char* zName, int nOut, char* zOut)
{
	if (is_id (zName)) {
		if (strcpy_s (zOut, nOut, zName))
			return SQLITE_CANTOPEN;
	} else {
		try {
			// Get full path name
			CosNaming::Name name;
			g_system->append_path (name, zName, true);
			auto s = g_system->to_string (name);
			if ((int)s.size () >= nOut)
				return SQLITE_CANTOPEN;
			const char* n = s.c_str ();
			std::copy (n, n + s.size () + 1, zOut);
		} catch (...) {
			return SQLITE_CANTOPEN;
		}
	}
	return SQLITE_OK;
}

extern "C" int xSleep (sqlite3_vfs*, int microseconds)
{
	g_system->sleep ((TimeBase::TimeT)microseconds * TimeBase::MICROSECOND);
	return SQLITE_OK;
}

extern "C" int xCurrentTimeInt64 (sqlite3_vfs*, sqlite3_int64* time)
{
	TimeBase::UtcT t = g_system->system_clock ();
	*time = (t.time () + t.tdf () * 600000000LL) / TimeBase::MILLISECOND + TimeBase::JULIAN_MS;
	return SQLITE_OK;
}

extern "C" int xRandomness (sqlite3_vfs*, int nByte, char* zOut)
{
	for (int cnt = nByte; cnt > 0; --cnt) {
		(*zOut++) = (char)Nirvana::g_system->rand ();
	}
	return nByte;
}

struct sqlite3_vfs vfs = {
	3,                   /* Structure version number (currently 3) */
	(int)sizeof (File),  /* Size of subclassed sqlite3_file */
  1024,                /* Maximum file pathname length */
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
