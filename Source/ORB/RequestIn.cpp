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
#include "RequestIn.h"

using namespace Nirvana;
using namespace Nirvana::Core;
using namespace std;

namespace CORBA {
namespace Core {

RequestIn::RequestIn (CoreRef <StreamIn>&& in, CoreRef <CodeSetConverterW>&& cscw) :
	Request (move (cscw))
{
	stream_in_ = move (in);
}

void RequestIn::switch_to_reply ()
{
	if (stream_in_) {
		size_t more_data = !stream_in_->end ();
		stream_in_.reset ();
		if (more_data > 7) // 8-byte alignment is ignored
			throw MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
	}
}

void RequestIn::unmarshal_end ()
{
	switch_to_reply ();
}

void RequestIn::marshal_op ()
{
	switch_to_reply ();
}

void RequestIn::success ()
{
	switch_to_reply ();
}

void RequestIn::cancel ()
{
	throw BAD_INV_ORDER ();
}

const IOP::ObjectKey& RequestIn_1_2::object_key () const
{
	switch (header ().target ()._d ()) {
		case GIOP::KeyAddr:
			return header ().target ().object_key ();

		case GIOP::ProfileAddr:
			return key_from_profile (header ().target ().profile ());

		case GIOP::ReferenceAddr:
		{
			const GIOP::IORAddressingInfo& ior = header ().target ().ior ();
			const IOP::TaggedProfileSeq& profiles = ior.ior ().profiles ();
			if (profiles.size () <= ior.selected_profile_index ())
				throw OBJECT_NOT_EXIST ();
			return key_from_profile (profiles [ior.selected_profile_index ()]);
		}
	}

	throw UNKNOWN ();
}

}
}