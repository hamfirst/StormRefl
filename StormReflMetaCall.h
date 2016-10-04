#pragma once

#include "StormReflMetaCallHelpers.h"

template <typename C>
constexpr static int StormReflGetFunctionCount()
{
  return StormReflFuncInfo<C>::funcs_n;
}

template <typename C, typename ReturnType, typename ... Args>
constexpr static int StormReflGetMemberFunctionIndex(ReturnType(C::* ptr)(Args...))
{
  return StormReflMetaHelpers::StormReflGetCompareMemberFunctionPointerIndex<C, StormReflGetFunctionCount<C>() - 1>::Compare(ptr);
}

template <typename C, typename ReturnType, typename ... Args>
constexpr static uint64_t StormReflGetMemberFunctionHash(ReturnType(C::* ptr)(Args...))
{
  return StormReflFuncInfo<C>::template func_data_static<StormReflGetMemberFunctionIndex(ptr)>::GetFunctionNameHash();
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
  auto call = [&](Args... args)
  {
    return (t.*func)(args...);
  };

  return StormReflMetaHelpers::StormReflCall<sizeof...(Args)>::StormReflCallDeserialize<Deserializer, decltype(call), T, ReturnType, Args...>(deserializer, call);
}


template <typename Deserializer, typename T, typename ReturnType, typename ... Args>
bool StormReflCallCheck(Deserializer & deserializer, T & t, ReturnType(T::*func)(Args...))
{
  auto call = [&](Args... args)
  {
    return (t.*func)(args...);
  };

  return StormReflMetaHelpers::StormReflCall<sizeof...(Args)>::StormReflCallDeserializeCheckReturn<Deserializer, decltype(call), T, ReturnType, Args...>(deserializer, call);
}

template <typename Deserializer, typename T, typename ReturnType, typename ... Args, typename ProvidedArg, typename ... ProvidedArgs>
ReturnType StormReflCall(Deserializer & deserializer, T & t, ReturnType(T::*func)(Args...), ProvidedArg && provided_arg, ProvidedArgs && ... provided_args)
{
  auto call = [&](Args... args)
  {
    return (t.*func)(args...);
  };

  return StormReflMetaHelpers::StormReflCallConsume<Args...>::template StormReflCallDeserialize<Deserializer, decltype(call), T, ReturnType, ProvidedArg, ProvidedArgs...>(
    deserializer, call, std::forward<ProvidedArg>(provided_arg), std::forward<ProvidedArgs>(provided_args)...);
}

template <typename Deserializer, typename T, typename ReturnType, typename ... Args, typename ProvidedArg, typename ... ProvidedArgs>
bool StormReflCallCheck(Deserializer & deserializer, T & t, ReturnType(T::*func)(Args...), ProvidedArg && provided_arg, ProvidedArgs && ... provided_args)
{
  auto call = [&](Args... args)
  {
    return (t.*func)(args...);
  };

  return StormReflMetaHelpers::StormReflCallConsume<Args...>::template StormReflCallDeserializeCheckReturn<Deserializer, decltype(call), T, ReturnType, ProvidedArg, ProvidedArgs...>(
    deserializer, call, std::forward<ProvidedArg>(provided_arg), std::forward<ProvidedArgs>(provided_args)...);
}
