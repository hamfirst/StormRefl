#pragma once

#include "StormReflMetaInfoBase.h"

namespace StormReflMetaHelpers
{
  template<typename T, typename Enable = void> struct StormReflCheckReflectable {
    static_assert(std::is_class<T>::value, "Not a class");

    struct Fallback { bool is_reflectable; };
    struct Derived : T, Fallback { };

    template<typename C, C> struct ChT;

    template<typename C> static char(&f(ChT<bool Fallback::*, &C::is_reflectable>*))[1];
    template<typename C> static char(&f(...))[2];

    static const bool value = sizeof(f<Derived>(0)) == 2;
  };

  template <typename T> struct StormReflCheckReflectable<T, typename std::enable_if<std::is_enum<T>::value>::type> { static const bool value = false; };
  template <typename T, int i> struct StormReflCheckReflectable<T[i], void> { static const bool value = false; };
  template <typename T> struct StormReflCheckReflectable<T *, void> { static const bool value = false; };
  template <> struct StormReflCheckReflectable<bool, void> { static const bool value = false; };
  template <> struct StormReflCheckReflectable<int8_t, void> { static const bool value = false; };
  template <> struct StormReflCheckReflectable<int16_t, void> { static const bool value = false; };
  template <> struct StormReflCheckReflectable<int32_t, void> { static const bool value = false; };
  template <> struct StormReflCheckReflectable<long, void> { static const bool value = false; };
  template <> struct StormReflCheckReflectable<long long, void> { static const bool value = false; };
  template <> struct StormReflCheckReflectable<uint8_t, void> { static const bool value = false; };
  template <> struct StormReflCheckReflectable<uint16_t, void> { static const bool value = false; };
  template <> struct StormReflCheckReflectable<uint32_t, void> { static const bool value = false; };
  template <> struct StormReflCheckReflectable<unsigned long, void> { static const bool value = false; };
  template <> struct StormReflCheckReflectable<unsigned long long, void> { static const bool value = false; };
  template <> struct StormReflCheckReflectable<float, void> { static const bool value = false; };
  template <> struct StormReflCheckReflectable<double, void> { static const bool value = false; };

  template <class C, class Visitor, int I>
  struct StormReflFieldIterator
  {
    void operator()(Visitor & v)
    {
      auto f = typename StormReflTypeInfo<std::decay_t<C>>::template field_data_static<StormReflTypeInfo<std::decay_t<C>>::fields_n - I>{};
      v(f);
      StormReflFieldIterator <C, Visitor, I - 1>() (v);
    }

    void operator()(C& c, Visitor & v)
    {
      auto f = typename StormReflTypeInfo<std::decay_t<C>>::template field_data<StormReflTypeInfo<std::decay_t<C>>::fields_n - I, C>(c);
      v(f);
      StormReflFieldIterator <C, Visitor, I - 1>() (c, v);
    }

    template <class C2>
    void operator()(C & c1, C2 & c2, Visitor & v)
    {
      auto f1 = typename StormReflTypeInfo<std::decay_t<C>>::template field_data<StormReflTypeInfo<std::decay_t<C>>::fields_n - I, C>(c1);
      auto f2 = typename StormReflTypeInfo<std::decay_t<C2>>::template field_data<StormReflTypeInfo<std::decay_t<C2>>::fields_n - I, C2>(c2);
      v(f1, f2);
      StormReflFieldIterator <C, Visitor, I - 1>() (c1, c2, v);
    }

    template <class C2, class C3>
    void operator()(C & c1, C2 & c2, C3 & c3, Visitor & v)
    {
      auto f1 = typename StormReflTypeInfo<std::decay_t<C>>::template field_data<StormReflTypeInfo<std::decay_t<C>>::fields_n - I, C>(c1);
      auto f2 = typename StormReflTypeInfo<std::decay_t<C2>>::template field_data<StormReflTypeInfo<std::decay_t<C2>>::fields_n - I, C2>(c2);
      auto f3 = typename StormReflTypeInfo<std::decay_t<C3>>::template field_data<StormReflTypeInfo<std::decay_t<C3>>::fields_n - I, C3>(c3);
      v(f1, f2, f3);
      StormReflFieldIterator <C, Visitor, I - 1>() (c1, c2, c3, v);
    }
  };

