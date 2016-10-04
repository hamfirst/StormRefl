#pragma once

#include <string>
#include <vector>
#include <map>
#include <tuple>

#include "StormReflJson.h"

template <class T>
std::string StormReflEncodeJson(const T & t)
{
  std::string sb;
  StormReflJson<T>::Encode(t, sb);
  return sb;
}

template <class T>
std::string StormReflEncodePrettyJson(const T & t)
{
  std::string sb;
  StormReflJson<T>::EncodePretty(t, sb, 0);
  return sb;
}

template <class T>
struct StormReflJson<std::vector<T>, void>
{
  template <class StringBuilder>
  static void Encode(const std::vector<T> & t, StringBuilder & sb)
  {
    sb += '[';

    std::size_t size = t.size();
    for(std::size_t index = 0; index < size; index++)
    {
      StormReflJson<T>::Encode(t[index], sb);

      if (index < size - 1)
      {
        sb += ',';
      }
    }

    sb += ']';
  }

  template <class StringBuilder>
  static void EncodePretty(const std::vector<T> & t, StringBuilder & sb, int indent)
  {
    sb += "[\n";

    std::size_t size = t.size();
    for (std::size_t index = 0; index < size; index++)
    {
      StormReflJsonHelpers::StormReflEncodeIndent(indent, sb);
      StormReflJson<T>::EncodePretty(t[index], sb, indent + 1);

      if (index < size - 1)
      {
        sb += ",\n";
      }
      else
      {
        sb += '\n';
      }
    }

    sb += " ]";
  }

  static bool Parse(std::vector<T> & t, const char * str, const char *& result)
  {
    if (*str != '[')
    {
      return false;
    }

    str++;
    while (true)
    {
      if (*str == ']')
      {
        str++;
        result = str;
        return true;
      }

      t.emplace_back();
      auto & elem = t.back();

      if (StormReflJson<T>::Parse(elem, str, str) == false)
      {
        if (StormReflJsonParseOverValue(str, str) == false)
        {
          return false;
        }
      }

      StormReflJsonAdvanceWhiteSpace(str);
      if (*str != ']')
      {
        if (*str != ',')
        {
          return false;
        }
        else
        {
          str++;
        }
      }
    }
  }
};

template <class K, class T>
struct StormReflJson<std::map<K, T>, void>
{
  template <class StringBuilder>
  static void Encode(std::map<K, T> & t, StringBuilder & sb)
  {
    if (t.size() == 0)
    {
      sb += "{}";
      return;
    }

    auto itr = t.begin();
    sb += "{\"";
    StormReflJson<K>::Encode(itr->first, sb);
    sb += "\":";
    StormReflJson<T>::Encode(itr->second, sb);
    itr++;

    while(itr != t.end())
    {
      sb += ",\"";
      StormReflJson<K>::Encode(itr->first, sb);
      sb += "\":";
      StormReflJson<T>::Encode(itr->second, sb);
      itr++;
    }

    sb += '}';
  }

  template <class StringBuilder>
  static void EncodePretty(std::map<K, T> & t, StringBuilder & sb, int indent)
  {
    if (t.size() == 0)
    {
      sb += "{}\n";
      return;
    }

    auto itr = t.begin();
    sb += "{\n";
    StormReflJsonHelpers::StormReflEncodeIndent(indent + 1, sb);
    sb += '\"';
    StormReflJson<K>::Encode(itr->first, sb);
    sb += "\":";
    StormReflJson<T>::EncodePretty(itr->second, sb);
    itr++;

    while (itr != t.end())
    {
      sb += ",\n";
      StormReflJsonHelpers::StormReflEncodeIndent(indent + 1, sb);
      sb += '\"';
      StormReflJson<K>::Encode(itr->first, sb);
      sb += "\":";
      StormReflJson<T>::EncodePretty(itr->second, sb);
      itr++;
    }

    StormReflJsonHelpers::StormReflEncodeIndent(indent, sb);
    sb += '}';
  }
};

template <>
struct StormReflJson<std::string, void>
{
  template <class StringBuilder>
  static void Encode(const std::string & t, StringBuilder & sb)
  {
    StormReflJsonEncodeString(t.c_str(), sb);
  }

  template <class StringBuilder>
  static void EncodePretty(const std::string & t, StringBuilder & sb, int indent)
  {
    Encode(t, sb);
  }

