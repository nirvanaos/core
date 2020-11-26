#ifndef NIRVANA_CORE_BINDER_H_
#define NIRVANA_CORE_BINDER_H_

#include "SyncDomain.h"
#include <Nirvana/Binder_s.h>
#include <Nirvana/Hash.h>
#include <CORBA/RepositoryId.h>

#pragma push_macro ("verify")
#undef verify

#include "parallel-hashmap/parallel_hashmap/phmap.h"
#include "parallel-hashmap/parallel_hashmap/btree.h"

#pragma pop_macro ("verify")

namespace Nirvana {
namespace Core {

class Binder :
	public CORBA::Nirvana::ServantStatic <Binder, Nirvana::Binder>
{
private:
	typedef CORBA::Nirvana::RepositoryId::Version Version;
	typedef CORBA::Nirvana::Interface_ptr Interface_ptr;

	class NameKey
	{
	public:
		NameKey (const char* id, Version& ver) :
			begin_ (id)
		{
			const char* sver = CORBA::Nirvana::RepositoryId::version (id, id + strlen (id));
			size_ = sver - begin_;
			ver = Version (sver);
		}

		NameKey (const std::string& id, Version& ver) :
			begin_ (id.c_str ())
		{
			const char* sver = CORBA::Nirvana::RepositoryId::version (begin_, begin_ + id.length ());
			size_ = sver - begin_;
			ver = Version (sver);
		}

		bool operator == (const NameKey& rhs) const
		{
			return size_ == rhs.size_ && std::equal (begin_, begin_ + size_, rhs.begin_);
		}

		const char* begin () const
		{
			return begin_;
		}

		size_t size () const
		{
			return size_;
		}

	private:
		const char* begin_;
		size_t size_;
	};

	struct NameKeyHash
	{
		size_t operator () (const NameKey& key) const
		{
			return Hash::hash_bytes (key.begin (), key.size ());
		}
	};

	class Key
	{
	public:
		Key (const char* id) :
			name_ (id, version_)
		{}

		Key (const std::string& id) :
			name_ (id, version_)
		{}

		const NameKey& name () const
		{
			return name_;
		}

		const Version& version () const
		{
			return version_;
		}

	private:
		NameKey name_;
		Version version_;
	};

	typedef phmap::btree_map <Version, Interface_ptr, std::less <Version>,
		CoreAllocator <std::pair <Version, Interface_ptr> >
	> VersionMap;

	class Versions
	{
	public:
		Versions (const Version& ver, Interface_ptr itf) :
			use_map_ (false),
			map_ (ver, itf)
		{}

		~Versions ()
		{
			if (use_map_)
				((VersionMap*)map_.map_space)->~VersionMap ();
		}

		size_t size () const
		{
			if (use_map_)
				return ((VersionMap*)map_.map_space)->size ();
			else
				return 1;
		}

		void emplace (const Version& ver, Interface_ptr itf)
		{
			if (!use_map_) {
				std::pair <Version, Interface_ptr> tmp = map_.single;
				try {
					(new (map_.map_space) VersionMap ())->emplace (ver, itf);
				} catch (...) {
					map_.single = tmp;
					throw;
				}
				use_map_ = true;
			}
			((VersionMap*)map_.map_space)->emplace (ver, itf);
		}

		bool erase (const Version& ver);

	private:
		bool use_map_;
		union Map
		{
			Map (const Version& ver, Interface_ptr itf) :
				single (ver, itf)
			{}

			std::pair <Version, Interface_ptr> single;
			uint8_t map_space [sizeof (VersionMap)];
		} map_;
	};

	typedef phmap::node_hash_map <NameKey, Versions, NameKeyHash, std::equal_to <NameKey>, CoreAllocator <std::pair <NameKey, Versions> > > Map;

public:
	static void initialize ();

	static CORBA::Nirvana::Interface_var bind (CORBA::String_in name, CORBA::String_in interface_id)
	{
		throw_NO_IMPLEMENT ();
	}

	class iterator
	{
		Map::iterator it_;
		Version ver;
	};

	static void erase (const iterator& it)
	{

	}

	class OLF_Iterator;

	static void add_export (const char* name, CORBA::Nirvana::Interface_ptr itf);

private:
	static ImplStatic <SyncDomain> sync_domain_;
	static Map map_;
};

}
}

#endif
