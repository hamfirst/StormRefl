#pragma once

#include "StormReflMetaInfoBase.h"
#include "StormReflMetaEnumHelpers.h"

template <class EnumType>
constexpr int StormReflGetEnumElemCount()
{
  return StormReflEnumInfo<EnumType>::elems_n;
}

template <class EnumType>
const char * StormReflGetEnumAsString(EnumType e)
{
  const char * val = nullptr;
  auto visitor = [&](auto f)
  {
    val = f.GetName();
  };

  StormReflMetaHelpers::StormReflEnumSelector<EnumType, decltype(visitor), StormReflGetEnumElemCount<EnumType>()> itr;
  itr(visitor, e);
  return val;
}

template <class EnumType>
bool StormReflGetEnumFromHash(EnumType & out, uint32_t enum_name_hash)
{
  bool found = false;
  auto visitor = [&](auto f)
  {
    out = f.GetValue();
    found = true;
  };

  StormReflMetaHelpers::StormReflEnumSelector<EnumType, decltype(visitor), StormReflGetEnumElemCount<EnumType>()> itr;
  itr(visitor, enum_name_hash);
  return found;
}

template <class EnumType>
struct StormReflVisitEnumValues
{
  template <class Visitor>
  void VisitEach(Visitor && visitor)
  {
    StormReflMetaHelpers::StormReflEnumIterator<EnumType, decltype(visitor), StormReflGetEnumElemCount<EnumType>()> itr;
    itr(visitor);
  }
};