  static bool Parse(std::string & t, const char * str, const char *& result)
  {
    StormReflJsonAdvanceWhiteSpace(str);
    if (*str != '\"')
    {
      return false;
    }

    str++;
    while (true)
    {
      if (*str == '\\')
      {
        str++;
        switch (*str)
        {
        default:
          return false;
        case '\"':
          t.push_back('\"');
          str++;
          break;
        case '\\':
          t.push_back('\\');
          str++;
          break;
        case '/':
          t.push_back('/');
          str++;
          break;
        case 'b':
          t.push_back('\b');
          str++;
          break;
        case 'f':
          t.push_back('\f');
          str++;
          break;
        case 'n':
          t.push_back('\n');
          str++;
          break;
        case 'r':
          t.push_back('\r');
          str++;
        case 't':
          t.push_back('\t');
          break;
        case 'u':
        {
          wchar_t val = 0;
          for (int index = 0; index < 4; index++)
          {
            val <<= 4;
            if (*str >= '0' && *str <= '9')
            {
              val += *str - '0';
            }
            else if (*str >= 'a' && *str <= 'f')
            {
              val += *str - 'a' + 10;
            }
            else if (*str >= 'A' && *str <= 'F')
            {
              val += *str - 'A' + 10;
            }
            else
            {
              return false;
            }
          }

          char utf8[MB_LEN_MAX];
          std::mbstate_t state{};

          std::size_t len;
          wcrtomb_s(&len, utf8, val, &state);
          for (std::size_t index = 0; index < len; index++)
          {
            t.push_back(utf8[index]);
          }
        }
        break;
        }
      }
      else if (*str == 0 || *str == '\n' || *str == '\r' || *str == '\b' || *str == '\t')
      {
        return false;
      }
      else if (*str == '\"')
      {
        str++;
        result = str;
        return true;
      }
      else
      {
        t.push_back(*str);
        str++;
      }
    }
  }
};

template <class First, class Second>
struct StormReflJson<std::pair<First, Second>, void>
{
  template <class StringBuilder>
  static void Encode(const std::pair<First, Second> & t, StringBuilder & sb)
  {
    sb += '[';
    StormReflJson<First>::Encode(t.first, sb);
    sb += ',';
    StormReflJson<Second>::Encode(t.second, sb);
    sb += ']';
  }

  template <class StringBuilder>
  static void EncodePretty(const std::pair<First, Second> & t, StringBuilder & sb, int indent)
  {
    sb += "[ ";
    StormReflJson<First>::Encode(t.first, sb);
    sb += ", ";
    StormReflJson<Second>::Encode(t.second, sb);
    sb += " ]";
  }

  static bool Parse(std::pair<First, Second> & t, const char * str, const char *& result)
  {
    StormReflJsonAdvanceWhiteSpace(str);
    if (*str != '[')
    {
      return false;
    }

    str++;

    if (StormReflJson<First>::Parse(t.first, str, str) == false)
    {
      return false;
    }

    StormReflJsonAdvanceWhiteSpace(str);
    if (*str != ',')
    {
      return false;
    }

    str++;

    if (StormReflJson<Second>::Parse(t.second, str, str) == false)
    {
      return false;
    }

    StormReflJsonAdvanceWhiteSpace(str);
    if (*str != ']')
    {
      return false;
    }

    str++;
    result = str;
    return true;
  }
};

namespace StormReflJsonHelpers
{
  template <int N, typename... Types>
  struct TupleArg
  {
    static constexpr int ElemIndex = sizeof...(Types)-N;

    template <class StringBuilder>
    static void Encode(const std::tuple<Types...> & t, StringBuilder & sb)
    {
      using elem_type = std::remove_const_t<std::remove_reference_t<decltype(std::get<ElemIndex>(t))>>;
      StormReflJson<elem_type>::Encode(std::get<ElemIndex>(t), sb);

      if (N > 0)
      {
        sb += ',';
      }

      TupleArg<N - 1>::Encode(t, sb);
    }

    static bool Parse(std::tuple<Types...> & t, const char * str, const char *& result)
    {
      using elem_type = std::remove_const_t<std::remove_reference_t<decltype(std::get<ElemIndex>(t))>>;
      if (StormReflJson<elem_type>::Parse(std::get<ElemIndex>(t), str, str) == false)
      {
        return false;
      }

      if (N > 0)
      {
        StormReflJsonAdvanceWhiteSpace(str);
        if (*str != ',')
        {
          return false;
        }

        str++;
      }

      return TupleArg<N - 1>::Parse(t, str, result);
    }
  };

  template <typename... Types>
  struct TupleArg<0, Types...>
  {
    template <class StringBuilder>
    static void Encode(const std::tuple<Types...> & t, StringBuilder & sb)
    {

    }

    static bool Parse(std::tuple<Types...> & t, const char * str, const char *& result)
    {
      result = str;
      return true;
    }
  };
}

template <class ... Types>
struct StormReflJson<std::tuple<Types...>, void>
{
  template <class StringBuilder>
  static void Encode(const std::tuple<Types...> & t, StringBuilder & sb)
  {
    sb += '[';
    StormReflJsonHelpers::TupleArg<0>::Encode(t, sb);
    sb += ']';
  }

  template <class StringBuilder>
  static void EncodePretty(const std::tuple<Types...> & t, StringBuilder & sb, int indent)
  {
    sb += "[ ";
    StormReflJsonHelpers::TupleArg<sizeof...(Types)>::Encode(t, sb);
    sb += " ]";
  }

  static bool Parse(std::tuple<Types...> & t, const char * str, const char *& result)
  {
    StormReflJsonAdvanceWhiteSpace(str);
    if (*str != '[')
    {
      return false;
    }

    str++;

    if (StormReflJsonHelpers::TupleArg<sizeof...(Types)>::Parse(t, str, str) == false)
    {
      return false;
    }

    StormReflJsonAdvanceWhiteSpace(str);
    if (*str != ']')
    {
      return false;
    }

    str++;
    result = str;
    return true;
  }
};
