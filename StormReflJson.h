#pragma once

#include <cwchar>

#include "StormReflMetaFuncs.h"
#include "StormReflMetaEnum.h"

inline bool StormReflJsonParseOverValue(const char * str, const char *& result);

namespace StormReflJsonHelpers
{
  template <class StringBuilder>
  void StormReflEncodeIndent(int indent, StringBuilder & sb)
  {
    while (indent > 0)
    {
      sb += "  ";
      indent--;
    }
  }
}

inline void StormReflJsonAdvanceWhiteSpace(const char * & str)
{
  char c = *str;
  while (true)
  {
    if (c == ' ' ||
        c == '\n' ||
        c == '\r' ||
        c == '\t')
    {
      str++;
      c = *str;
    }
    else
    {
      return;
    }
  }
}

inline bool StormReflJsonMatchStr(const char * str, const char *& result, const char * match)
{
  while (*match != 0)
  {
    if (*match != *str)
    {
      return false;
    }

    match++;
    str++;
  }

  result = str;
  return true;
}

inline bool StormReflJsonParseStringHash(uint32_t & hash, const char * str, const char *& result)
{
  hash = crc32begin();

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
        hash = crc32additive(hash, '\"');
        break;
      case '\\':
        hash = crc32additive(hash, '\\');
        break;
      case '/':
        hash = crc32additive(hash, '/');
        break;
      case 'b':
        hash = crc32additive(hash, '\b');
        break;
      case 'f':
        hash = crc32additive(hash, '\f');
        break;
      case 'n':
        hash = crc32additive(hash, '\n');
        break;
      case 'r':
        hash = crc32additive(hash, '\r');
        break;
      case 't':
        hash = crc32additive(hash, '\t');
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
            hash = crc32additive(hash, utf8[index]);
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
      hash = crc32end(hash);
      return true;
    }
    else
    {
      hash = crc32additive(hash, *str);
    }

    str++;
  }
}

inline bool StormReflJsonParseOverNumber(const char * str, const char *& result)
{
  StormReflJsonAdvanceWhiteSpace(str);
  const char * start = str;

  if (*str == '-' || *str == '+')
  {
    str++;
  }

  if (*str < '0' || *str > '9')
  {
    return false;
  }

  while (*str >= '0' && *str <= '9')
  {
    str++;
  }

  if (*str == '.')
  {
    while (*str >= '0' && *str <= '9')
    {
      str++;
    }
  }

  if (*str == 'E' || *str == 'e')
  {
    str++;

    while (*str >= '0' && *str <= '9')
    {
      str++;
    }
  }

  result = str;
  return true;
}

inline bool StormReflJsonParseOverString(const char * str, const char *& result)
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
      case '\\':
      case '/':
      case 'b':
      case 'f':
      case 'n':
      case 'r':
      case 't':
      case 'u':
        {
          for (int index = 0; index < 4; index++)
          {
            bool valid = (*str >= '0' && *str <= '9') || (*str >= 'a' && *str <= 'f') || (*str >= 'A' && *str <= 'F');
            if(!valid)
            {
              return false;
            }

            str++;
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
      str++;
    }
  }
}

inline bool StormReflJsonParseOverTrue(const char * str, const char *& result)
{
  return StormReflJsonMatchStr(str, result, "true");
}

inline bool StormReflJsonParseOverFalse(const char * str, const char *& result)
{
  return StormReflJsonMatchStr(str, result, "false");
}

inline bool StormReflJsonParseOverNull(const char * str, const char *& result)
{
  return StormReflJsonMatchStr(str, result, "null");
}

