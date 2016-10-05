#pragma once

#include "StormReflMetaCallHelpers.h"

template <typename C>
constexpr int StormReflGetFunctionCount()
{
  return StormReflFuncInfo<C>::funcs_n;
}

template <typename C, typename ReturnType, typename ... Args>
constexpr int StormReflGetMemberFunctionIndex(ReturnType(C::* ptr)(Args...))
{
  return StormReflMetaHelpers::StormReflGetCompareMemberFunctionPointerIndex<C, StormReflGetFunctionCount<C>() - 1>::Compare(ptr);
}

template <typename C, typename ReturnType, typename ... Args>
constexpr uint64_t StormReflGetMemberFunctionHash(ReturnType(C::* ptr)(Args...))
{
  return StormReflFuncInfo<C>::template func_data_static<StormReflGetMemberFunctionIndex(ptr)>::GetFunctionNameHash();
}

template <typename C, int FuncIndex, int ParamIndex>
struct StormReflGetParamType : StormReflMetaHelpers::StormReflParamInfo<C, FuncIndex, ParamIndex>
{

};

template <typename C, int FuncIndex, int ParamIndex, typename ParamType>
constexpr bool StormReflIsParamOfType()
{
  return std::is_same<typename StormReflGetParamType<C, FuncIndex, ParamIndex>::type, ParamType>::value;
}


template<class C, class Visitor>
void StormReflVisitFuncs(C & c, Visitor & v)
{
  StormReflMetaHelpers::StormReflFunctionIterator<C, Visitor, StormReflGetFunctionCount<C>()> itr;
  itr(v);
}

template<class C, class Visitor>
void StormReflVisitFuncByIndex(C & c, Visitor & v, int func_index)
{
  auto visitor = [&](auto f) { if (f.GetFunctionIndex() == func_index) v(f); };
  StormReflVisitFuncs(c, visitor);
}

template <typename Serializer, typename C, typename ReturnType, typename ... Args>
auto StormReflCallSerialize(Serializer & serializer, ReturnType(C::*func)(Args...))
{
  auto serialize_func = [&](Args && ... args)
  {
    StormReflMetaHelpers::StormReflCallSerialize(serializer, std::forward<Args>(args)...);
  };

  return serialize_func;
}


template <typename Deserializer, typename T, typename ReturnType, typename ... Args>
ReturnType StormReflCall(Deserializer & deserializer, T & t, ReturnType(T::*func)(Args...))
{
  StormReflMetaHelpers::StormReflReturnBuffer<ReturnType> ret_buffer;
  if (StormReflMetaHelpers::StormReflCreateCallable(deserializer, t, func, (ReturnType *)ret_buffer.m_Buffer, std::forward<ProvidedArgs>(args)...))
  {
    return StormReflMetaHelpers::StormReflReturnDestroy<ReturnType>::Destroy((ReturnType *)ret_buffer.m_Buffer);
  }

  throw std::runtime_error("Deserialize failure");
}

template <typename Deserializer, typename T, typename ReturnType, typename ... Args, typename ... ProvidedArgs>
bool StormReflCallCheck(Deserializer & deserializer, T & t, ReturnType(T::*func)(Args...), ProvidedArgs && ... args)
{
  StormReflMetaHelpers::StormReflReturnBuffer<ReturnType> ret_buffer;
  if (StormReflMetaHelpers::StormReflCreateCallable(deserializer, t, func, (ReturnType *)ret_buffer.m_Buffer, std::forward<ProvidedArgs>(args)...))
  {
    StormReflMetaHelpers::StormReflReturnDestroy<ReturnType>::Destroy((ReturnType *)ret_buffer.m_Buffer);
    return true;
  }

  return false;
}
