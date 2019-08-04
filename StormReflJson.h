#pragma once

#include <cwchar>
#include <climits>
#include <cstdlib>
#include <cinttypes>

#include <vector>

#include "StormReflMetaFuncs.h"
#include "StormReflMetaEnum.h"

#pragma warning(disable:4996)

struct StormReflStringViewProxy;

template <typename CharPtr>
inline bool StormReflJsonParseOverValue(CharPtr str, CharPtr & result);


template <class T>
bool StormReflParseJson(T & t, const char * str, const char *& result, bool additive = false);

template <class T>
bool StormReflParseJson(T & t, const char * str, bool additive = false);

template <class T>
bool StormReflParseJson(T & t, const std::string_view & str, std::string_view & result, bool additive = false);

template <class T>
bool StormReflParseJson(T & t, const std::string_view & str, bool additive = false);

template <class T>
bool StormReflParseJson(T & t, StormReflStringViewProxy str, StormReflStringViewProxy & result, bool additive = false);

template <class T>
bool StormReflParseJson(T & t, StormReflStringViewProxy str, bool additive = false);

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

template <typename CharPtr>
inline void StormReflJsonAdvanceWhiteSpace(CharPtr & str)
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

template <typename CharPtr>
inline bool StormReflJsonMatchStr(CharPtr str, CharPtr& result, const char * match)
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

template <typename CharPtr>
inline bool StormReflJsonParseStringHash(uint32_t & hash, CharPtr str, CharPtr& result)
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

          std::size_t len = wcrtomb(utf8, val, &state);
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

template <typename CharPtr>
inline bool StormReflJsonParseOverNumber(CharPtr str, CharPtr& result)
{
  StormReflJsonAdvanceWhiteSpace(str);
  auto start = str;

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
    if (*str == '+' || *str == '-')
    {
      str++;
    }

    while (*str >= '0' && *str <= '9')
    {
      str++;
    }
  }

  result = str;
  return true;
}

template <typename CharPtr>
inline bool StormReflJsonParseOverString(CharPtr str, CharPtr& result)
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
        str++;
        break;
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

template <typename CharPtr>
inline bool StormReflJsonParseOverTrue(CharPtr str, CharPtr& result)
{
  return StormReflJsonMatchStr(str, result, "true");
}

template <typename CharPtr>
inline bool StormReflJsonParseOverFalse(CharPtr str, CharPtr& result)
{
  return StormReflJsonMatchStr(str, result, "false");
}

template <typename CharPtr>
inline bool StormReflJsonParseOverNull(CharPtr str, CharPtr& result)
{
  return StormReflJsonMatchStr(str, result, "null");
}

template <typename CharPtr>
inline bool StormReflJsonParseOverObject(CharPtr str, CharPtr& result)
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

template <typename CharPtr>
inline bool StormReflJsonParseOverArray(CharPtr str, CharPtr& result)
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

template <typename CharPtr>
inline bool StormReflJsonParseOverValue(CharPtr str, CharPtr& result)
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

template <class StringBuilder>
void StormReflJsonEncodeString(const char * str, StringBuilder & sb)
{
  sb += '\"';
  while (*str != 0)
  {
    switch (*str)
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
      sb += *str;
      break;
    }

    str++;
  }

  sb += '\"';
}



template <class T, class Enable = void>
struct StormReflJson
{

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

  template <class StringBuilder>
  static void SerializeDefault(StringBuilder & sb)
  {
    sb += '[';

    for (int index = 0; index < i; index++)
    {
      StormReflJson<T>::SerializeDefault(sb);
      if (index != i - 1)
      {
        sb += ',';
      }
    }

    sb += ']';
  }

  template <typename CharPtr>
  static bool Parse(T(&t)[i], CharPtr str, CharPtr& result, bool additive)
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
        if (StormReflJson<T>::Parse(t[index], str, str, additive) == false)
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

template <int i>
struct StormReflJson<char[i], void>
{
  template <class StringBuilder>
  static void Encode(const char * str, StringBuilder & sb)
  {
    StormReflJsonEncodeString(str, sb);
  }

  template <class StringBuilder>
  static void EncodePretty(const char * str, StringBuilder & sb, int indent)
  {
    StormReflJsonEncodeString(str, sb);
  }

  template <class StringBuilder>
  static void SerializeDefault(StringBuilder & sb)
  {
    sb += "\"\"";
  }
};

template <>
struct StormReflJson<char *, void>
{
  template <class StringBuilder>
  static void Encode(const char * str, StringBuilder & sb)
  {
    StormReflJsonEncodeString(str, sb);
  }

  template <class StringBuilder>
  static void EncodePretty(const char * str, StringBuilder & sb, int indent)
  {
    StormReflJsonEncodeString(str, sb);
  }

