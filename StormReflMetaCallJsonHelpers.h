#pragma once

#include "StormReflJson.h"

namespace StormReflMetaHelpers
{
  template <class StringBuilder, typename Arg, typename ... Args>
  void StormReflCallSerializeJsonParameterPack(StringBuilder & sb, Arg && arg, Args && ... args)
  {
    StormReflEncodeJson(arg, sb);
    sb += ',';
    StormReflCallSerializeJsonParameterPack(sb, std::forward<Args>(args)...);
  }

  template <class StringBuilder>
  void StormReflCallSerializeJsonParameterPack(StringBuilder & sb)
  {

  }
}
