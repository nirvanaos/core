#include "Binder.h"
#include <Port/SystemInfo.h>
#include <Nirvana/OLF.h>
#include "ORB/POA.h"
#include "ORB/ObjectFactory.h"

namespace Nirvana {
namespace Core {

using namespace std;
using namespace CORBA;
using namespace CORBA::Nirvana;
using namespace PortableServer;
using CORBA::Nirvana::Core::ObjectFactory;
using CORBA::Nirvana::Core::POA;

ImplStatic <SyncDomain> Binder::sync_domain_;
Binder::Map Binder::map_;

class Binder::OLF_Iterator
{
public:
	OLF_Iterator (const void* address, size_t size) :
		cur_ptr_ ((OLF_Command*)address),
		end_ ((OLF_Command*)((uint8_t*)address + size))
	{
		check ();
	}

	bool end () const
	{
		return cur_ptr_ == end_;
	}

	OLF_Command* cur () const
	{
		return cur_ptr_;
	}

	void next ()
	{
		if (!end ()) {
			Word idx = (Word)(*cur_ptr_) - 1;
			assert (idx >= 0);
			cur_ptr_ = (OLF_Command*)((uint8_t*)cur_ptr_ + command_sizes_ [idx]);
			check ();
		}
	}

private:
	void check ();

private:
	OLF_Command* cur_ptr_;
	OLF_Command* end_;

	static const size_t command_sizes_ [OLF_IMPORT_OBJECT];
};

const size_t Binder::OLF_Iterator::command_sizes_ [OLF_IMPORT_OBJECT] = {
	sizeof (ImportInterface),
	sizeof (ExportInterface),
	sizeof (ExportObject),
	sizeof (ExportLocal),
	sizeof (ImportInterface)
};

void Binder::OLF_Iterator::check ()
{
	if (cur_ptr_ >= end_)
		cur_ptr_ = end_;
	else {
		OLF_Command cmd = *cur_ptr_;
		if (OLF_END == cmd)
			cur_ptr_ = end_;
		else if (cmd > OLF_IMPORT_OBJECT)
			throw CORBA::INITIALIZE ();
	}
}

void Binder::add_export (const char* name, CORBA::Nirvana::Interface_ptr itf)
{
	assert (itf);
	Key key (name);
	auto ins = map_.emplace (piecewise_construct, forward_as_tuple (ref (key.name ())), forward_as_tuple (ref (key.version ()), itf));
	if (!ins.second)
		ins.first->second.emplace (key.version (), itf);
}

void Binder::initialize ()
{
	Section exports;
	if (!Port::SystemInfo::get_OLF_section (exports))
		throw_INITIALIZE ();

	add_export (g_binder.imp.name, g_binder.imp.itf);

	// Pass 1: Export pseudo objects.
	bool exp_objects = false;
	for (OLF_Iterator it (exports.address, exports.size); !it.end (); it.next ()) {
		switch (*it.cur ()) {
			case OLF_EXPORT_INTERFACE:
			{
				const ExportInterface* ps = reinterpret_cast <const ExportInterface*> (it.cur ());
				add_export (ps->name, ps->itf);
			}
			break;

			case OLF_EXPORT_OBJECT:
			case OLF_EXPORT_LOCAL:
				exp_objects = true;
				break;

			default:
				throw_INITIALIZE ();
		}
	}

	// Create POA
//	Servant_var <POA> poa = new POA;
//	CORBA::Nirvana::Core::g_root_POA = poa->_this ();

	if (exp_objects) {
		void* writable = Port::Memory::copy (const_cast <void*> (exports.address), const_cast <void*> (exports.address), exports.size, 0);

		// Pass 2: Export objects.
		for (OLF_Iterator it (writable, exports.size); !it.end (); it.next ()) {
			switch (*it.cur ()) {
				case OLF_EXPORT_OBJECT:
				{
					ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
					PortableServer::ServantBase_var core_obj = ObjectFactory::create_servant (TypeI <PortableServer::ServantBase>::in (ps->servant_base));
					Object_ptr obj = AbstractBase_ptr (core_obj)->_query_interface <Object> ();
					ps->core_object = &core_obj._retn ();
					add_export (ps->name, obj);
				}
				break;

				case OLF_EXPORT_LOCAL:
				{
					ExportLocal* ps = reinterpret_cast <ExportLocal*> (it.cur ());
					LocalObject_ptr core_obj = ObjectFactory::create_local_object (TypeI <LocalObject>::in (ps->local_object), TypeI <AbstractBase>::in (ps->abstract_base));
					Object_ptr obj = core_obj;
					ps->core_object = &core_obj;
					add_export (ps->name, obj);
				}
				break;
			}
		}

		Port::Memory::copy (const_cast <void*> (exports.address), writable, exports.size, Memory::READ_ONLY | Memory::RELEASE);
	}
}

}
extern const ImportInterfaceT <Binder> g_binder = { OLF_IMPORT_INTERFACE, "Nirvana/g_binder", Binder::repository_id_, STATIC_BRIDGE (Binder, Core::Binder) };
}
