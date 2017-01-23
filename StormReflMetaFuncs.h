#pragma once

#include <type_traits>
#include <cstdint>

#include <hash/Hash.h>

#include "StormReflMetaInfoBase.h"
#include "StormReflMetaHelpers.h"

template <class T>
using StormReflCheckReflectable = StormReflMetaHelpers::StormReflCheckReflectable<T>;

template <class C>
constexpr int StormReflGetFieldCount()
{
  return StormReflTypeInfo<std::decay_t<C>>::fields_n;
}

template<class C, class Visitor>
void StormReflVisitEach(C & c, Visitor && v)
{
  StormReflMetaHelpers::StormReflFieldIterator<C, Visitor, StormReflGetFieldCount<C>()> itr;
  itr(c, v);
}

template<class C1, class C2, class Visitor>
void StormReflVisitEach(C1 & c1, C2 & c2, Visitor && v)
{
  StormReflMetaHelpers::StormReflFieldIterator<C1, Visitor, StormReflGetFieldCount<C1>()> itr;
  itr(c1, c2, v);
}

template<class C, class Visitor>
void StormReflVisitField(C & c, Visitor && v, Hash field_name_hash)
{
  auto visitor = [&](auto f) { if (f.GetFieldNameHash() == field_name_hash) v(f); };
  StormReflVisitEach(c, visitor);
}

template<class C, class Visitor>
void StormReflVisitField(C & c, Visitor && v, czstr field_name)
{
  Hash hash = crc32(field_name);
  StormReflVisitField(c, v, hash);
}

template <class C>
bool StormReflCompare(const C & c1, const C & c2)
{
  return StormReflMetaHelpers::StormReflEquality<C>::Compare(c1, c2);
}

template <class C>
void StormReflCopy(C & dst, const C & src)
{
  StormReflMetaHelpers::StormReflEquality<C>::Copy(src, dst);
}

template <class C>
void StormReflMove(C & dst, C && src)
{
  StormReflMetaHelpers::StormReflEquality<C>::Move(std::move(src), dst);
}

template <class C>
bool StormReflCompareAndCopy(C & dst, const C & src)
{
  return StormReflMetaHelpers::StormReflEquality<C>::CompareAndCopy(src, dst);
}

template <class C>
void StormReflSefDefault(C & c)
{
  auto visitor = [&](auto f) { f.SetDefault(); };
  StormReflVisitEach(c, visitor);
}

template <class C1, class M1, class C2, class M2>
constexpr bool StormReflCompareMemberPointers(M1 C1::*ptr1, M2 C2::*ptr2)
{
  return StormReflMetaHelpers::StormReflCompareMemberPointers<C1, M1, C2, M2>::Compare(ptr1, ptr2);
}

template <class C1, class M1, class C2, class M2, class Callable, class ... Args>
auto StormReflCompareMemberPointersToCall(M1 C1::*ptr1, M2 C2::*ptr2, Callable && callable, Args && ... args)
{
  StormReflMetaHelpers::StormReflCompareMemberPointers<C1, M1, C2, M2>::CallIfEqual(ptr1, ptr2, callable, std::forward<Args>(args)...);
}

template <class C1, class M1, class C2, class M2, class Callable, class DefaultReturn, class ... Args>
auto StormReflCompareMemberPointersToCallReturn(M1 C1::*ptr1, M2 C2::*ptr2, Callable && callable, DefaultReturn default_return, Args && ... args)
{
  return StormReflMetaHelpers::StormReflCompareMemberPointers<C1, M1, C2, M2>::CallIfEqualReturn(ptr1, ptr2, callable, default_return, std::forward<Args>(args)...);
}

template <class C, class M>
constexpr int StormReflGetMemberFieldIndex(M C::* member_ptr)
{
  return StormReflMetaHelpers::StormReflGetCompareMemberPointerIndex<C, StormReflGetFieldCount<C>() - 1>::Compare(member_ptr);
}

template <typename T>
bool StormReflElementwiseCompare(const T & a, const T & b)
{
  StormReflMetaHelpers::StormReflElementwiseComparer<T> comparer;
  return comparer(a, b);
}

template <typename T>
void StormReflElementwiseCopy(T & a, const T & b)
{
  StormReflMetaHelpers::StormReflElementwiseCopier<T> copier;
  copier(a, b);
}

template <typename T>
void StormReflElementwiseMove(T & a, T & b)
{
  StormReflMetaHelpers::StormReflElementwiseMover<T> mover;
  mover(a, b);
}

template <typename A, typename B>
void StormReflAggregate(A & a, const B & b)
{
  auto visitor = [&](auto f)
  {
    auto adder = [&](auto b)
    {
      f.Get() += b.Get();
    };

    StormReflVisitField(b, adder, f.GetFieldNameHash());
  };

  StormReflVisitEach(a, visitor);
}

template <typename T, int MemberIndex>
constexpr int StormReflGetAnnotationCount()
{
  return StormReflMetaHelpers::StormReflGetAnnotationCount<T, MemberIndex>();
}

template <typename C, int MemberIndex, typename Visitor>
void StormReflVisitFieldAnnotations(Visitor & visitor)
{
  StormReflMetaHelpers::StormReflAnnotationIterator<C, Visitor, MemberIndex, StormReflGetAnnotationCount<C, MemberIndex>()> itr;
  itr(visitor);
}

template <class C, int MemberIndex>
bool StormReflHasAnnotation(const char * annotation)
{
  return StormReflMetaHelpers::StormReflHasAnnotation<C, MemberIndex>(annotation);
}

template <typename C, typename Visitor>
void StormReflVisitFieldsWithAnnotation(const char * annotation, Visitor & visitor)
{
  StormReflMetaHelpers::StormReflVisitFieldsWithAnnotation<C, Visitor>(annotation, visitor);
}

template <typename C, typename Visitor>
void StormReflVisitFieldsWithAnnotation(C & c, const char * annotation, Visitor & visitor)
{
  StormReflMetaHelpers::StormReflVisitFieldsWithAnnotation<C, Visitor>(annotation, visitor);
}

template <typename T>
uint32_t StormReflGetAdditiveHashForType(Hash hash)
{
  return StormReflMetaHelpers::StormReflHashType<T>(hash);
}

template <typename FileInfoType>
uint32_t StormReflGetAdditiveHashForFile(Hash hash)
{
  return StormReflMetaHelpers::StormReflHashFileTypes<FileInfoType, FileInfoType::types_n>::GetHash(hash);
}