  template <class StringBuilder>
  static void SerializeDefault(StringBuilder & sb)
  {
    sb += "\"\"";
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

      using member_type = typename decltype(f)::member_type;

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
      StormReflJsonHelpers::StormReflEncodeIndent(indent + 1, sb);
      sb += '\"';
      sb += f.GetName();
      sb += "\" : ";

      using member_type = typename decltype(f)::member_type;

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

  template <class StringBuilder>
  static void SerializeDefault(StringBuilder & sb)
  {
    sb += "{}";
  }

  template <class StringBuilder>
  void StormReflEncodeJson(const T & t, StringBuilder & sb)
  {
    StormReflJson<T>::Encode(t, sb);
  }

  template <typename CharPtr>
  static bool Parse(T & t, CharPtr str, CharPtr& result, bool additive)
  {
    StormReflJsonAdvanceWhiteSpace(str);
    if (*str != '{')
    {
      return false;
    }

    std::vector<uint32_t> parsed_fields;

    str++;
    while (true)
    {
      StormReflJsonAdvanceWhiteSpace(str);

      if (*str == '}')
      {
        str++;
        result = str;

        if (additive == false)
        {
          auto field_visitor = [&](auto f)
          {
            bool set_field = false;
            for (auto & elem : parsed_fields)
            {
              if (elem == f.GetFieldNameHash())
              {
                set_field = true;
                break;
              }
            }

            if (set_field == false)
            {
              using FieldType = decltype(f);
              if constexpr(StormReflHasDefault<T>::value && FieldType::HasDefault())
              {
                f.SetDefault();
              }
            }
          };

          StormReflVisitEach(t, field_visitor);
        }

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
      CharPtr result_str = str;

      auto field_visitor = [&](auto f)
      {
        auto & member = f.Get();
        parsed_field = StormReflParseJson(member, str, result_str, additive);
      };

      StormReflVisitField(t, field_visitor, field_name_hash);
      if (parsed_field)
      {
        if (additive == false)
        {
          parsed_fields.push_back(field_name_hash);
        }

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

  template <class StringBuilder>
  static void SerializeDefault(StringBuilder & sb)
  {
    sb += "\"\"";
  }

  template <typename CharPtr>
  static bool Parse(T & t, CharPtr str, CharPtr& result, bool additive)
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

template <class IntType, class ParseType, typename CharPtr>
static IntType StormReflParseDigits(CharPtr & str, bool negative)
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

template <class T, class IntType, typename CharPtr>
static IntType StormReflApplyExponent(IntType val, int8_t exp, bool negative, CharPtr fractional_str)
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
    if (fractional_str && *fractional_str >= '0' && *fractional_str <= '9')
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
    snprintf(buffer, sizeof(buffer), "%" PRIu64, (uint64_t)t);
    if (sizeof(T) > 4)
    {
      sb += '\"';
    }

    sb += buffer;
    if (sizeof(T) > 4)
    {
      sb += '\"';
    }
  }

  template <class StringBuilder>
  static void EncodePretty(const T & t, StringBuilder & sb, int indent)
  {
    Encode(t, sb);
  }

  template <class StringBuilder>
  static void SerializeDefault(StringBuilder & sb)
  {
    sb += "0";
  }

  template <typename CharPtr>
  static bool Parse(T & t, CharPtr str, CharPtr& result, bool additive)
  {
    StormReflJsonAdvanceWhiteSpace(str);

    bool is_string = false;
    if (*str == '\"')
    {
      str++;
      is_string = true;
    }

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
    CharPtr fractional_part = nullptr;

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

    if (is_string)
    {
      if (*str != '\"')
      {
        return false;
      }

      str++;
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
    snprintf(buffer, sizeof(buffer), "%lli", (long long)t);

    if (sizeof(T) > 4)
    {
      sb += '\"';
    }

    sb += buffer;

    if (sizeof(T) > 4)
    {
      sb += '\"';
    }
  }

  template <class StringBuilder>
  static void EncodePretty(const T & t, StringBuilder & sb, int indent)
  {
    Encode(t, sb);
  }

  template <class StringBuilder>
  static void SerializeDefault(StringBuilder & sb)
  {
    sb += "0";
  }

  template <typename CharPtr>
  static bool Parse(T & t, CharPtr str, CharPtr& result, bool additive)
  {
    using IntType = std::make_unsigned_t<T>;

    StormReflJsonAdvanceWhiteSpace(str);

    bool is_string = false;
    if (*str == '\"')
    {
      str++;
      is_string = true;
    }

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
    CharPtr fractional_part = nullptr;

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

    if (is_string)
    {
      if (*str != '\"')
      {
        return false;
      }

      str++;
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

  template <typename CharPtr>
  static bool Parse(T & t, CharPtr str, CharPtr& result, bool additive)
  {
    StormReflJsonAdvanceWhiteSpace(str);
    auto start = str;

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

  template <class StringBuilder>
  static void SerializeDefault(StringBuilder & sb)
  {
    sb += "0";
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

  template <class StringBuilder>
  static void SerializeDefault(StringBuilder & sb)
  {
    sb += "false";
  }

  template <typename CharPtr>
  static bool Parse(bool & t, CharPtr str, CharPtr& result, bool additive)
  {
    if (StormReflJsonMatchStr(str, result, "true"))
    {
      t = true;
      return true;
    }
    else if (StormReflJsonMatchStr(str, result, "false"))
    {
      t = false;
      return true;
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

template <class T, class StringBuilder>
void StormReflEncodePrettyJson(const T & t, StringBuilder & sb, int indent)
{
  StormReflJson<T>::EncodePretty(t, sb, indent);
}

template <class T, class StringBuilder>
void StormReflSerializeDefaultJson(const T & t, StringBuilder & sb)
{
  StormReflJson<T>::SerializeDefault(sb);
}

template <class T, class Meta, class StringBuilder>
void StormReflEncodeJsonWithMetaData(const T & t, const Meta & meta, StringBuilder & sb)
{
  static_assert(StormReflCheckReflectable<T>::value, "Can only append meta data to reflectable objects");

  sb += "{\"__meta\":";
  StormReflEncodeJson(meta, sb);

  auto field_iterator = [&](auto f)
  {      
    sb += ',';
    sb += '\"';
    sb += f.GetName();
    sb += "\":";

    using member_type = typename decltype(f)::member_type;

    StormReflJson<member_type>::Encode(f.Get(), sb);
  };

  StormReflVisitEach(t, field_iterator);
  sb += '}';

}

struct StormReflStringViewProxy
{
  const char * start;
  const char * end;

  StormReflStringViewProxy()
  {
    start = nullptr;
    end = nullptr;
  }

  StormReflStringViewProxy(const char * ptr)
  {
    if(ptr)
    {
      start = ptr;
      end = ptr + strlen(ptr);
    }
    else
    {
      start = nullptr;
      end = nullptr;
    }
  }

  StormReflStringViewProxy(const char * s, const char * e)
  {
    start = s;
    end = e;
  }

  StormReflStringViewProxy(const StormReflStringViewProxy & rhs) = default;
  StormReflStringViewProxy(StormReflStringViewProxy && rhs) = default;
  StormReflStringViewProxy & operator = (const StormReflStringViewProxy & rhs) = default;
  StormReflStringViewProxy & operator = (StormReflStringViewProxy && rhs) = default;

  char operator *() const
  {
    return start != end ? *start : 0;
  }

  StormReflStringViewProxy & operator++()
  {
    if(start != end)
    {
      ++start;
    }
    return *this;
  }

  StormReflStringViewProxy operator++(int)
  {
    auto tmp = *this;
    if(start != end)
    {
      ++start;
    }
    return tmp;
  }

  operator bool() const
  {
    return start != nullptr;
  }
};

template <class T>
bool StormReflParseJson(T & t, const char * str, const char *& result, bool additive)
{
  return StormReflJson<T>::Parse(t, str, result, additive);
}

template <class T>
bool StormReflParseJson(T & t, const char * str, bool additive)
{
  return StormReflJson<T>::Parse(t, str, str, additive);
}

template <class T>
bool StormReflParseJson(T & t, const std::string_view & str, std::string_view & result, bool additive)
{
  StormReflStringViewProxy proxy(str.data(), str.data() + str.length());
  StormReflStringViewProxy out;

  if(StormReflJson<T>::Parse(t, proxy, out, additive))
  {
    result = std::string_view(out.start, out.end - out.start);
    return true;
  }

  return false;
}

template <class T>
bool StormReflParseJson(T & t, const std::string_view & str, bool additive)
{
  StormReflStringViewProxy proxy(str.data(), str.data() + str.length());
  StormReflStringViewProxy out;

  if(StormReflJson<T>::Parse(t, proxy, out, additive))
  {
    return true;
  }

  return false;
}


template <class T>
bool StormReflParseJson(T & t, StormReflStringViewProxy str, StormReflStringViewProxy & result, bool additive)
{
  return StormReflJson<T>::Parse(t, str, result, additive);
}

template <class T>
bool StormReflParseJson(T & t, StormReflStringViewProxy str, bool additive)
{
  return StormReflJson<T>::Parse(t, str, str, additive);
}


