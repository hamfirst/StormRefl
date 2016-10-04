#pragma once

namespace StormReflMetaHelpers
{
  template <typename T1, typename T2>
  struct StormReflConvertableCompare
  {
    static constexpr bool Compare(T1 t1, T2 t2)
    {
      return false;
    }
  };

  template <typename T>
  struct StormReflConvertableCompare<T, T>
  {
    static constexpr bool Compare(T t1, T t2)
    {
      return t1 == t2;
    }
  };


  template <class C, class Visitor, int I>
  struct StormReflFunctionIterator
  {
    void operator()(Visitor & v)
    {
      auto f = typename StormReflFuncInfo<std::decay_t<C>>::template func_data_static<StormReflFuncInfo<std::decay_t<C>>::funcs_n - I>();
      v(f);
      StormReflFunctionIterator <C, Visitor, I - 1>() (v);
    }
  };

  template <class C, class Visitor>
  struct StormReflFunctionIterator<C, Visitor, 0>
  {
    void operator()(Visitor & v)
    {

    }
  };


  template <typename T, typename Ret1, typename ... Args1, typename Ret2, typename ... Args2>
  constexpr bool StormReflCompareMemberFunctionPointers(Ret1(T::* f1)(Args1...), Ret2(T::* f2)(Args2...))
  {
    return StormReflConvertableCompare<decltype(f1), decltype(f2)>::Compare(f1, f2);
  }

  template <typename C, int i>
  struct StormReflGetCompareMemberFunctionPointerIndex
  {
    template <typename ReturnType, typename ... Args>
    constexpr static int Compare(ReturnType(C::* ptr)(Args...))
    {
      return StormReflCompareMemberFunctionPointers(StormReflFuncInfo<C>::template func_data_static<i>::GetFunctionPtr(), ptr) ?
        i :
        StormReflGetCompareMemberFunctionPointerIndex<C, i - 1>::Compare(ptr);
    }
  };

  template <class C>
  struct StormReflGetCompareMemberFunctionPointerIndex<C, -1>
  {
    template <typename ReturnType, typename ... Args>
    constexpr static int Compare(ReturnType(C::* ptr)(Args...))
    {
      return -1;
    }
  };

  template <typename C, int i>
  struct StormReflGetCompareMemberFunctionPointerParamCount
  {
    template <typename ReturnType, typename ... Args>
    constexpr static int Compare(ReturnType(C::* ptr)(Args...))
    {
      return StormReflCompareMemberFunctionPointers(StormReflFuncInfo<C>::template func_data_static<i>::GetFunctionPtr(), ptr) ?
        StormReflFuncInfo<C>::template func_data_static<i>::params_n :
        StormReflGetCompareMemberFunctionPointerIndex<C, i - 1>::Compare(ptr);
    }
  };

  template <class C>
  struct StormReflGetCompareMemberFunctionPointerParamCount<C, -1>
  {
    template <typename ReturnType, typename ... Args>
    constexpr static int Compare(ReturnType(C::* ptr)(Args...))
    {
      return 0;
    }
  };

  template <typename Serializer, typename Arg, typename ... Args>
  void StormReflCallSerialize(Serializer & serializer, Arg && arg, Args && ... args)
  {
    serializer(arg);
    StormReflCallSerialize(serializer, std::forward<Args>(args)...);
  }

  template <typename Serializer>
  void StormReflCallSerialize(Serializer & serializer)
  {

  }

  template <typename Arg, typename ... Args>
  struct StormReflCallConsume
  {
    template <typename Deserializer, typename Callable, typename T, typename ReturnType, typename ProvidedArg, typename ... ProvidedArgs>
    static ReturnType StormReflCallDeserialize(Deserializer & deserializer, Callable & callable, ProvidedArg && arg, ProvidedArgs && ... args)
    {
      auto func = [&](Args & ... args)
      {
        return callable(arg, args...);
      };

      return StormReflCallConsume<Args...>::template StormReflCallDeserialize<Deserializer, decltype(func), T, ReturnType, ProvidedArgs...>(
        deserializer, func, std::forward<ProvidedArgs>(args)...);
    }

    template <typename Deserializer, typename Callable, typename T, typename ReturnType>
    static ReturnType StormReflCallDeserialize(Deserializer & deserializer, Callable & callable)
    {
      return StormReflCall<sizeof...(Args)+1>::template StormReflCallDeserialize<Deserializer, Callable, T, ReturnType, Arg, Args...>(deserializer, callable);
    }

    template <typename Deserializer, typename Callable, typename T, typename ReturnType, typename ProvidedArg, typename ... ProvidedArgs>
    static bool StormReflCallDeserializeCheckReturn(Deserializer & deserializer, Callable & callable, ProvidedArg && arg, ProvidedArgs && ... args)
    {
      auto func = [&](Args & ... args)
      {
        return callable(arg, args...);
      };

      return StormReflCallConsume<Args...>::template StormReflCallDeserializeCheckReturn<Deserializer, decltype(func), T, ReturnType, ProvidedArgs...>(
        deserializer, func, std::forward<ProvidedArgs>(args)...);
    }

    template <typename Deserializer, typename Callable, typename T, typename ReturnType>
    static bool StormReflCallDeserializeCheckReturn(Deserializer & deserializer, Callable & callable)
    {
      return StormReflCall<sizeof...(Args)+1>::template StormReflCallDeserializeCheckReturn<Deserializer, Callable, T, ReturnType, Arg, Args...>(deserializer, callable);
    }
  };

  template <int N>
  struct StormReflCall
  {
    template <typename Deserializer, typename Callable, typename T, typename ReturnType, typename Arg, typename ... Args>
    static ReturnType StormReflCallDeserialize(Deserializer & deserializer, Callable & callable)
    {
      std::decay_t<Arg> arg;
      deserializer(arg);

      auto func = [&](Args & ... args)
      {
        return callable(arg, args...);
      };

      return StormReflCall<N - 1>::template StormReflCallDeserialize<Deserializer, decltype(func), T, ReturnType, Args...>(deserializer, func);
    }

    template <typename Deserializer, typename Callable, typename T, typename ReturnType, typename Arg, typename ... Args>
    static bool StormReflCallDeserializeCheckReturn(Deserializer & deserializer, Callable & callable)
    {
      std::decay_t<Arg> arg;
      if (deserializer(arg) == false)
      {
        return false;
      }

      auto func = [&](Args & ... args)
      {
        return callable(arg, args...);
      };

      return StormReflCall<N - 1>::template StormReflCallDeserializeCheckReturn<Deserializer, decltype(func), T, ReturnType, Args...>(deserializer, func);
    }
  };

  template <>
  struct StormReflCall<0>
  {
    template <typename Deserializer, typename Callable, typename T, typename ReturnType>
    static ReturnType StormReflCallDeserialize(Deserializer & deserializer, Callable & callable)
    {
      return callable();
    }

    template <typename Deserializer, typename Callable, typename T, typename ReturnType>
    static bool StormReflCallDeserializeCheckReturn(Deserializer & deserializer, Callable & callable)
    {
      callable();
      return true;
    }
  };
}
