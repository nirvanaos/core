/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#ifndef NIRVANA_CORE_EXECUTOR_H_
#define NIRVANA_CORE_EXECUTOR_H_
#pragma once

#include "CoreInterface.h"

namespace Nirvana {
namespace Core {

class Thread;

/// @brief Executor is interface that queued to a scheduler
/// and then called to do some work.
class NIRVANA_NOVTABLE Executor
{
	DECLARE_CORE_INTERFACE

public:
	/// @brief Called from the worker thread.
	/// 
	/// @param worker The worker.
	/// @param holder Smart reference to the Executor itself.
	virtual void execute (Thread& worker, Ref <Executor> holder) noexcept = 0;
};

}
}

#endif