inline bool StormReflJsonParseOverObject(const char * str, const char *& result)
{
  if (*str != '{')
  {
    return false;
  }

  str++;
  while (true)
  {
    StormReflJsonAdvanceWhiteSpace(str);
    if (*str == '}')
    {
      str++;
      result = str;
      return true;
    }

    if (StormReflJsonParseOverString(str, str) == false)
    {
      return false;
    }

    StormReflJsonAdvanceWhiteSpace(str);
    if (*str != ':')
    {
      return false;
    }
    
    str++;

    StormReflJsonAdvanceWhiteSpace(str);
    if (StormReflJsonParseOverValue(str, str) == false)
    {
      return false;
    }

    StormReflJsonAdvanceWhiteSpace(str);
    if (*str != '}')
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

inline bool StormReflJsonParseOverArray(const char * str, const char *& result)
{
  if (*str != '[')
  {
    return false;
  }

  str++;
  StormReflJsonAdvanceWhiteSpace(str);

  while (true)
  {
    if (*str == ']')
    {
      str++;
      result = str;
      return true;
    }

    if (StormReflJsonParseOverValue(str, str) == false)
    {
      return false;
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
        StormReflJsonAdvanceWhiteSpace(str);
      }
    }
  }
}

inline bool StormReflJsonParseOverValue(const char * str, const char *& result)
{
  StormReflJsonAdvanceWhiteSpace(str);
  if (StormReflJsonParseOverNumber(str, result))
  {
    return true;
  }
  if (StormReflJsonParseOverString(str, result))
  {
    return true;
  }
  if (StormReflJsonParseOverTrue(str, result))
  {
    return true;
  }
  if (StormReflJsonParseOverFalse(str, result))
  {
    return true;
  }
  if (StormReflJsonParseOverNull(str, result))
  {
    return true;
  }
  if (StormReflJsonParseOverObject(str, result))
  {
    return true;
  }
  if (StormReflJsonParseOverArray(str, result))
  {
    return true;
  }

  return false;
}

template <class T, class Enable = void>
struct StormReflJson
{
  static bool Parse(T & t, const char * str, const char *& result)
  {
    return false;
  }
};

template <class T, int i>
struct StormReflJson<T[i], void>
{
  template <class StringBuilder>
  static void Encode(const T(&t)[i], StringBuilder & sb)
  {
    sb += '[';

    for (int index = 0; index < i; index++)
    {
      StormReflJson<T>::Encode(t[index], sb);
      if (index < i - 1)
      {
        sb += ',';
      }
    }

    sb += ']';
  }

  template <class StringBuilder>
  static void EncodePretty(const T(&t)[i], StringBuilder & sb, int indent)
  {
    sb += "[\n";

    for (int index = 0; index < i; index++)
    {
      StormReflJsonHelpers::StormReflEncodeIndent(indent + 1, sb);
      StormReflJson<T>::EncodePretty(t[index], sb, indent + 1);
      if (index < i - 1)
      {
        sb += ",\n";
      }
      else
      {
        sb += '\n';
      }
    };

    StormReflJsonHelpers::StormReflEncodeIndent(indent, sb);
    sb += ']';
  }

  static bool Parse(T(&t)[i], const char * str, const char *& result)
  {
    StormReflJsonAdvanceWhiteSpace(str);
    if (*str != '[')
    {
      return false;
    }

    int index = 0;

    str++;
    while (true)
    {
      StormReflJsonAdvanceWhiteSpace(str);

      if (*str == ']')
      {
        str++;
        result = str;
        return true;
      }

      if (index >= i)
      {
        if (StormReflJsonParseOverValue(str, str) == false)
        {
          return false;
        }
      }
      else
      {
        if (StormReflJson<T>::Parse(t[index], str, str) == false)
        {
          return false;
        }
      }

      index++;

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

template <class T>
struct StormReflJson<T, typename std::enable_if<StormReflCheckReflectable<T>::value>::type>
{
  template <class StringBuilder>
  static void Encode(const T & t, StringBuilder & sb)
  {
    sb += '{';
    
    auto field_iterator = [&](auto f)
    {
      sb += '\"';
      sb += f.GetName();
      sb += "\":";

      using member_type = decltype(f)::member_type;

      StormReflJson<member_type>::Encode(f.Get(), sb);
      if (f.GetFieldIndex() < StormReflTypeInfo<T>::fields_n - 1)
      {
        sb += ',';
      }
    };

    StormReflVisitEach(t, field_iterator);
    sb += '}';
  }

  template <class StringBuilder>
  static void EncodePretty(const T & t, StringBuilder & sb, int indent)
  {
    sb += "{\n";

    auto field_iterator = [&](auto f)
    {
      StormReflEncodeIndent(indent + 1, sb);
      sb += '\"';
      sb += f.GetName();
      sb += "\" : ";

      using member_type = decltype(f)::member_type;

      StormReflJson<member_type>::EncodePretty(f.Get(), sb, indent + 1);
      if (f.GetFieldIndex() < StormReflTypeInfo<T>::fields_n - 1)
      {
        sb += ",\n";
      }
      else
      {
        sb += '\n';
      }
    };

    StormReflVisitEach(t, field_iterator);
    StormReflJsonHelpers::StormReflEncodeIndent(indent, sb);
    sb += '}';
  }

  template <class T, class StringBuilder>
  void StormReflEncodeJson(const T & t, StringBuilder & sb)
  {
    StormReflJson<T>::Encode(t, sb);
  }

  static bool Parse(T & t, const char * str, const char *& result)
  {
    StormReflJsonAdvanceWhiteSpace(str);
    if (*str != '{')
    {
      return false;
    }

    str++;
    while (true)
    {
      StormReflJsonAdvanceWhiteSpace(str);

      if (*str == '}')
      {
        str++;
        result = str;
        return true;
      }

      Hash field_name_hash;
      if (StormReflJsonParseStringHash(field_name_hash, str, str) == false)
      {
        return false;
      }

      StormReflJsonAdvanceWhiteSpace(str);
      if (*str != ':')
      {
        return false;
      }

      str++;

      StormReflJsonAdvanceWhiteSpace(str);
      bool parsed_field = false;
      const char * result_str = str;

      auto field_visitor = [&](auto f)
      {
        using member_type = decltype(f)::member_type;
        member_type & member = f.Get();
        parsed_field = StormReflJson<member_type>::Parse(member, str, result_str);
      };

      StormReflVisitField(t, field_visitor, field_name_hash);
      if (parsed_field)
      {
        str = result_str;
      }
      else
      {
        if (StormReflJsonParseOverValue(str, str) == false)
        {
          return false;
        }
      }

      StormReflJsonAdvanceWhiteSpace(str);
      if (*str != '}')
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

template <class T>
struct StormReflJson<T, typename std::enable_if<std::is_enum<T>::value>::type>
{
  template <class StringBuilder>
  static void Encode(const T & t, StringBuilder & sb)
  {
    sb += '\"';
    sb += StormReflGetEnumAsString(t);
    sb += '\"';
  }

  template <class StringBuilder>
  static void EncodePretty(const T & t, StringBuilder & sb, int indent)
  {
    Encode(t, sb);
  }

  static bool Parse(T & t, const char * str, const char *& result)
  {
    StormReflJsonAdvanceWhiteSpace(str);
    if (*str != '\"')
    {
      return false;
    }

    str++;
    uint32_t hash = crc32begin();

    while (*str != '\"')
    {
      if (*str == 0)
      {
        return false;
      }

      hash = crc32additive(hash, *str);
      str++;
    }

    hash = crc32end(hash);
    str++;
    result = str;

    return StormReflGetEnumFromHash(t, hash);
  }
};

template <class IntType, class ParseType>
static IntType StormReflParseDigits(const char * & str, bool negative)
{
  if (negative && std::is_unsigned<ParseType>::value)
  {
    while (*str >= 0 && *str <= 9)
    {
      str++;
    }

    return 0;
  }

  IntType val = 0;
  IntType max_val = std::numeric_limits<ParseType>::max() + (negative ? 1 : 0);
  IntType max_pre_digit = (max_val / 10) + 1;

  bool overflow = false;

  while (*str >= '0' && *str <= '9')
  {
    if (val > max_pre_digit)
    {
      overflow = true;
      break;
    }

    val *= 10;

    IntType add_val = (*str) - '0';
    if (val > max_val - add_val)
    {
      overflow = true;
      break;
    }

    val += add_val;
    str++;
  }

  if (overflow)
  {
    while (*str >= '0' && *str <= '9')
    {
      str++;
    }

    return max_val;
  }

  return val;
}

template <class T, class IntType>
static IntType StormReflApplyExponent(IntType val, int8_t exp, bool negative, const char * fractional_str)
{
  if (exp == 1)
  {
    return val;
  }

  if (exp == 0)
  {
    return 1;
  }

  if (exp < 0)
  {
    return 0;
  }

  IntType max_val = std::numeric_limits<T>::max() + (negative ? 1 : 0);
  IntType max_pre_digit = (max_val / 10) + 1;

  bool overflow = false;
  while (exp > 0)
  {
    if (val > max_pre_digit)
    {
      overflow = true;
      break;
    }

    val *= 10;

    IntType add_val;
    if (fractional_str != nullptr && *fractional_str >= '0' && *fractional_str <= '9')
    {
      add_val = *fractional_str - '0';
      fractional_str++;

      if (val > max_val - add_val)
      {
        overflow = true;
        break;
      }

      val += add_val;
    }

    exp--;
  }

  if (overflow)
  {
    return max_val;
  }

  return val;
}


template <class T>
struct StormReflJson<T, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type>
{
  template <class StringBuilder>
  static void Encode(const T & t, StringBuilder & sb)
  {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%llu", (uint64_t)t);
    sb += buffer;
  }

  template <class StringBuilder>
  static void EncodePretty(const T & t, StringBuilder & sb, int indent)
  {
    Encode(t, sb);
  }

  static bool Parse(T & t, const char * str, const char *& result)
  {
    StormReflJsonAdvanceWhiteSpace(str);
    bool negative;
    int flow = 0;

    if (*str == '-')
    {
      negative = true;
      str++;
    }
    else if (*str == '+')
    {
      negative = false;
      str++;
    }
    else
    {
      negative = false;
    }

    if (*str < '0' || *str > '9')
    {
      return false;
    }

    while (*str == '0')
    {
      str++;
    }

    T val = StormReflParseDigits<T, T>(str, negative);
    const char * fractional_part = nullptr;

    if (*str == '.')
    {
      str++;
      fractional_part = str;

      while (*str >= '0' && *str <= '9')
      {
        str++;
      }
    }

    int8_t exp = 1;
    if (*str == 'E' || *str == 'e')
    {
      str++;

      bool exp_negative = false;
      if (*str == '-')
      {
        str++;
        exp_negative = true;
      }
      else if (*str == '+')
      {
        str++;
      }

      if (*str < '0' || *str > '9')
      {
        return false;
      }

      uint8_t uexp = StormReflParseDigits<uint8_t, int8_t>(str, exp_negative);
      exp = static_cast<int8_t>(exp_negative ? -uexp : uexp);
    }

    result = str;
    if (exp < 0)
    {
      t = 0;
      return true;
    }
    else if (exp > 1)
    {
      val = StormReflApplyExponent<T, T>(val, exp, negative, fractional_part);
    }

    t = negative ? 0 : val;
    return true;
  }
};

template <class T>
struct StormReflJson<T, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
{
  template <class StringBuilder>
  static void Encode(const T & t, StringBuilder & sb)
  {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%lli", (int64_t)t);
    sb += buffer;
  }

  template <class StringBuilder>
  static void EncodePretty(const T & t, StringBuilder & sb, int indent)
  {
    Encode(t, sb);
  }

  static bool Parse(T & t, const char * str, const char *& result)
  {
    using IntType = std::make_unsigned_t<T>;

    StormReflJsonAdvanceWhiteSpace(str);
    bool negative;
    int flow = 0;

    if (*str == '-')
    {
      negative = true;
      str++;
    }
    else if (*str == '+')
    {
      negative = false;
      str++;
    }
    else
    {
      negative = false;
    }

    if (*str < '0' || *str > '9')
    {
      return false;
    }

    while (*str == '0')
    {
      str++;
    }

    IntType val = StormReflParseDigits<IntType, T>(str, negative);
    const char * fractional_part = nullptr;

    if (*str == '.')
    {
      str++;
      fractional_part = str;

      while (*str >= '0' && *str <= '9')
      {
        str++;
      }
    }

    int8_t exp = 1;
    if (*str == 'E' || *str == 'e')
    {
      str++;

      bool exp_negative = false;
      if (*str == '-')
      {
        str++;
        exp_negative = true;
      }
      else if (*str == '+')
      {
        str++;
      }

      if (*str < '0' || *str > '9')
      {
        return false;
      }

      uint8_t uexp = StormReflParseDigits<uint8_t, int8_t>(str, exp_negative);
      exp = static_cast<int8_t>(exp_negative ? -uexp : uexp);
    }

    result = str;
    if (exp < 0)
    {
      t = 0;
      return true;
    }
    else if (exp > 1)
    {
      val = StormReflApplyExponent<T, IntType>(val, exp, negative, fractional_part);
    }

    t = negative ? -static_cast<T>(val) : static_cast<T>(val);
    return true;
  }
};

template <class T>
struct StormReflJson<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
{
  template <class StringBuilder>
  static void Encode(const T & t, StringBuilder & sb)
  {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%f", t);

    sb += buffer;
  }

  template <class StringBuilder>
  static void EncodePretty(const T & t, StringBuilder & sb, int indent)
  {
    Encode(t, sb);
  }

  static bool Parse(T & t, const char * str, const char *& result)
  {
    StormReflJsonAdvanceWhiteSpace(str);
    const char * start = str;

    if (*str == '-' || *str == '+')
    {
      str++;
    }

    if (*str < '0' || *str > '9')
    {
      return false;
    }

    while (*str >= '0' && *str <= '9')
    {
      str++;
    }

    if (*str == '.')
    {
      str++;
      while (*str >= '0' && *str <= '9')
      {
        str++;
      }
    }

    if (*str == 'E' || *str == 'e')
    {
      str++;

      while (*str >= '0' && *str <= '9')
      {
        str++;
      }
    }

    char * bs;
    t = static_cast<T>(strtod(start, &bs));
    result = str;

    return true;
  }
};

template <>
struct StormReflJson<bool, void>
{
  template <class StringBuilder>
  static void Encode(const bool & t, StringBuilder & sb)
  {
    sb += t ? "true" : "false";
  }

  template <class StringBuilder>
  static void EncodePretty(const bool & t, StringBuilder & sb, int indent)
  {
    Encode(t, sb);
  }

  static bool Parse(bool & t, const char * str, const char *& result)
  {
    if (StormReflJsonMatchStr(str, result, "true"))
    {
      t = true;
      return true;
    }
    else if (StormReflJsonMatchStr(str, result, "false"))
    {
      t = false;
      return false;
    }
    else
    {
      return false;
    }
  }
};

template <class T, class StringBuilder>
void StormReflEncodeJson(const T & t, StringBuilder & sb)
{
  StormReflJson<T>::Encode(t, sb);
}

template <class T, class StringBuilder>
void StormReflEncodePrettyJson(const T & t, StringBuilder & sb)
{
  StormReflJson<T>::EncodePretty(t, sb, 0);
}

template <class T>
bool StormReflParseJson(T & t, const char * str, const char *& result)
{
  return StormReflJson<T>::Parse(t, str, result);
}

template <class T>
bool StormReflParseJson(T & t, const char * str)
{
  return StormReflJson<T>::Parse(t, str, str);
}

