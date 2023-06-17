/// \file
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
#ifndef NIRVANA_NAMESERVICE_BINDINGITERATOR_H_
#define NIRVANA_NAMESERVICE_BINDINGITERATOR_H_
#pragma once

#include "VirtualIterator.h"
#include <CORBA/CosNaming_s.h>
#include <memory>

namespace CosNaming {
namespace Core {

class BindingIterator :
	public CORBA::servant_traits <CosNaming::BindingIterator>::Servant <BindingIterator>
{
public:
	static CosNaming::BindingIterator::_ref_type create (std::unique_ptr <VirtualIterator>&& vi);

	bool next_one (Binding& b)
	{
		if (vi_)
			return vi_->next_one (b);
		else
			return false;
	}

	bool next_n (uint32_t how_many, BindingList& bl)
	{
		if (!how_many)
			throw CORBA::BAD_PARAM ();

		if (vi_)
			return vi_->next_n (how_many, bl);
		else
			return false;
	}

	void destroy ()
	{
		if (vi_) {
			vi_.reset ();
			_default_POA ()->deactivate_object (_default_POA ()->servant_to_id (this));
		}
	}

private:
	template <class T, class ... Args>
	friend CORBA::servant_reference <T> CORBA::make_reference (Args ... args);

	BindingIterator (std::unique_ptr <VirtualIterator>&& vi) :
		vi_ (std::move (vi))
	{}

private:
	std::unique_ptr <VirtualIterator> vi_;
};

}
}

#endif
