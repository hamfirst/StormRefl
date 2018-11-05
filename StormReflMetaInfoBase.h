#pragma once

#include <type_traits>

#include <sb/cstr.h>
#include <sb/match_const.h>

template <typename T>
struct StormReflTypeInfo
{

};

template <typename T>
struct StormReflFuncInfo
{

};

template <typename T>
struct StormReflEnumInfo
{

};

struct StormRelfEmptyBase
{

};

template <>
struct StormReflTypeInfo<StormRelfEmptyBase>
{
  using MyBase = void;
  static constexpr int fields_n = 0;
  template <int N> struct field_data_static {};
  template <int N, typename Self> struct field_data {};
  template <int N> struct annotations { static constexpr int annotations_n = 0; template <int A> struct annoation { }; };
  static constexpr auto GetName() { return "StormRelfEmptyBase"; }
  static constexpr auto GetNameHash() { return 0; }
  static StormRelfEmptyBase & GetDefault() { static StormRelfEmptyBase def; return def; }

  static void * CastFromTypeNameHash(uint32_t type_name_hash, void * ptr)
  {
    auto c = static_cast<StormRelfEmptyBase *>(ptr);
    if(GetNameHash() == type_name_hash) return c;
    return nullptr;
  }

  static const void * CastFromTypeNameHash(uint32_t type_name_hash, const void * ptr)
  {
    auto c = static_cast<const StormRelfEmptyBase *>(ptr);
    if(GetNameHash() == type_name_hash) return c;
    return nullptr;
  }

  static void * CastFromTypeIdHash(std::size_t type_id_hash, void * ptr)
  {
    auto c = static_cast<StormRelfEmptyBase *>(ptr);
    if(typeid(StormRelfEmptyBase).hash_code() == type_id_hash) return c;
    return nullptr;
  }

  static const void * CastFromTypeIdHash(std::size_t type_id_hash, const void * ptr)
  {
    auto c = static_cast<const StormRelfEmptyBase *>(ptr);
    if(typeid(StormRelfEmptyBase).hash_code() == type_id_hash) return c;
    return nullptr;
  }
};

template <>
struct StormReflFuncInfo<StormRelfEmptyBase>
{
  static constexpr int funcs_n = 0;
  template <int N> struct func_data_static {};
};