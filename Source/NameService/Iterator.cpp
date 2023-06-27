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
#include "Iterator.h"
#include "NamingContextRoot.h"
#include <CORBA/Server.h>
#include <CORBA/CosNaming_s.h>
#include "../Synchronized.h"
#include "../MemContext.h"
#include "../deactivate_servant.h"

using namespace Nirvana::Core;

namespace CosNaming {
namespace Core {

bool Iterator::next_one (CosNaming::Binding& b)
{
	Binding vb;
	if (next_one (vb)) {
		b.binding_name ().push_back (NamingContextRoot::to_component (std::move (vb.name)));
		b.binding_type (vb.type);
		return true;
	}
	return false;
}

bool Iterator::next_n (uint32_t how_many, CosNaming::BindingList& bl)
{
	bool ret = false;
	CosNaming::Binding b;
	while (how_many-- && next_one (b)) {
		bl.push_back (std::move (b));
		ret = true;
	}
	return ret;
}

class BindingIterator :
	public CORBA::servant_traits <CosNaming::BindingIterator>::Servant <BindingIterator>
{
public:
	static CosNaming::BindingIterator::_ref_type create (std::unique_ptr <Iterator>&& vi);

	bool _non_existent () const noexcept
	{
		return !vi_;
	}

	bool next_one (Binding& b)
	{
		if (!vi_)
			throw CORBA::OBJECT_NOT_EXIST ();

		return vi_->next_one (b);
	}

	bool next_n (uint32_t how_many, BindingList& bl)
	{
		if (!vi_)
			throw CORBA::OBJECT_NOT_EXIST ();

		if (!how_many)
			throw CORBA::BAD_PARAM ();

		return vi_->next_n (how_many, bl);
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

	BindingIterator (std::unique_ptr <Iterator>&& vi) :
		vi_ (std::move (vi))
	{}

private:
	std::unique_ptr <Iterator> vi_;
};

CosNaming::BindingIterator::_ref_type Iterator::create_iterator (std::unique_ptr <Iterator>&& vi)
{
	SYNC_BEGIN (g_core_free_sync_context, &MemContext::current ())
		return CORBA::make_reference <BindingIterator> (std::move (vi))->_this ();
	SYNC_END ()
}

}
}

