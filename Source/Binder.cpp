#include "Binder.h"
#include "Module.h"
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

void Binder::initialize ()
{
	Section metadata;
	if (!Port::SystemInfo::get_OLF_section (metadata))
		throw_INITIALIZE ();

	add_export (nullptr, g_binder.imp.name, g_binder.imp.itf);
	bind_module (nullptr, metadata);
}

void Binder::bind_module (Module* mod, const Section& metadata)
{
	enum MetadataFlags
	{
		IMPORT_INTERFACES = 0x01,
		EXPORT_OBJECTS = 0x02,
		IMPORT_OBJECTS = 0x04
	};

	const ImportInterface* module_entry = nullptr;

	// Pass 1: Export pseudo objects and bind g_module.
	unsigned flags = 0;
	Key k_gmodule (g_module.imp.name);
	for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
		switch (*it.cur ()) {
			case OLF_IMPORT_INTERFACE:
				assert (mod);
				{
					ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
					Key key (ps->name);
					if (key.is_a (k_gmodule)) {
						if (!k_gmodule.compatible (key))
							throw_INITIALIZE ();
						module_entry = ps;
						ps->itf = &mod->_get_ptr ();
					}
				}
				flags |= MetadataFlags::IMPORT_INTERFACES;
				break;

			case OLF_EXPORT_INTERFACE:
			{
				const ExportInterface* ps = reinterpret_cast <const ExportInterface*> (it.cur ());
				add_export (mod, ps->name, ps->itf);
			}
			break;

			case OLF_EXPORT_OBJECT:
			case OLF_EXPORT_LOCAL:
				flags |= MetadataFlags::EXPORT_OBJECTS;
				break;

			case OLF_IMPORT_OBJECT:
				flags |= MetadataFlags::IMPORT_OBJECTS;
				break;

			default:
				throw_INITIALIZE ();
		}
	}

	if (!mod) {
		// Create POA
		// TODO: It is temporary solution.
		Servant_var <POA> poa = new POA;
		CORBA::Nirvana::Core::g_root_POA = poa->_this ();
	}

	if (flags) {
		void* writable = Port::Memory::copy (const_cast <void*> (metadata.address), const_cast <void*> (metadata.address), metadata.size, Memory::READ_WRITE);

		if (module_entry)
			module_entry = (ImportInterface*)((uint8_t*)module_entry + ((uint8_t*)writable - (uint8_t*)metadata.address));

		try {
			// Pass 2: Import pseudo objects.
			if (flags & MetadataFlags::IMPORT_INTERFACES)
				for (OLF_Iterator it (writable, metadata.size); !it.end (); it.next ()) {
					if (OLF_IMPORT_INTERFACE == *it.cur ()) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						if (ps != module_entry)
							ps->itf = &bind_sync (ps->name, ps->interface_id)._retn ();
					}
				}

			// Pass 3: Export objects.
			if (flags & MetadataFlags::EXPORT_OBJECTS)
				for (OLF_Iterator it (writable, metadata.size); !it.end (); it.next ()) {
					switch (*it.cur ()) {
						case OLF_EXPORT_OBJECT:
						{
							ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
							PortableServer::ServantBase_var core_obj = ObjectFactory::create_servant (TypeI <PortableServer::ServantBase>::in (ps->servant_base));
							Object_ptr obj = AbstractBase_ptr (core_obj)->_query_interface <Object> ();
							ps->core_object = &core_obj._retn ();
							add_export (nullptr, ps->name, obj);
						}
						break;

						case OLF_EXPORT_LOCAL:
						{
							ExportLocal* ps = reinterpret_cast <ExportLocal*> (it.cur ());
							LocalObject_ptr core_obj = ObjectFactory::create_local_object (TypeI <LocalObject>::in (ps->local_object), TypeI <AbstractBase>::in (ps->abstract_base));
							Object_ptr obj = core_obj;
							ps->core_object = &core_obj;
							add_export (nullptr, ps->name, obj);
						}
						break;
					}
				}

			// Pass 4: Import objects.
			if (flags & MetadataFlags::IMPORT_OBJECTS)
				for (OLF_Iterator it (writable, metadata.size); !it.end (); it.next ()) {
					if (OLF_IMPORT_OBJECT == *it.cur ()) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						ps->itf = &bind_sync (ps->name, ps->interface_id)._retn ();
					}
				}

			Port::Memory::copy (const_cast <void*> (metadata.address), writable, metadata.size, Memory::READ_ONLY | Memory::RELEASE);
		} catch (...) {
			if (writable != metadata.address)
				Port::Memory::release (writable, metadata.size);
			throw;
		}
	}
}

void Binder::add_export (Module* mod, const char* name, CORBA::Nirvana::Interface_ptr itf)
{
	assert (itf);
	Key key (name);
	auto ins = map_.emplace (name, itf);
	if (!ins.second)
		throw_INV_OBJREF ();	// Duplicated name

	if (mod) {
		try {
			mod->exported_interfaces_.push_back (ins.first);
		} catch (...) {
			map_.erase (ins.first);
			throw;
		}
	}
}

inline
Interface_var Binder::bind_sync (const char* name, size_t name_len, const char* iid, size_t iid_len)
{
	Key key (name, name_len);
	auto pf = map_.lower_bound (key);
	if (pf != map_.end () && pf->first.compatible (key)) {
		Interface* itf = &pf->second;
		const StringBase <char> itf_id = itf->_epv ().interface_id;
		const StringBase <char> requested_iid (iid, iid_len);
		if (!RepositoryId::compatible (itf_id, requested_iid)) {
			AbstractBase_ptr ab = AbstractBase::_nil ();
			if (RepositoryId::compatible (itf_id, Object::repository_id_))
				ab = Object_ptr (static_cast <Object*> (itf));
			if (RepositoryId::compatible (itf_id, AbstractBase::repository_id_))
				ab = static_cast <AbstractBase*> (itf);
			else
				throw_INV_OBJREF ();
			Interface* qi = ab->_query_interface (requested_iid);
			if (!qi)
				throw_INV_OBJREF ();
			itf = qi;
		}

		return Interface_ptr (interface_duplicate (itf));

	} else
		throw_OBJECT_NOT_EXIST ();
}

}
extern const ImportInterfaceT <Binder> g_binder = { OLF_IMPORT_INTERFACE, "Nirvana/g_binder", Binder::repository_id_, STATIC_BRIDGE (Binder, Core::Binder) };
}
