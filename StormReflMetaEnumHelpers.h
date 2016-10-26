#pragma once

namespace StormReflMetaHelpers
{
  template <class C, class Visitor, int I>
  struct StormReflEnumIterator
  {
    void operator()(Visitor & v)
    {
      using info_type = StormReflEnumInfo<std::decay_t<C>>;
      auto f = info_type::template elems<info_type::elems_n - I>();
      v(f);
      StormReflEnumIterator<C, Visitor, I - 1> itr; itr(v);
    }
  };

  template <class C, class Visitor>
  struct StormReflEnumIterator<C, Visitor, 0>
  {
    void operator()(C& c, Visitor & v)
    {

    }
  };

  template <class C, class Visitor, int I>
  struct StormReflEnumSelector
  {
    void operator()(Visitor & v, uint32_t enum_name_hash)
    {
      using info_type = StormReflEnumInfo<std::decay_t<C>>;
      using elem_type = typename info_type::template elems<info_type::elems_n - I>;

      auto f = elem_type{};

      if (f.GetNameHash() == enum_name_hash)
      {
        v(f);
        return;
      }

      StormReflEnumSelector<C, Visitor, I - 1> itr; itr(v, enum_name_hash);
    }

    void operator()(Visitor & v, C enum_val)
    {
      using info_type = StormReflEnumInfo<std::decay_t<C>>;
      using elem_type = typename info_type::template elems<info_type::elems_n - I>;

      auto f = elem_type{};

      if (f.GetValue() == enum_val)
      {
        v(f);
        return;
      }

      StormReflEnumSelector<C, Visitor, I - 1> itr; itr(v, enum_val);
    }
  };

  template <class C, class Visitor>
  struct StormReflEnumSelector<C, Visitor, 0>
  {
    void operator()(Visitor & v, uint32_t enum_name_hash)
    {

    }

    void operator()(Visitor & v, C enum_name_hash)
    {

    }
  };
}
