#pragma once

#include <string>
#include <vector>

struct ReflectedField
{
  std::string m_Name;
  std::string m_Type;

  std::vector<std::string> m_Attrs;
};

struct ReflectedClass
{
  std::string m_Name;
  std::string m_Base;
  std::vector<ReflectedField> m_Fields;
};

void OutputReflectedFile(const std::string & filename, const std::vector<ReflectedClass> & class_data, const std::vector<std::string> & headers);