  template <class C, class Visitor>
  struct StormReflFieldIterator<C, Visitor, 0>
  {
    void operator()(Visitor & v)
    {

    }

    void operator()(C& c, Visitor & v)
    {

    }

    template <class C2>
    void operator()(C& c1, C2 & c2, Visitor & v)
    {

    }

    template <class C2, class C3>
    void operator()(C & c1, C2 & c2, C3 & c3, Visitor & v)
    {

    }
  };

  template <class C1, class M1, class C2, class M2>
  struct StormReflCompareMemberPointers
  {
    constexpr static bool Compare(M1 C1::*ptr1, M2 C2::*ptr2)
    {
      return false;
    }

    template <class Callback, class ... Args>
    static void CallIfEqual(M1 C1::*ptr1, M2 C2::*ptr2, Callback && callback, Args &&... args)
    {

    }

    template <class Callback, class DefaultReturn, class ... Args>
    static DefaultReturn CallIfEqualReturn(M1 C1::*ptr1, M2 C2::*ptr2, Callback && callback, DefaultReturn default_return, Args &&... args)
    {
      return default_return;
    }
  };

  template <class C, class M>
  struct StormReflCompareMemberPointers<C, M, C, M>
  {
    constexpr static bool Compare(M C::*ptr1, M C::*ptr2)
    {
      return ptr1 == ptr2;
    }

    template <class Callback, class ... Args>
    static void CallIfEqual(M C::*ptr1, M C::*ptr2, Callback && callback, Args &&... args)
    {
      if (ptr1 == ptr2)
      {
        callback(std::forward<Args>(args)...);
      }
    }

    template <class Callback, class DefaultReturn, class ... Args>
    static DefaultReturn CallIfEqualReturn(M C::*ptr1, M C::*ptr2, Callback && callback, DefaultReturn default_return, Args &&... args)
    {
      if (ptr1 == ptr2)
      {
        return callback(std::forward<Args>(args)...);
      }

      return default_return;
    }
  };

  template <class C1, class M1, class C2, class M2>
  constexpr bool StormReflCompareMembers(M1 C1::*ptr1, M2 C2::*ptr2)
  {
    return StormReflCompareMemberPointers<C1, M1, C2, M2>::Compare(ptr1, ptr2);
  }

  template <class C, int i>
  struct StormReflGetCompareMemberPointerIndex
  {
    template <class M>
    constexpr static int Compare(M C::*ptr)
    {
      return StormReflCompareMembers(StormReflTypeInfo<std::decay_t<C>>::template field_data<i, C>::GetMemberPtr(), ptr) ? i : StormReflGetCompareMemberPointerIndex<C, i - 1>::Compare(ptr);
    }
  };

  template <class C>
  struct StormReflGetCompareMemberPointerIndex<C, -1>
  {
    template <class M>
    constexpr static int Compare(M C::*ptr)
    {
      return -1;
    }
  };


  template <typename T>
  struct StormReflElementwiseComparer
  {
    bool operator()(const T & a, const T & b)
    {
      if (a == b)
      {
        return true;
      }
      
      return false;
    }
  };

  template <typename T, int i>
  struct StormReflElementwiseComparer<T[i]>
  {
    bool operator()(const T * a, const T * b)
    {
      auto end = a + i;
      while (a != end)
      {
        if (*a != *b)
        {
          return false;
        }

        ++a;
        ++b;
      }

      return true;
    }
  };

  template <typename T>
  struct StormReflElementwiseCopier
  {
    void operator()(T & a, const T & b)
    {
      a = b;
    }
  };

  template <typename T, int i>
  struct StormReflElementwiseCopier<T[i]>
  {
    void operator()(T * a, const T * b)
    {
      StormReflElementwiseCopier<T> copier;

      auto end = a + i;
      while (a != end)
      {
        copier(*a, *b);

        ++a;
        ++b;
      }
    }
  };

  template <typename T>
  struct StormReflElementwiseMover
  {
    void operator()(T & dst, T & src)
    {
      dst = std::move(src);
    }
  };

