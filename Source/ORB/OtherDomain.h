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
#ifndef NIRVANA_ORB_CORE_OTHERDOMAIN_H_
#define NIRVANA_ORB_CORE_OTHERDOMAIN_H_
#pragma once

#include "../CoreInterface.h"
#include <Port/ESIOP.h>
#include "../SharedObject.h"
#include "../Chrono.h"

namespace Nirvana {
namespace ESIOP {

class OtherDomains;

/// Other protection domain communication endpoint.
class NIRVANA_NOVTABLE OtherDomain :
	public Core::SharedObject
{
public:
	/// Platform properties.
	struct Sizes
	{
		size_t allocation_unit; ///< Shared memory ALLOCATION_UNIT.
		size_t block_size; ///< Stream block size granularity.
		size_t sizeof_pointer; ///< sizeof (void*) on target platform.
		size_t sizeof_size; ///< sizeof (size_t) on target platform.
		size_t max_size; ///< maximal size_t value.
	};

	virtual SharedMemPtr reserve (size_t size) = 0;
	virtual SharedMemPtr copy (SharedMemPtr reserved, void* src, size_t& size, bool release_src) = 0;
	virtual void release (SharedMemPtr p, size_t size) = 0;
	virtual void get_sizes (Sizes& sizes) NIRVANA_NOEXCEPT = 0;
	virtual void* store_pointer (void* where, SharedMemPtr p) NIRVANA_NOEXCEPT = 0;
	virtual void* store_size (void* where, size_t size) NIRVANA_NOEXCEPT = 0;

	virtual void send_message (const void* msg, size_t size) = 0;

	/// Check for protection domain is alive.
	/// Protection domain may be killed by system when we hold OtherDomain for it.
	/// But when OtherDomain object exists, id of this domain can't be reused even if the domain is dead.
	/// 
	/// \param domain_id The protection domain id.
	/// \returns `true` if the domain is alive.
	// virtual bool is_alive (ProtDomainId domain_id) = 0;

	/// Factory method must be implemented in the Port layer.
	/// Use CORBA::make_reference for the object creation.
	/// 
	/// \param domain_id Protection domain id.
	/// \returns Created OtherDomain object.
	///   If the \p domain_id is not exists, return `nullptr`.
	static OtherDomain* create (ProtDomainId domain_id);

protected:
	OtherDomain () :
		ref_cnt_ (1)
	{}
	
	virtual ~OtherDomain ()
	{}

private:
	friend class Core::CoreRef <OtherDomain>;
	friend class OtherDomains;

	void _add_ref () NIRVANA_NOEXCEPT
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () NIRVANA_NOEXCEPT
	{
		if (!ref_cnt_.decrement ())
			release_time_ = Core::Chrono::steady_clock ();
	}

private:
	Core::AtomicCounter <false> ref_cnt_;
	Core::Chrono::Duration release_time_;
};

}
}

#endif
