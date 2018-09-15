// Nirvana Project
// Compile configuration parameters

#ifndef NIRVANA_CORE_CONFIG_H_
#define NIRVANA_CORE_CONFIG_H_

#include <BasicTypes.h>

namespace Nirvana {
namespace Core {

using namespace ::CORBA;

// Heap parameters

/*
	HEAP_UNIT - минимальный размер выдел€емого блока пам€ти по умолчанию.
	–азмер выдел€емого блока выравниваетс€ в сторону увеличени€ на эту величину.
	“аким образом, за счет выравнивани€, накладные расходы составл€ют
	HEAP_UNIT/2 байт на каждый выделенный блок.
	 роме того, размер битовой карты составл€ет 2 бита на HEAP_UNIT байт кучи.
	“аким образом, оптимальна€ величина HEAP_UNIT зависит от количества и среднего
	размера выделенных блоков.
	¬ классических реализаци€х кучи, накладные расходы составл€ют обычно не менее 8 байт
	на выделенный блок. ƒл€ современных объектно-ориентированных программ характерно большое
	количество небольших блоков пам€ти. “аким образом, накладные расходы в обычной
	куче достаточно велики.
	¬ данной реализации размер блока по умолчанию равен 16 байт.
*/

const ULong HEAP_UNIT_MIN = 16;
const ULong HEAP_UNIT_DEFAULT = 16;
const ULong HEAP_UNIT_MAX = 4096;

/**	–азмер управл€ющего блока кучи. 
–азмер должен быть кратен гранул€рности пам€ти домена защиты - максимальному
значению MAX (ALLOCATION_UNIT, PROTECTION_UNIT, SHARING_UNIT). ≈сли HEAP_DIRECTORY_SIZE
меньше этой величины, заголовок кучи содержит несколько управл€ющих блоков, а сама куча
делитс€ на соответствующее количество частей, кажда€ из которых работает отдельно.
ƒл€ Windows размер заголовка равен 64K. ƒл€ систем с меньшими размерами ALLOCATION_UNIT
и SHARING_UNIT его можно сделать меньше.
*/
const ULong HEAP_DIRECTORY_SIZE = 0x10000;

/** Use exceptions to handle uncommitted pages in heap directory.
When set to `false`, heap algorithm uses `Memory::is_readable ()`
to detect uncommitted pages. `true` provides better performance, but maybe not for all platforms.
*/
const Boolean HEAP_DIRECTORY_USE_EXCEPTION = true;

/** Maximum count of levels in PriorityQueue.
To provide best performance with a probabilistic time complexity of
O(logN) where N is the maximum number of elements, the queue should
have PRIORITY_QUEUE_LEVELS = logN. Too large value degrades the performance.
*/
const ULong SYNC_DOMAIN_PRIORITY_QUEUE_LEVELS = 10; ///< For syncronization domain.
const ULong SYS_DOMAIN_PRIORITY_QUEUE_LEVELS = 10; ///< For system-wide scheduler.

}
}

#endif
