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
#ifndef NIRVANA_ORB_CORE_SERVANTMARSHALER_H_
#define NIRVANA_ORB_CORE_SERVANTMARSHALER_H_

#include <CORBA/Server.h>
#include "../CoreObject.h"
#include "LifeCyclePseudo.h"
#include "../SyncContext.h"
#include <CORBA/Marshal_s.h>
#include <CORBA/Unmarshal_s.h>

namespace CORBA {
namespace Nirvana {
namespace Core {

template <class T>
class ServantMarshalerImpl :
	public ::Nirvana::Core::CoreObject,
	public ::Nirvana::Core::LifeCyclePseudo <T>,
	public ServantTraits <T>,
	public InterfaceImplBase <T, Marshal>,
	public InterfaceImplBase <T, Unmarshal>
{
public:
	using InterfaceImplBase <T, Marshal>::__get_marshal_context;
	using InterfaceImplBase <T, Unmarshal>::__get_marshal_context;

protected:
	ServantMarshalerImpl (::Nirvana::Core::SyncContext& sc) :
		memory_ (sc.memory ())
	{}

protected:
	::Nirvana::Core::Heap& memory_;
	
	typedef ::Nirvana::UIntPtr Tag;
	Tag* cur_ptr_;
};

class ServantMarshaler :
	public ServantMarshalerImpl <ServantMarshaler>
{
public:
	static const size_t BLOCK_SIZE = 32 * sizeof (Tag);

	ServantMarshaler (::Nirvana::Core::SyncContext& sc) :
		ServantMarshalerImpl (sc)
	{
		cur_ptr_ = block_;
		*cur_ptr_ = RT_END;
	}

	~ServantMarshaler ()
	{
		Block* pb = clear_block (block_);
		while (pb) {
			Block* next = clear_block (*pb);
			memory_.release (pb, sizeof (Block));
			pb = next;
		}
	}

	Marshal::_ptr_type marshaler ()
	{
		return &static_cast <Marshal&> (static_cast <Bridge <Marshal>&> (*this));
	}

	Unmarshal::_ptr_type unmarshaler ()
	{
		cur_ptr_ = block_;
		return &static_cast <Unmarshal&> (static_cast <Bridge <Unmarshal>&> (*this));
	}

	static Unmarshal::_ptr_type unmarshaler (Marshal::_ptr_type marshaler)
	{
		ServantMarshaler* obj = static_cast <ServantMarshaler*> (&marshaler);
		if (obj)
			return obj->unmarshaler ();
		else
			return Unmarshal::_nil ();
	}

	MarshalContext marshal_context () const
	{
		if (shared_memory ())
			return MarshalContext::SHARED_MEMORY;
		else
			return MarshalContext::SHARED_PROTECTION_DOMAIN;
	}

	uintptr_t marshal_memory (const void* p, size_t& size, size_t release_size)
	{
		RecMemory* rec = (RecMemory*)add_record (RT_MEMORY, sizeof (RecMemory));
		rec->p = nullptr;
		if (release_size && shared_memory ()) {
			rec->p = const_cast <::Nirvana::Pointer> (p);
			rec->size = release_size;
			size = release_size;
		} else {
			uint8_t* pc = (uint8_t*)memory_.copy (nullptr, const_cast <void*> (p), size, 0);
			size_t au = memory_.query (pc, ::Nirvana::Memory::QueryParam::ALLOCATION_UNIT);
			size = ::Nirvana::round_up (pc + size, au) - pc;
			rec->p = pc;
			rec->size = size;
			if (release_size)
				::Nirvana::Core::SyncContext::current ().memory ().release (const_cast <void*> (p), release_size);
		}
		return (::Nirvana::UIntPtr)(rec->p);
	}

	::Nirvana::UIntPtr get_buffer (size_t& size, void*& buf_ptr)
	{
		RecMemory* rec = (RecMemory*)add_record (RT_MEMORY, sizeof (RecMemory));
		rec->p = nullptr;
		rec->p = memory_.allocate (nullptr, size, 0);
		rec->size = size;
		buf_ptr = rec->p;
		return (::Nirvana::UIntPtr)(rec->p);
	}

	void adopt_memory (const void* p, size_t size)
	{
		RecMemory* rec = (RecMemory*)get_record (RT_MEMORY);
		if (rec->p == p && rec->size == size) {
			rec->p = nullptr;
			move_next (sizeof (RecMemory));
		} else
			throw MARSHAL ();
	}

	::Nirvana::UIntPtr marshal_interface (Interface::_ptr_type obj)
	{
		RecInterface* rec = (RecInterface*)add_record (RT_INTERFACE, sizeof (RecInterface));
		return (::Nirvana::UIntPtr)(rec->p = interface_duplicate (&obj));
	}

	Interface::_ref_type unmarshal_interface (::Nirvana::ConstPointer marshal_data, const String& iid)
	{
		RecInterface* rec = (RecInterface*)get_record (RT_INTERFACE);
		Interface::_ptr_type itf = rec->p;
		if (marshal_data == &itf) {
			const Char* bridge_id = itf->_epv ().interface_id;
			if (!RepositoryId::compatible (bridge_id, iid)) {
				if (RepositoryId::compatible (bridge_id, Object::repository_id_)) {
					Object::_ptr_type obj (static_cast <Object*> (&itf));
					itf = AbstractBase_ptr (obj)->_query_interface (iid);
				}
			}

			if (itf) {
				move_next (sizeof (RecInterface));
				return std::move (reinterpret_cast <Interface::_ref_type&> (rec->p));
			}
		}
		throw MARSHAL ();
	}

private:
	enum RecordType
	{
		RT_END = 0,
		RT_MEMORY,
		RT_INTERFACE
	};

	struct RecMemory
	{
		void* p;
		size_t size;
	};

	struct RecInterface
	{
		Interface* p;
	};

	typedef Tag Block [BLOCK_SIZE / sizeof (Tag)];

	Block* clear_block (Tag* p);

	Tag* block_end (Tag* p)
	{
		return ::Nirvana::round_up (p, BLOCK_SIZE);
	}

	void* add_record (RecordType tag, size_t record_size);

	void* get_record (RecordType tag)
	{
		if (tag == *cur_ptr_)
			return cur_ptr_ + 1;
		else
			throw BAD_INV_ORDER ();
	}

	void move_next (size_t record_size);

	bool shared_memory () const
	{
		return &memory_ == &::Nirvana::Core::SyncContext::current ().memory ();
	}

	Tag block_ [(BLOCK_SIZE - sizeof (ServantMarshalerImpl <ServantMarshaler>)) / sizeof (Tag)];
};

static_assert (sizeof (ServantMarshaler) == ServantMarshaler::BLOCK_SIZE, "sizeof (ServantMarshaler)");

}
}
}

#endif
