#ifndef _COST_OF_OPERATION_H_
#define _COST_OF_OPERATION_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WinMemory.h"

class WinMemory::CostOfOperation
{
public:
  
  CostOfOperation (Word cost = 0)
  {
    m_remap_type = REMAP_NONE;
    m_costs [REMAP_NONE] = cost;
    m_costs [REMAP_PART] = cost;
    m_costs [REMAP_FULL] = cost;
  }
  
  void remap_type (RemapType remap_type)
  {
    m_remap_type |= remap_type;
  }
  
  RemapType decide_remap () const;
  
  Word& operator [] (RemapType remap_type)
  {
    return m_costs [remap_type];
  }
  
  void operator += (Word inc)
  {
    m_costs [REMAP_NONE] += inc;
    m_costs [REMAP_PART] += inc;
    m_costs [REMAP_FULL] += inc;
  }
  
protected:
  Word m_remap_type;
  Word m_costs [REMAP_FULL + 1];
};

#endif // _COST_OF_OPERATION_H_

