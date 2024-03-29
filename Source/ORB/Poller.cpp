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
#include "../pch.h"
#include "Poller.h"
#include "call_handler.h"

namespace CORBA {
using namespace Internal;
namespace Core {

Interface::_ptr_type Poller::_query_valuetype (String_in id)
{
	if (id.empty ())
		return &primary_->value;
	else {
		const ValueEntry* pv = find_value (id);
		if (pv)
			return &pv->value;
		else
			return nullptr;
	}
}

Poller::ValueEntry* Poller::find_value (String_in iid)
{
	ValueEntry* pv = std::lower_bound (values_.begin (), values_.end (), iid);
	if (pv != values_.end () && RepId::compatible (pv->iid, pv->iid_len, iid))
		return pv;
	return nullptr;
}

void Poller::create_value (ValueEntry& ve, const ProxyManager::InterfaceEntry& ie)
{
	if (!ve.value) {
		const InterfaceMetadata& metadata = ie.metadata ();
		const Char* const* base = metadata.interfaces.p;
		const Char* const* base_end = base + metadata.interfaces.size;
		++base;
		for (; base != base_end; ++base) {
			const ProxyManager::InterfaceEntry* base_ie = proxy_->find_interface (*base);
			assert (base_ie);
			const Char* base_poller_id = base_ie->metadata ().poller_id;
			if (base_poller_id) {
				ValueEntry* base_poller = find_value (base_poller_id);
				assert (base_poller);
				create_value (*base_poller, *base_ie);
				// May be created in the nested call
				if (ve.value)
					return;
			}
		}
		Interface::_ref_type holder;
		Interface* val = ie.proxy_factory->create_poller (this, 
			(UShort)(&ie - proxy_->interfaces ().begin ()) + 1, holder);
		if (!val || !holder)
			throw MARSHAL ();
		ve.value = Interface::_check (val, ve.iid);
		ve.holder = std::move (holder);
	}
}

void Poller::on_complete (IORequest::_ptr_type reply)
{
	// Called in synchronization domain
	if (associated_handler_)
		call_handler (proxy_->operation_metadata (op_), reply, associated_handler_, op_handler_);
	else
		reply_ = reply;

	Pollable::on_complete (reply);
}

}
}
