#ifndef NIRVANA_CORE_BINDER_H_
#define NIRVANA_CORE_BINDER_H_

#include <Nirvana/Binder_s.h>
#include "SyncDomain.h"
#include "Section.h"
#include "Synchronized.h"
#include <CORBA/RepositoryId.h>
#include <map>

namespace Nirvana {
namespace Core {

class Module;

typedef std::basic_string <char, std::char_traits <char>, CoreAllocator <char>> CoreString;

class Binder :
	public CORBA::Nirvana::ServantStatic <Binder, Nirvana::Binder>
{
private:
	typedef CORBA::Nirvana::RepositoryId RepositoryId;
	typedef CORBA::Nirvana::RepositoryId::Version Version;
	typedef CORBA::Nirvana::Interface_ptr Interface_ptr;
	typedef CORBA::Nirvana::Interface_var Interface_var;

	class Key
	{
	public:
		Key (const char* id, size_t length) :
			name_ (id)
		{
			const char* sver = RepositoryId::version (id, id + length);
			length_ = sver - id;
			version_ = Version (sver);
		}

		Key (const char* id) :
			Key (id, strlen (id))
		{}

		Key (const std::string& id) :
			Key (id.c_str (), id.length ())
		{}

		bool operator < (const Key& rhs) const
		{
			if (length_ < rhs.length_)
				return true;
			else if (length_ > rhs.length_)
				return false;
			else {
				int c = RepositoryId::lex_compare (name_, name_ + length_, rhs.name_, rhs.name_ + length_);
				if (c < 0)
					return true;
				else if (c > 0)
					return false;
			}
			return version_ < rhs.version_;
		}

		bool is_a (const Key& rhs) const
		{
			return length_ == rhs.length_ && std::equal (name_, name_ + length_, rhs.name_);
		}

		bool compatible (const Key& rhs) const
		{
			return length_ == rhs.length_
				&& !RepositoryId::lex_compare (name_, name_ + length_, rhs.name_, rhs.name_ + length_)
				&& version_.compatible (rhs.version_);
		}

	private:
		const char* name_;
		size_t length_;
		Version version_;
	};

	typedef std::map <Key, Interface_ptr, std::less <Key>, CoreAllocator <std::pair <Key, Interface_ptr> > > Map;

public:
	static void initialize ();

	static Interface_var bind (const std::string& _name, const std::string& _iid)
	{
		CoreString name (_name.c_str (), _name.length ()), iid (_iid.c_str (), _iid.length ());
		Synchronized sync (&sync_domain_);
		return bind_sync (name, iid);
	}

	typedef Map::iterator Iterator;

private:
	class OLF_Iterator;

	static void add_export (Module* mod, const char* name, CORBA::Nirvana::Interface_ptr itf);
	static void bind_module (Module* mod, const Section& metadata);

	static Interface_var bind_sync (const CoreString& name, const CoreString& iid)
	{
		return bind_sync (name.c_str (), name.length (), iid.c_str (), iid.length ());
	}

	static Interface_var bind_sync (const char* name, const char* iid)
	{
		return bind_sync (name, strlen (name), iid, strlen (iid));
	}

	static Interface_var bind_sync (const char* name, size_t name_len, const char* iid, size_t iid_len);

private:
	static ImplStatic <SyncDomain> sync_domain_;
	static Map map_;
};

}
}

#endif
