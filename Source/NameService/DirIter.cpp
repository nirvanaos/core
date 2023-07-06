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
#include "DirIter.h"
#include "Dir.h"
#include "../deactivate_servant.h"

using namespace CosNaming;

namespace Nirvana {
namespace Core {

bool DirIter::next_one (DirEntry& de)
{
	if (end ())
		return false;

	Binding b;
	while (iterator_->next_one (b)) {
		const NameComponent& nc = b.binding_name ().front ();
		size_t cc = nc.id ().size ();
		if (!nc.kind ().empty ())
			cc += nc.kind ().size () + 1;
		de.name ().reserve (cc);
		de.name (nc.id ());
		if (!nc.kind ().empty ()) {
			de.name () += '.';
			de.name () += nc.kind ();
		}

/*
		if (flags_ & USE_REGEX) {
			std::basic_string <char32_t> wname;
			utf8_to_wide (de.name (), wname);
			if (!std::regex_match (wname, regex_))
				continue;
		}
*/

		try {
			DirItem::_ref_type item = dir_->resolve (b.binding_name ());
			item->stat (de.st ());
		} catch (const CORBA::NO_PERMISSION ()) {
			if (flags_ & Nirvana::DirIterator::SKIP_PERMISSION_DENIED)
				continue;
			throw;
		}

		return true;
	}

	iterator_ = nullptr;
	dir_ = nullptr;
	return false;
}

bool DirIter::next_n (uint32_t how_many, DirEntryList& l)
{
	if (end ())
		return false;

	bool ret = false;
	DirEntry entry;
	while (how_many-- && next_one (entry)) {
		l.push_back (std::move (entry));
		ret = true;
	}
	return ret;
}

class DirIterator :
	public CORBA::servant_traits <Nirvana::DirIterator>::Servant <DirIterator>
{
public:
	static Nirvana::DirIterator::_ref_type create (std::unique_ptr <DirIter>&& vi);

	bool _non_existent () const noexcept
	{
		return !vi_;
	}

	bool next_one (DirEntry& entry)
	{
		if (!vi_)
			throw CORBA::OBJECT_NOT_EXIST ();

		return vi_->next_one (entry);
	}

	bool next_n (uint32_t how_many, DirEntryList& list)
	{
		if (!vi_)
			throw CORBA::OBJECT_NOT_EXIST ();

		if (!how_many)
			throw CORBA::BAD_PARAM ();

		return vi_->next_n (how_many, list);
	}

	void destroy ()
	{
		if (!vi_)
			throw CORBA::OBJECT_NOT_EXIST ();

		vi_ = nullptr;
		deactivate_servant (this);
	}

private:
	template <class T, class ... Args>
	friend CORBA::servant_reference <T> CORBA::make_reference (Args ... args);

	DirIterator (std::unique_ptr <DirIter>&& vi) :
		vi_ (std::move (vi))
	{}

private:
	std::unique_ptr <DirIter> vi_;
};

Nirvana::DirIterator::_ref_type DirIter::create_iterator (std::unique_ptr <DirIter>&& vi)
{
	SYNC_BEGIN (g_core_free_sync_context, &MemContext::current ())
		return CORBA::make_reference <DirIterator> (std::move (vi))->_this ();
	SYNC_END ()
}

}
}
