#pragma once

#include <string>
#include <vector>
#include <map>

#include "StormReflJson.h"

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
      StormReflJson<T>::Encode(f.Get(), sb);

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
      StormReflEncodeIndent(indent, sb);
      StormReflJson<T>::EncodePretty(f.Get(), sb, indent + 1);

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
      if (*str != ']' || *str != ',')
      {
        return false;
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
    StormReflEncodeIndent(indent + 1, sb); 
    sb += \"";
    StormReflJson<K>::Encode(itr->first, sb);
    sb += "\":";
    StormReflJson<T>::EncodePretty(itr->second, sb);
    itr++;

    while (itr != t.end())
    {
      sb += ",\n";
      StormReflEncodeIndent(indent + 1, sb);
      sb += \"";
      StormReflJson<K>::Encode(itr->first, sb);
      sb += "\":";
      StormReflJson<T>::EncodePretty(itr->second, sb);
      itr++;
    }

    StormReflEncodeIndent(indent, sb);
    sb += '}';
  }
};

template <>
struct StormReflJson<std::string, void>
{
  template <class StringBuilder>
  static void Encode(const std::string & t, StringBuilder & sb)
  {
    sb += '\"';
    for (auto c : t)
    {
      switch (c)
      {
      case '\"':
        sb += "\\\"";
        break;
      case '\\':
        sb += "\\\\";
        break;
      case '/':
        sb += "\\/";
        break;
      case '\b':
        sb += "\\b";
        break;
      case '\f':
        sb += "\\f";
        break;
      case '\n':
        sb += "\\n";
        break;
      case '\r':
        sb += "\\r";
        break;
      case '\t':
        sb += "\\r";
        break;
      default:
        sb += c;
        break;
      }
    }

    sb += '\"';
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
