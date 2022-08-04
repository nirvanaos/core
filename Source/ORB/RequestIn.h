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
#ifndef NIRVANA_ORB_CORE_REQUESTIN_H_
#define NIRVANA_ORB_CORE_REQUESTIN_H_
#pragma once

#include <CORBA/Server.h>
#include "StreamIn.h"
#include "IDL/IORequest_s.h"
#include "RqHelper.h"
#include "../CoreObject.h"
#include "../LifeCyclePseudo.h"

namespace CORBA {
namespace Core {

/// Implements IORequest for GIOP messages
class RequestIn :
	public servant_traits <Internal::IORequest>::Servant <RequestIn>,
	private RqHelper,
	public Nirvana::Core::CoreObject, // quick create, short life
	public Nirvana::Core::LifeCyclePseudo <RequestIn>
{
public:
	RequestIn (StreamIn& stream) :
		stream_ (&stream)
	{}

private:
	void marshal_op ();

private:
	Nirvana::Core::CoreRef <StreamIn> stream_;
};

}
}

#endif
