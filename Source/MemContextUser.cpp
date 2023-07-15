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
#include "MemContextUser.h"
#include "NameService/FileSystem.h"
#include "ORB/Services.h"
#include <Nirvana/Legacy/Legacy_Process.h>
#include <fnctl.h>
#include "virtual_copy.h"

using namespace CosNaming;
using CORBA::Core::Services;

namespace Nirvana {
namespace Core {

MemContextUser::FileDescriptor::FileDescriptor (CORBA::AbstractBase::_ptr_type access)
{
	CORBA::ValueBase::_ref_type vb = access->_to_value ();
	if (vb) {
		AccessBuf::_ptr_type ab = AccessBuf::_downcast (vb);
		if (ab) {
			access_ = ab;
			access_type_ = AccessType::ACCESS_BUF;
		} else
			throw_UNKNOWN (make_minor_errno (EIO));
	} else {
		AccessChar::_ref_type ac = AccessChar::_narrow (access->_to_object ());
		if (ac) {
			access_ = std::move (ac);
			access_type_ = AccessType::ACCESS_CHAR;
		} else
			throw_UNKNOWN (make_minor_errno (EIO));
	}
}

inline
void MemContextUser::FileDescriptor::close ()
{
	switch (access_type_) {
	case AccessType::ACCESS_BUF:
		access <AccessBuf> ()->close ();
		break;
	case AccessType::ACCESS_CHAR:
		access <AccessChar> ()->close ();
		break;
	default:
		throw_BAD_PARAM (make_minor_errno (EBADF));
	}
	access_ = nullptr;
	access_type_ = AccessType::EMPTY;
}

inline
size_t MemContextUser::FileDescriptor::read (void* p, size_t size) const
{
	switch (access_type_) {
	case AccessType::ACCESS_BUF:
		return access <AccessBuf> ()->read (p, size);
		break;

	case AccessType::ACCESS_CHAR: {
		if (sizeof (size_t) > 4 && size > std::numeric_limits <uint32_t>::max ())
			throw_IMP_LIMIT (make_minor_errno (ENXIO));
		IDL::String buf;
		access <AccessChar> ()->read ((uint32_t)size, buf);
		size_t readed = buf.size ();
		if (readed > size)
			throw_UNKNOWN (make_minor_errno (EIO));
		Port::Memory::copy (p, const_cast <char*> (buf.data ()), readed, Memory::SRC_DECOMMIT);
		return readed;
	} break;

	default:
		throw_BAD_PARAM (make_minor_errno (EBADF));
	}
}

inline
void MemContextUser::FileDescriptor::write (const void* p, size_t size) const
{
	switch (access_type_) {
	case AccessType::ACCESS_BUF:
		access <AccessBuf> ()->write (p, size);
		break;

	case AccessType::ACCESS_CHAR:
		access <AccessChar> ()->write (CORBA::Internal::StringView <char> ((const char*)p, size));
		break;

	default:
		throw_BAD_PARAM (make_minor_errno (EBADF));
	}
}

inline
uint64_t MemContextUser::FileDescriptor::seek (unsigned method, const int64_t& off) const
{
	switch (access_type_) {
	case AccessType::ACCESS_BUF:
		return access <AccessBuf> ()->seek ((SeekMethod)method, off);
		
	case AccessType::ACCESS_CHAR:
		throw_BAD_OPERATION (make_minor_errno (ESPIPE));

	default:
		throw_BAD_PARAM (make_minor_errno (EBADF));
	}
}

MemContextUser::Data* MemContextUser::Data::create ()
{
	return new Data;
}

MemContextUser::Data::~Data ()
{}

inline
MemContextUser::Data::Data (const InheritedFiles& inh)
{
	size_t max = 0;
	for (const auto& f : inh) {
		if (max < f.ifd ())
			max = f.ifd ();
	}
	if (max >= StandardFileDescriptor::STD_CNT)
		file_descriptors_.resize (max + 1 - StandardFileDescriptor::STD_CNT);
	for (const auto& f : inh) {
		FileDescriptor& fd = get_fd (f.ifd ());
		assert (fd.empty ());
		if (fd.empty ())
			fd = FileDescriptor (f.access ());
	}
}

Name MemContextUser::Data::default_dir ()
{
	Name home (2);
	home.back ().id ("home");
	return home;
}

Name MemContextUser::Data::get_name_from_path (const IDL::String& path) const
{
	IDL::String translated;
	const IDL::String* ppath;
	if (Port::FileSystem::translate_path (path, translated))
		ppath = &translated;
	else
		ppath = &path;

	Name name;
	if (!FileSystem::is_absolute (*ppath))
		name = current_dir_.empty () ? default_dir () : current_dir_;
	FileSystem::get_name_from_path (name, *ppath);
	return name;
}

NamingContext::_ref_type MemContextUser::Data::name_service ()
{
	return NamingContext::_narrow (Services::bind (Services::NameService));
}

inline void MemContextUser::Data::chdir (const IDL::String& path)
{
	if (path.empty ()) {
		current_dir_.clear ();
		return;
	}

	Name new_dir = get_name_from_path (path);

	// Check that new directory exists
	Dir::_ref_type dir = Dir::_narrow (name_service ()->resolve (new_dir));
	if (!dir)
		throw_INTERNAL (make_minor_errno (ENOTDIR));

	if (dir->_non_existent ())
		throw_OBJECT_NOT_EXIST (make_minor_errno (ENOENT));

	current_dir_ = std::move (new_dir);
}

inline
unsigned MemContextUser::Data::fd_open (const IDL::String& path, uint_fast16_t flags, mode_t mode)
{
	// Open file access
	Name name = get_name_from_path (path);
	name.erase (name.begin ());
	Dir::_ref_type root = Dir::_narrow (name_service ()->resolve (Name ()));
	Access::_ref_type access = root->open (name, flags & ~O_DIRECT, mode);

	// Allocate file descriptor cell
	unsigned ifd;
	{
		auto it = file_descriptors_.begin ();
		for (; it != file_descriptors_.end (); ++it) {
			if (it->empty ())
				break;
		}
		size_t i = it - file_descriptors_.begin ();
		if (i > (unsigned)std::numeric_limits <uint16_t>::max () - (unsigned)STD_CNT)
			throw_IMP_LIMIT (make_minor_errno (EMFILE));
		if (it == file_descriptors_.end ())
			file_descriptors_.emplace_back ();
		ifd = (unsigned)(i + STD_CNT);
		file_descriptors_ [i] = FileDescriptor (access);
	}

	return ifd;
}

inline
void MemContextUser::Data::fd_close (unsigned ifd)
{
	FileDescriptor& fd = get_fd (ifd);
	fd.close ();
	while (!file_descriptors_.empty () && file_descriptors_.back ().empty ()) {
		file_descriptors_.pop_back ();
	}
}

MemContextUser::FileDescriptor& MemContextUser::Data::get_fd (unsigned ifd)
{
	if (ifd < StandardFileDescriptor::STD_CNT)
		return std_file_descriptors_ [ifd];
	ifd -= StandardFileDescriptor::STD_CNT;
	if (ifd >= file_descriptors_.size ())
		throw_BAD_PARAM (make_minor_errno (EBADF));
	return file_descriptors_ [ifd];
}

inline
size_t MemContextUser::Data::fd_read (unsigned ifd, void* p, size_t size)
{
	return get_fd (ifd).read (p, size);
}

inline
void MemContextUser::Data::fd_write (unsigned ifd, const void* p, size_t size)
{
	get_fd (ifd).write (p, size);
}

inline
uint64_t MemContextUser::Data::fd_seek (unsigned ifd, const int64_t& off, unsigned method)
{
	return get_fd (ifd).seek (method, off);
}

MemContextUser& MemContextUser::current ()
{
	MemContextUser* p = MemContext::current ().user_context ();
	if (!p)
		throw_BAD_OPERATION (); // Unavailable for core contexts
	return *p;
}

MemContextUser::MemContextUser () :
	MemContext (true)
{}

MemContextUser::MemContextUser (Heap& heap, const InheritedFiles& inh) :
	MemContext (heap, true)
{
	if (!inh.empty ())
		data_.reset (new Data (inh));
}

MemContextUser::~MemContextUser ()
{}

inline
MemContextUser::Data& MemContextUser::data ()
{
	if (!data_)
		data_.reset (Data::create ());
	return *data_;
}

RuntimeProxy::_ref_type MemContextUser::runtime_proxy_get (const void* obj)
{
	return data ().runtime_proxy_get (obj);
}

void MemContextUser::runtime_proxy_remove (const void* obj) noexcept
{
	if (data_)
		data_->runtime_proxy_remove (obj);
}

void MemContextUser::on_object_construct (MemContextObject& obj) noexcept
{
	object_list_.push_back (obj);
}

void MemContextUser::on_object_destruct (MemContextObject& obj) noexcept
{
	// The object itself will remove from list. Nothing to do.
}

CosNaming::Name MemContextUser::get_current_dir_name () const
{
	CosNaming::Name cur_dir;
	if (data_)
		cur_dir = data_->current_dir ();
	if (cur_dir.empty ())
		cur_dir = Data::default_dir ();
	return cur_dir;
}

void MemContextUser::chdir (const IDL::String& path)
{
	data ().chdir (path);
}

unsigned MemContextUser::fd_open (const IDL::String& path, uint_fast16_t flags, mode_t mode)
{
	return data ().fd_open (path, flags, mode);
}

void MemContextUser::fd_close (unsigned fd)
{
	if (!data_)
		throw_BAD_PARAM (make_minor_errno (EBADF));
	data_->fd_close (fd);
}

size_t MemContextUser::fd_read (unsigned fd, void* p, size_t size)
{
	return data ().fd_read (fd, p, size);
}

void MemContextUser::fd_write (unsigned fd, const void* p, size_t size)
{
	data ().fd_write (fd, p, size);
}

uint64_t MemContextUser::fd_seek (unsigned fd, const int64_t& off, unsigned method)
{
	return data ().fd_seek (fd, off, method);
}

}
}
