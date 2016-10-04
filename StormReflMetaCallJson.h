  #pragma once

#include "StormReflMetaCall.h"
#include "StormReflMetaCallJsonHelpers.h"

template <typename StringBuilder, typename C, typename ReturnType, typename ... FuncArgs, typename ... ParamArgs>
void StormReflCallSerializeJson(StringBuilder & sb, ReturnType(C::*func)(FuncArgs...), ParamArgs && ... args)
{
  sb += '[';
  StormReflEncodeJson(StormReflGetMemberFunctionIndex(func), sb);

  if (sizeof...(ParamArgs) != 0)
  {
    sb += ',';
    StormReflMetaHelpers::StormReflCallSerializeJsonParameterPack(sb, std::forward<ParamArgs>(args)...);
  }

  sb += ']';
}

template <typename C, typename ... ProvidedArgs>
bool StormReflCallParseJson(C & c, const char * str, const char *& result, ProvidedArgs && ... args)
{
  StormReflJsonAdvanceWhiteSpace(str);
  if (*str != '[')
  {
    return false;
  }

  str++;
  StormReflJsonAdvanceWhiteSpace(str);
  int func_index = 0;
  if (StormReflJson<int>::Parse(func_index, str, str) == false)
  {
    return false;
  }

  bool parsed = false;

  auto deserializer = [&](auto & t)
  {
    StormReflJsonAdvanceWhiteSpace(str);
    if (*str != ',')
    {
      return false;
    }

    str++;
    StormReflJsonAdvanceWhiteSpace(str);
    return StormReflParseJson(t, str, str);
  };

  auto func_visitor = [&](auto f)
  {
    auto func_pointer = f.GetFunctionPtr();
    parsed = StormReflCallCheck(deserializer, c, func_pointer, args...);
  };

  StormReflVisitFuncByIndex(c, func_visitor, func_index);

  if (parsed)
  {
    StormReflJsonAdvanceWhiteSpace(str);

    if (*str == ']')
    {
      result = str;
      return true;
    }
  }

  return false;
}

template <typename C, typename ... ProvidedArgs>
bool StormReflCallDeserializeJson(C & c, const char * str, ProvidedArgs && ... args)
{
  return StormReflCallParseJson(c, str, str, std::forward<ProvidedArgs>(args)...);
}

