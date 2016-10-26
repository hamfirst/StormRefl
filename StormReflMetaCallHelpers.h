#pragma once

template <typename T>
struct StormReflTypeInfo;

template <typename T>
struct StormReflFuncInfo;

template <typename T>
struct StormReflEnumInfo;


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

  template <typename T, int FuncIndex, int ParamIndex, bool InvParamIndex>
  struct StormReflParamDetailInfo
  {
    using type = void;
  };

  template <typename T, int FuncIndex, int ParamIndex>
  struct StormReflParamDetailInfo<T, FuncIndex, ParamIndex, false>
  {
    using type = typename StormReflFuncInfo<T>::template func_data_static<FuncIndex>::template param_info<ParamIndex>::param_type;
  };

  template <typename T, int FuncIndex, int ParamIndex, bool InvFuncIndex>
  struct StormReflParamFuncInfo
  {
    using type = void;
  };

  template <typename T, int FuncIndex, int ParamIndex>
  struct StormReflParamFuncInfo<T, FuncIndex, ParamIndex, false> : 
    StormReflParamDetailInfo<T, FuncIndex, ParamIndex, ParamIndex < StormReflFuncInfo<T>::template func_data_static<FuncIndex>::params_n>
  {

  };

  template <typename T, int FuncIndex, int ParamIndex>
  struct StormReflParamInfo : 
    StormReflParamFuncInfo<T, FuncIndex, ParamIndex, FuncIndex < StormReflFuncInfo<T>::funcs_n>
  {

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

  template <typename ReturnType>
  struct StormReflReturnBuffer
  {
    char m_Buffer[sizeof(ReturnType)];
  };

  template <>
  struct StormReflReturnBuffer<void>
  {
    char m_Buffer[1];
  };

  template <typename ReturnType, typename Enable = void>
  struct StormReflReturnDestroy
  {
    static ReturnType Destroy(ReturnType * t)
    {
      return *t;
    }
  };

  template <typename ReturnType>
  struct StormReflReturnDestroy<ReturnType, typename std::enable_if<std::is_destructible<ReturnType>::value>::type>
  {
    static ReturnType Destroy(ReturnType * t)
    {
      ReturnType t_moved(std::move(*t));
      t->~ReturnType();
      return t_moved;
    }
  };

  template <>
  struct StormReflReturnDestroy<void, void>
  {
    static void Destroy(void * t)
    {

    }
  };

  template <typename ReturnType>
  struct StormReflCallReturnCapture
  {
    template <typename Callable>
    static void Capture(Callable && callable, ReturnType * ret_val)
    {
      new (ret_val) ReturnType(callable());
    }
  };

  template <>
  struct StormReflCallReturnCapture<void>
  {
    template <typename Callable>
    static void Capture(Callable && callable, void * ret_val)
    {
      callable();
    }
  };

  template <typename Deserializer, typename Callable, typename ReturnType>
  bool StormReflCallDeserialize(Deserializer && deserializer, Callable & callable, ReturnType(Callable::*ptr)() const, ReturnType * ret_val);

  template <typename Deserializer, typename Callable, typename ReturnType, typename FuncArg, typename ... FuncArgs>
  bool StormReflCallDeserialize(Deserializer && deserializer, Callable & callable, ReturnType(Callable::*ptr)(FuncArg, FuncArgs...) const, ReturnType * ret_val)
  {
    typename std::remove_const<std::remove_reference_t<FuncArg>>::type f{};
    if (deserializer(f, sizeof...(FuncArgs) == 0) == false)
    {
      return false;
    }

    auto func = [&](FuncArgs & ... args)
    {
      return callable(f, args...);
    };

    return StormReflCallDeserialize(deserializer, func, &decltype(func)::operator(), ret_val);
  }

  template <typename Deserializer, typename Callable, typename ReturnType>
  bool StormReflCallDeserialize(Deserializer && deserializer, Callable & callable, ReturnType(Callable::*ptr)() const, ReturnType * ret_val)
  {
    StormReflCallReturnCapture<ReturnType>::Capture(callable, ret_val);
    return true;
  }

  template <bool Valid>
  struct StormReflCallableAdditive;

  template <typename Deserializer, typename Callable, typename ReturnType, typename FuncArg, typename ProvidedArg, typename ... FuncArgs, typename ... ProvidedArgs>
  static bool StormReflCreateCallableAdditive(Deserializer && deserializer, Callable & callable, ReturnType(Callable::*ptr)(FuncArg, FuncArgs...) const, ReturnType * ret_val,
    ProvidedArg && provided_arg, ProvidedArgs && ... provided_args)
  {
    return StormReflCallableAdditive<std::is_convertible<ProvidedArg, FuncArg>::value>::CreateCallableAdditive(
      deserializer, callable, &Callable::operator(), ret_val, std::forward<ProvidedArg>(provided_arg), std::forward<ProvidedArgs>(provided_args)...);
  }

  template <typename Deserializer, typename Callable, typename ReturnType, typename ... FuncArgs>
  static bool StormReflCreateCallableAdditive(Deserializer && deserializer, Callable & callable, ReturnType(Callable::*ptr)(FuncArgs...) const, ReturnType * ret_val)
  {
    return StormReflCallDeserialize(deserializer, callable, &Callable::operator(), ret_val);
  }

  template <bool Valid>
  struct StormReflCallableAdditive
  {
    template <typename Deserializer, typename Callable, typename ReturnType, typename ... FuncArgs, typename ... ProvidedArgs>
    static bool CreateCallableAdditive(Deserializer && deserializer, Callable & callable, ReturnType(Callable::*ptr)(FuncArgs...) const, ReturnType * ret_val, ProvidedArgs && ... provided_args)
    {
      return false;
    }
  };

  template <>
  struct StormReflCallableAdditive<true>
  {
    template <typename Deserializer, typename Callable, typename ReturnType, typename FuncArg, typename ProvidedArg, typename ... FuncArgs, typename ... ProvidedArgs>
    static bool CreateCallableAdditive(Deserializer && deserializer, Callable & callable, ReturnType(Callable::*ptr)(FuncArg, FuncArgs...) const, ReturnType * ret_val,
      ProvidedArg && provided_arg, ProvidedArgs && ... provided_args)
    {
      auto func = [&](FuncArgs & ... args)
      {
        return callable(provided_arg, args...);
      };

      return StormReflCreateCallableAdditive(deserializer, func, &decltype(func)::operator(), ret_val, std::forward<ProvidedArgs>(provided_args)...);
    }
  };

  template <bool Valid>
  struct StormReflCallable
  {
    template <typename Deserializer, typename T, typename ReturnType, typename ... FuncArgs, typename ... ProvidedArgs>
    static bool CreateCallable(Deserializer && deserializer, T & t, ReturnType(T::*ptr)(FuncArgs...), ReturnType * ret_val, ProvidedArgs && ... provided_args)
    {
      return false;
    }
  };

  template <>
  struct StormReflCallable<true>
  {
    template <typename Deserializer, typename T, typename ReturnType, typename ... FuncArgs, typename ... ProvidedArgs>
    static bool CreateCallable(Deserializer && deserializer, T & t, ReturnType(T::*ptr)(FuncArgs...), ReturnType * ret_val, ProvidedArgs && ... provided_args)
    {
      auto callable = [&](FuncArgs & ... args)
      {
        return (t.*ptr)(args...);
      };

      return StormReflCreateCallableAdditive(deserializer, callable, &decltype(callable)::operator(), ret_val, std::forward<ProvidedArgs>(provided_args)...);
    }
  };

  template <typename Deserializer, typename T, typename ReturnType, typename ... FuncArgs, typename ... ProvidedArgs>
  bool StormReflCreateCallable(Deserializer && deserializer, T & t, ReturnType (T::*ptr)(FuncArgs...), ReturnType * ret_val, ProvidedArgs && ... provided_args)
  {
    return StormReflCallable<sizeof...(ProvidedArgs) <= sizeof...(FuncArgs)>::CreateCallable(deserializer, t, ptr, ret_val, std::forward<ProvidedArgs>(provided_args)...);
  }
}
