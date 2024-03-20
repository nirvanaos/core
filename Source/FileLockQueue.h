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
#ifndef NIRVANA_CORE_FILELOCKQUEUE_H_
#define NIRVANA_CORE_FILELOCKQUEUE_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/File.h>
#include <Nirvana/SimpleList.h>
#include "UserObject.h"
#include "EventSync.h"

namespace Nirvana {
namespace Core {

class FileLockQueue
{
public:
	class Entry :
		public FileLock,
		public SimpleList <Entry>::Element,
		public UserObject
	{
	public:
		const void* owner () const noexcept
		{
			return owner_;
		}

	private:
		const void* owner_;
		EventSync event_;
	};

	void enqueue (const FileLock& lock, const void* owner);
	
	typedef SimpleList <Entry>::iterator iterator;

	iterator begin () const noexcept
	{
		return list_.begin ();
	}

	iterator end () const noexcept
	{
		return list_.end ();
	}

private:
	SimpleList <Entry> list_;
};

}
}

#endif
