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
#include "Memory.h"

namespace Nirvana {

__declspec (selectany)
extern const ImportInterfaceT <Memory> g_memory = { OLF_IMPORT_INTERFACE, "Nirvana/g_memory", Memory::repository_id_, NIRVANA_STATIC_BRIDGE (Memory, Core::Memory) };

}

NIRVANA_EXPORT (_exp_Nirvana_g_memory, "Nirvana/g_memory", Nirvana::Memory, Nirvana::Core::Memory)
