#include "CostOfOperation.h"

WinMemory::RemapType WinMemory::CostOfOperation::decide_remap () const
{
  if (m_remap_type & REMAP_FULL) {
    if (m_costs [REMAP_FULL] <= m_costs [REMAP_NONE])
      return REMAP_FULL;
  } else if (m_remap_type & REMAP_PART) {
    if (m_costs [REMAP_PART] <= m_costs [REMAP_NONE])
      return REMAP_PART;
  }
  return REMAP_NONE;
}