  template <typename T, int i>
  struct StormReflElementwiseMover<T[i]>
  {
    void operator()(T * a, T * b)
    {
      StormReflElementwiseMover<T> mover;

      auto end = a + i;
      while (a != end)
      {
        mover(*a, *b);

        ++a;
        ++b;
      }
    }
  };

  template <class C, class Enable = void>
  struct StormReflEquality
  {
    static void Copy(const C & src, C & dst)
    {
      StormReflElementwiseCopier<C> copier;
      copier(dst, src);
    }

    static void Move(C && src, C & dst)
    {
      StormReflElementwiseMover<C> mover;
      mover(dst, src);
    }

    static bool Compare(const C & a, const C & b)
    {
      StormReflElementwiseComparer<C> comparer;
      return comparer(a, b);
    }

    static bool CompareAndCopy(const C & src, C & dst)
    {
      if (Compare(src, dst))
      {
        return false;
      }

      Copy(src, dst);
      return true;
    }
  };

  template <class C>
  struct StormReflEquality<C, typename std::enable_if<StormReflCheckReflectable<C>::value>::type>
  {
    static void Copy(const C & src, C & dst)
    {
      auto copier = [](auto & src_f, auto & dst_f)
      {
        auto & src_member = src_f.Get();
        auto & dst_member = dst_f.Get();

        using MemberType = typename std::template remove_reference_t<decltype(dst_member)>;

        StormReflEquality<MemberType>::Copy(src_member, dst_member);
      };

      StormReflVisitEach(src, dst, copier);
    }

    static void Move(C && src, C & dst)
    {
      auto mover = [](auto & src_f, auto & dst_f)
      {
        auto & src_member = src_f.Get();
        auto & dst_member = dst_f.Get();

        using MemberType = typename std::template remove_reference_t<decltype(dst_member)>;

        StormReflEquality<MemberType>::Move(std::move(src_member), dst_member);
      };

      StormReflVisitEach(src, dst, mover);
    }

    static bool Compare(const C & a, const C & b)
    {
      bool changed = true;
      auto copier = [&](auto & src_f, auto & dst_f)
      {
        auto & src_member = src_f.Get();
        auto & dst_member = dst_f.Get();

        using MemberType = typename std::template remove_const_t<std::template remove_reference_t<decltype(dst_member)>>;
        changed &= StormReflMetaHelpers::StormReflEquality<MemberType>::Compare(src_member, dst_member);
      };

      StormReflVisitEach(a, b, copier);
      return changed;
    }

    static bool CompareAndCopy(const C & src, C & dst)
    {
      bool changed = false;
      auto copier = [&](auto & src_f, auto & dst_f)
      {
        auto & src_member = src_f.Get();
        auto & dst_member = dst_f.Get();

        using MemberType = typename std::template remove_reference_t<decltype(dst_member)>;
        changed &= StormReflMetaHelpers::StormReflEquality<MemberType>::CompareAndCopy(src_member, dst_member);
      };

      StormReflVisitEach(src, dst, copier);
      return true;
    }
  };

  inline Hash StormReflAdditiveHash(const char * str, Hash hash)
  {
    while (*str != 0)
    {
      hash = crc32additive(hash, *str);
      str++;
    }

    return crc32additive(hash, 0);
  }

  template <typename T>
  Hash StormReflHashType(Hash hash)
  {
    hash = StormReflAdditiveHash(StormReflTypeInfo<T>::GetName(), hash);

    auto visitor = [&](auto f)
    {
      hash = StormReflAdditiveHash(f.GetName(), hash);
      hash = StormReflAdditiveHash(f.GetType(), hash);
    };

    hash = crc32additive(hash, 1);
    hash = crc32additive(hash, 2);

    T * t = nullptr;
    StormReflFieldIterator<T, decltype(visitor), StormReflTypeInfo<std::decay_t<T>>::fields_n> itr;
    itr(*t, visitor);
    return hash;
  }

  template <typename T, int MemberIndex>
  constexpr int StormReflGetAnnotationCount()
  {
    return StormReflTypeInfo<T>::template annotations<MemberIndex>::annotations_n;
  }

