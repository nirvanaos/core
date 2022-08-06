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
#ifndef NIRVANA_ORB_CORE_MAPUNORDEREDSTABLE_H_
#define NIRVANA_ORB_CORE_MAPUNORDEREDSTABLE_H_
#pragma once

// Unordered map with
// Map must provide the pointer stability.
// phmap::node_hash_map is declared as with pointer stability,
// but it seems that not. While we use STL implementation.
#define MAPUNORDEREDSTABLE_STL

#ifdef MAPUNORDEREDSTABLE_STL
#include <unordered_map>
#else
#pragma push_macro ("verify")
#undef verify
#include "parallel-hashmap/parallel_hashmap/phmap.h"
#pragma pop_macro ("verify")
#endif

namespace Nirvana {
namespace Core {

/// Unordered map with pointer stability.
template <class Key, class T, class Hash = std::hash <Key>, class KeyEqual = std::equal_to <Key>,
	class Allocator = std::allocator <std::pair <const Key, T> > >
using MapUnorderedStable =
#ifdef MAPUNORDEREDSTABLE_STL
	std::unordered_map
#else
	phmap::node_hash_map
#endif
	<Key, T, Hash, KeyEqual, Allocator>;

}
}

#endif
