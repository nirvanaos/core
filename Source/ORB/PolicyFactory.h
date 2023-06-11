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
#ifndef NIRVANA_ORB_CORE_POLICYFACTORY_H_
#define NIRVANA_ORB_CORE_POLICYFACTORY_H_
#pragma once

#include <CORBA/CORBA.h>
#include "StreamIn.h"
#include "StreamOut.h"

/// COMPRESSION_MIN_RATIO_POLICY_ID = 67.
/// See ZIOP::CompressionMinRatioPolicy.
#define MAX_ORB_POLICY_ID 67

namespace CORBA {
namespace Core {

class PolicyFactory
{
public:
	struct Functions
	{
		Policy::_ref_type (*create) (const Any&);
		Policy::_ref_type (*read) (StreamIn&);
		void (*write) (Policy::_ptr_type, StreamOut&);
	};

	static Policy::_ref_type create (PolicyType type, const Any& val)
	{
		const Functions* f = functions (type);
		if (!f)
			throw PolicyError (BAD_POLICY);
		return (f->create) (val);
	}

	static Policy::_ref_type create (PolicyType type, const OctetSeq& data);

	static void write (Policy::_ptr_type, StreamOut& out);

private:
	static const Functions* functions (PolicyType type) noexcept;

	static const Functions* const ORB_policies_ [MAX_ORB_POLICY_ID];
};

class PolicyUnsupported
{
public:
	static PolicyFactory::Functions functions_;

private:
	static Policy::_ref_type create (const Any&);
};

}
}

#endif