  template <typename C, typename Visitor, int MemberIndex, int AnnotationIndex>
  struct StormReflAnnotationIterator
  {
    void operator()(Visitor & v)
    {
      using AnnotationTypeInfo = typename StormReflTypeInfo<std::decay_t<C>>::template annotations<MemberIndex>;
      constexpr int AnnotationIndexInv = AnnotationTypeInfo::annotations_n - AnnotationIndex;

      auto a = typename StormReflTypeInfo<std::decay_t<C>>::template annotations<MemberIndex>::template annoation<AnnotationIndexInv>{};
      v(a);
      StormReflAnnotationIterator <C, Visitor, MemberIndex, AnnotationIndex - 1>() (v);
    }
  };

  template <typename C, typename Visitor, int MemberIndex>
  struct StormReflAnnotationIterator<C, Visitor, MemberIndex, 0>
  {
    void operator()(Visitor & v)
    {

    }
  };

  template <typename C, int MemberIndex>
  bool StormReflHasAnnotation(const char * annotation)
  {
    bool has_annotation = false;
    uint32_t annotation_hash = crc32(annotation);

    auto visitor = [&](auto a)
    {
      if (a.GetAnnotationHash() == annotation_hash)
      {
        has_annotation = true;
      }
    };

    StormReflAnnotationIterator<C, decltype(visitor), MemberIndex, StormReflGetAnnotationCount<C, MemberIndex>()> itr;
    itr(visitor);

    return has_annotation;
  }

  template <typename C, int MemberIndex>
  const char * StormReflGetAnnotationValue(const char * annotation)
  {
    const char * value = nullptr;
    int annotation_length = 0;
    for (const char * ptr = annotation; *ptr != 0; ++ptr, annotation_length++);

    auto visitor = [&](auto a)
    {
      auto check_annotation = a.GetAnnotation();
      auto ptr = annotation;

      for (std::size_t index = (std::size_t)annotation_length; index > 0; --index, ++ptr, ++check_annotation)
      {
        if (*ptr != *check_annotation)
        {
          return;
        }
      }

      if (*check_annotation != ':' || *(check_annotation + 1) != ' ')
      {
        return;
      }

      value = check_annotation + 2;
    };

    StormReflAnnotationIterator<C, decltype(visitor), MemberIndex, StormReflGetAnnotationCount<C, MemberIndex>()> itr;
    itr(visitor);

    return value;
  }

  template <typename T, typename Visitor>
  void StormReflVisitFieldsWithAnnotation(const char * annotation, Visitor & v)
  {
    auto visitor = [&](auto f)
    {
      if (StormReflHasAnnotation<T, std::decay_t<decltype(f)>::GetFieldIndex()>(annotation))
      {
        v(f);
      }
    };

    StormReflMetaHelpers::StormReflFieldIterator<T, decltype(visitor), StormReflTypeInfo<std::decay_t<T>>::fields_n> itr;
    itr(visitor);
  }

  template <typename T, int I>
  struct StormReflHashFileTypes
  {
    static Hash GetHash(Hash hash)
    {
      using Type = typename T::template type_info<I - 1>::type;
      return StormReflHashFileTypes<T, I - 1>::GetHash(StormReflHashType<Type>(hash));
    }
  };

  template <typename T>
  struct StormReflHashFileTypes<T, 0>
  {
    static Hash GetHash(Hash hash)
    {
      return hash;
    }
  };

  template<typename T>
  struct StormReflHasDefault
  {
    using MetaType = StormReflTypeInfo<T>;

    template<typename U, T &()> struct SFINAE {};
    template<typename U> static char Test(SFINAE<U, &U::GetDefault>*);
    template<typename U> static int Test(...);
    static const bool value = sizeof(Test<MetaType>(0)) == sizeof(char);
  };

  template <typename T, bool HasDefault>
  struct StormReflGetDefault
  {
    static T * Process()
    {
      return nullptr;
    }
  };

  template <typename T>
  struct StormReflGetDefault<T, true>
  {
    static T * Process()
    {
      auto & default_data = StormReflTypeInfo<T>::GetDefault();
      return &default_data;
    }
  };
}

