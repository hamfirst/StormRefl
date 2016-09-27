#pragma once

#include <string>
#include <vector>

struct ReflectedParam
{
  std::string m_Name;
  std::string m_Type;
};

struct ReflectedFunc
{
  std::string m_Name;
  std::string m_ReturnType;

  std::vector<ReflectedParam> m_Params;
};

struct ReflectedField
{
  std::string m_Name;
  std::string m_Type;

  std::vector<std::string> m_Attrs;
};

struct ReflectedDataClass
{
  std::string m_Name;
  std::string m_Base;
  std::vector<ReflectedField> m_Fields;
};

struct ReflectedFunctionalClass
{
  std::string m_Name;
  std::string m_Base;
  std::vector<ReflectedFunc> m_Funcs;
};

struct ReflectedEnumElem
{
  std::string m_Name;
  std::string m_FullName;
};

struct ReflectedEnum
{
  std::string m_Name;
  bool m_Scoped;
  std::vector<ReflectedEnumElem> m_Elems;
};

void OutputReflectedFile(const std::string & filename, const std::vector<ReflectedFunctionalClass> & class_funcs, 
                                                       const std::vector<ReflectedDataClass> & class_data, 
                                                       const std::vector<ReflectedEnum> & enum_data,
                                                       const std::vector<std::string> & headers);
