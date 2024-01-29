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
#include "pch.h"
#include "../Source/FileAccessBuf.h"
#include "../Source/ORB/ExceptionHolder.h"
#include "../Source/Launcher.h"

NIRVANA_EXPORT_FACTORY (_exp_Nirvana_AccessBuf, Nirvana::AccessBuf)
NIRVANA_EXPORT_FACTORY (_exp_Messaging_ExceptionHolder, Messaging::ExceptionHolder)
NIRVANA_EXPORT_OBJECT (_exp_Nirvana_g_launcher, Nirvana::Static_g_launcher)

