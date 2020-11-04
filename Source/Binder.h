#ifndef NIRVANA_CORE_BINDER_H_
#define NIRVANA_CORE_BINDER_H_

#include "CoreObject.h"
#include "LifeCyclePseudo.h"
#include "SyncDomain.h"
#include <Nirvana/Binder_s.h>
#include <Nirvana/Hash.h>
#include <CORBA/RepositoryId.h>
#include <unordered_map>
#include <map>

namespace Nirvana {
namespace Core {

class Binder :
	public CoreObject,
	public LifeCyclePseudo <Binder>,
	public CORBA::Nirvana::ImplementationPseudo <Binder, Nirvana::Binder>
{
	typedef CORBA::Nirvana::RepositoryId::Version Version;
	typedef CORBA::Nirvana::Interface_ptr Interface_ptr;

	class NameKey
	{
	public:
		NameKey (const char* id, Version& ver) :
			begin_ (id)
		{
			const char* sver = CORBA::Nirvana::RepositoryId::version (id, id + strlen (id));
			size_ = sver - id;
			ver = Version (sver);
		}

		NameKey (const std::string& id, Version& ver) :
			begin_ (id.c_str ())
		{
			const char* sver = CORBA::Nirvana::RepositoryId::version (begin_, begin_ + id.length ());
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
		size_t operator () (const NameKey& key)
		{
			return Hash::hash_bytes (key.begin (), key.size ());
		}
	};

	typedef std::map <Version, Interface_ptr, std::less <Version>,
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

		std::pair <VersionMap::iterator, bool> emplace (const Version& ver, Interface_ptr itf)
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

	typedef std::unordered_map <NameKey, Versions> Map;

public:
	static void initialize (const void* OLF_section);
	static void terminate ();

	CORBA::Nirvana::Interface_var bind (CORBA::String_in name, CORBA::String_in interface_id);

	class iterator
	{
		Map::iterator it_;
		Version ver;
	};

	void erase (const iterator& it)
	{

	}

private:
	ImplStatic <SyncDomain> sync_domain_;
	Map map_;
};

}
}

#endif
