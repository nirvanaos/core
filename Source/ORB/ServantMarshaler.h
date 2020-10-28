#ifndef NIRVANA_ORB_CORE_SERVANTMARSHALER_H_
#define NIRVANA_ORB_CORE_SERVANTMARSHALER_H_

#include "../UserObject.h"
#include "LifeCyclePseudo.h"
#include "../SynchronizationContext.h"
#include <CORBA/Proxy/Marshal_s.h>
#include <CORBA/Proxy/Unmarshal_s.h>

namespace CORBA {
namespace Nirvana {
namespace Core {

template <class T>
class ServantMarshalerImpl :
	public ::Nirvana::Core::UserObject,
	public LifeCyclePseudo <T>,
	public ServantTraits <T>,
	public InterfaceImplBase <T, Marshal>,
	public InterfaceImplBase <T, Unmarshal>
{
public:
	using InterfaceImplBase <T, Marshal>::_context;
	using InterfaceImplBase <T, Unmarshal>::_context;
};

class ServantMarshaler :
	public ServantMarshalerImpl <ServantMarshaler>
{
	typedef ::Nirvana::UIntPtr Tag;
public:
	static const size_t BLOCK_SIZE = 32 * sizeof (Tag);

	ServantMarshaler (::Nirvana::Core::SynchronizationContext* sc) :
		sync_context_ (sc),
		cur_ptr_ (block_)
	{
		*cur_ptr_ = RT_END;
	}

	~ServantMarshaler ()
	{
		Block* pb = clear_block (block_);
		while (pb) {
			Block* next = clear_block (*pb);
			sync_context_->memory ().release (pb, sizeof (Block));
			pb = next;
		}
	}

	Marshal_ptr marshaler ()
	{
		return &static_cast <Marshal&> (static_cast <Bridge <Marshal>&> (*this));
	}

	Unmarshal_ptr unmarshaler ()
	{
		cur_ptr_ = block_;
		return &static_cast <Unmarshal&> (static_cast <Bridge <Unmarshal>&> (*this));
	}

	static Unmarshal_ptr unmarshaler (Marshal_ptr marshaler)
	{
		ServantMarshaler* obj = static_cast <ServantMarshaler*> (&marshaler);
		if (obj)
			return obj->unmarshaler ();
		else
			return Unmarshal::_nil ();
	}

	MarshalContext context () const
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
			::Nirvana::Core::Heap& mem = sync_context_->memory ();
			uint8_t* pc = (uint8_t*)mem.copy (nullptr, const_cast <void*> (p), size, 0);
			size_t au = mem.query (pc, ::Nirvana::MemQuery::ALLOCATION_UNIT);
			size = ::Nirvana::round_up (pc + size, au) - pc;
			rec->p = pc;
			rec->size = size;
			if (release_size)
				::Nirvana::g_memory->release (const_cast <void*> (p), release_size);
		}
		return (::Nirvana::UIntPtr)(rec->p);
	}

	::Nirvana::UIntPtr get_buffer (size_t& size, void*& buf_ptr)
	{
		RecMemory* rec = (RecMemory*)add_record (RT_MEMORY, sizeof (RecMemory));
		rec->p = nullptr;
		rec->p = sync_context_->memory ().allocate (nullptr, size, 0);
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

	::Nirvana::UIntPtr marshal_object (Object_ptr obj)
	{
		RecObject* rec = (RecObject*)add_record (RT_OBJECT, sizeof (RecObject));
		rec->p = Object::_duplicate (obj);
		return (::Nirvana::UIntPtr)&obj;
	}

	NIRVANA_NODISCARD Interface* unmarshal_interface (::Nirvana::ConstPointer marshal_data, const String& iid)
	{
		RecObject* rec = (RecObject*)get_record (RT_OBJECT);
		if (marshal_data == &rec->p) {
			Interface* itf = AbstractBase_ptr (rec->p)->_query_interface (iid);
			if (itf) {
				rec->p = Object::_nil ();
				move_next (sizeof (RecObject));
				return itf;
			}
		}
		throw MARSHAL ();
	}

	::Nirvana::UIntPtr marshal_type_code (TypeCode_ptr tc)
	{
		RecTypeCode* rec = (RecTypeCode*)add_record (RT_TYPE_CODE, sizeof (RecTypeCode));
		rec->p = TypeCode::_duplicate (tc);
		return (::Nirvana::UIntPtr)&tc;
	}

	TypeCode_var unmarshal_type_code (::Nirvana::ConstPointer marshal_data)
	{
		RecTypeCode* rec = (RecTypeCode*)get_record (RT_TYPE_CODE);
		if (marshal_data == &rec->p) {
			TypeCode_ptr tc = rec->p;
			rec->p = TypeCode::_nil ();
			move_next (sizeof (RecTypeCode));
			return tc;
		}
		throw MARSHAL ();
	}

private:
	enum RecordType
	{
		RT_END = 0,
		RT_MEMORY,
		RT_OBJECT,
		RT_TYPE_CODE
	};

	struct RecMemory
	{
		void* p;
		size_t size;
	};

	struct RecObject
	{
		Object_ptr p;
	};

	struct RecTypeCode
	{
		TypeCode_ptr p;
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
		return &sync_context_->memory () == &::Nirvana::Core::SynchronizationContext::current ().memory ();
	}

	::Nirvana::Core::Core_var <::Nirvana::Core::SynchronizationContext> sync_context_;
	Tag* cur_ptr_;
	Tag block_ [(BLOCK_SIZE - sizeof (ServantMarshalerImpl <ServantMarshaler>)
	- sizeof (sync_context_) - sizeof (cur_ptr_)) / sizeof (Tag)];
};

static_assert (sizeof (ServantMarshaler) == ServantMarshaler::BLOCK_SIZE, "sizeof (ServantMarshaler)");

}
}
}

#endif
