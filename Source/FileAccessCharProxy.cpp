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
#include "FileAccessCharProxy.h"

namespace Nirvana {
namespace Core {

void FileAccessCharProxy::disconnect_push_supplier () noexcept
{
	CosEventComm::PushConsumer::_ref_type tmp (std::move (push_consumer_));
	if (tmp) {
		access_->remove_push_consumer (*this);
		try {
			tmp->disconnect_push_consumer ();
		} catch (...) {
			// TODO: Log
		}
	}
}

void FileAccessCharProxy::disconnect_pull_supplier () noexcept
{
	if (pull_consumer_connected_) {
		CosEventComm::PullConsumer::_ref_type tmp = pull_consumer_;
		pull_consumer_connected_ = false;
		access_->remove_pull_consumer ();
		if (tmp) {
			try {
				tmp->disconnect_pull_consumer ();
			} catch (...) {
				// TODO: Log
			}
		}
	}
}

bool FileAccessCharProxy::really_need_text_fix (const CORBA::Any& evt) const
{
	if (flags_ & O_TEXT) {
		const IDL::String* s;
		if (evt >>= s)
			return s->find (Port::FileSystem::eol () [0]) != IDL::String::npos;
	}
	return false;
}

void FileAccessCharProxy::text_fix (CORBA::Any& evt)
{
	IDL::String* s;
	if (evt >>= s) {
		for (char& c : *s) {
			if (replace_eol () == c)
				c = '\n';
		}
	}
}

}
}
