
#include "StormReflOutput.h"

#include <experimental\filesystem>

#include <hash\Hash.h>

namespace fs = std::experimental::filesystem;

fs::path ConvertFileToMeta(const std::string & filename)
{
  fs::path path(filename);

  auto parent_path = path.parent_path();
  auto stem = path.stem();
  auto extension = path.extension();

  auto new_path = parent_path;
  new_path /= stem;
  new_path += ".meta";
  new_path += extension;

  return new_path;
}

fs::path RelativePath(const fs::path &path, const fs::path &relative_to)
{
  // create absolute paths
  fs::path p = fs::absolute(path);
  fs::path r = fs::absolute(relative_to);

  // if root paths are different, return absolute path
  if (p.root_path() != r.root_path())
    return p;

  // initialize relative path
  fs::path result;

  // find out where the two paths diverge
  fs::path::const_iterator itr_path = p.begin();
  fs::path::const_iterator itr_relative_to = r.begin();
  while (*itr_path == *itr_relative_to && itr_path != p.end() && itr_relative_to != r.end()) {
    ++itr_path;
    ++itr_relative_to;
  }

  // add "../" for each remaining token in relative_to
  if (itr_relative_to != r.end()) {
    ++itr_relative_to;
    while (itr_relative_to != r.end()) {
      result /= "..";
      ++itr_relative_to;
    }
  }

  // add remaining path
  while (itr_path != p.end()) {
    result /= *itr_path;
    ++itr_path;
  }

  return result;
}

void OutputReflectedFile(const std::string & filename, const std::vector<ReflectedFunctionalClass> & class_funcs, 
                                                       const std::vector<ReflectedDataClass> & class_data,
                                                       const std::vector<std::string> & headers)
{
  auto cur_path = fs::canonical(fs::path(filename));
  auto new_path = ConvertFileToMeta(filename);
  auto fp = _wfopen(new_path.c_str(), L"wt");
  if (!fp)
  {
    return;
  }

  fprintf(fp, "#pragma once\n\n");
  fprintf(fp, "#include <StormRefl\\StormReflMetaInfoBase.h>\n\n");

  fprintf(fp, "#include \"%s\"\n", cur_path.filename().u8string().c_str());
  for (auto & header : headers)
  {
    if (header.find(".refl.") != std::string::npos)
    {
      fprintf(fp, "#include \"%s\"\n", ConvertFileToMeta(header).u8string().c_str());
    }
  }

  fprintf(fp, "\n\n");

  for (auto & cl : class_data)
  {
    fprintf(fp, "template <>\n");
    fprintf(fp, "struct StormReflTypeInfo<%s>\n", cl.m_Name.c_str());
    fprintf(fp, "{\n");

    if (cl.m_Base.size() == 0)
    {
      fprintf(fp, "  static constexpr int fields_n = %d;\n", cl.m_Fields.size());
      fprintf(fp, "  template <int N> struct field_data_static {};\n");
      fprintf(fp, "  template <int N, typename Self> struct field_data {};\n");
      fprintf(fp, "  template <int N> struct annotations { static constexpr int annotations_n = 0; template <int A> struct annoation { }; };\n");
    }
    else
    {
      fprintf(fp, "  using MyBase = %s;\n", cl.m_Base.c_str());
      fprintf(fp, "  static constexpr int fields_n = %d + StormReflTypeInfo<MyBase>::fields_n;\n", cl.m_Fields.size());
      fprintf(fp, "  template <int N> struct field_data_static : public StormReflTypeInfo<MyBase>::field_data_static<N> {};\n");
      fprintf(fp, "  template <int N, typename Self> struct field_data : public StormReflTypeInfo<MyBase>::field_data<N, match_const_t<Self, MyBase>>\n");
      fprintf(fp, "  {\n");
      fprintf(fp, "    field_data(Self & self) : StormReflTypeInfo<MyBase>::field_data<N, match_const_t<Self, MyBase>>(self) {}\n");
      fprintf(fp, "  };\n");
      fprintf(fp, "  template <int N> struct annotations : public StormReflTypeInfo<MyBase>::annotations<N> {};\n");
    }

    fprintf(fp, "  static %s & GetDefault() { static %s def; return def; }\n", cl.m_Name.c_str(), cl.m_Name.c_str());
    fprintf(fp, "};\n\n");

    std::string base_str = cl.m_Base.size() == 0 ? "" :
      " + StormReflTypeInfo<" + cl.m_Base + ">::fields_n";

    for (std::size_t index = 0; index < cl.m_Fields.size(); index++)
    {
      auto & field = cl.m_Fields[index];

      fprintf(fp, "template <>\n");
      fprintf(fp, "struct StormReflTypeInfo<%s>::field_data_static<%d%s>\n", cl.m_Name.c_str(), index, base_str.c_str());
      fprintf(fp, "{\n");
      fprintf(fp, "  using member_type = %s;\n", field.m_Type.c_str());
      fprintf(fp, "  static constexpr auto GetName() { return \"%s\"; }\n", field.m_Name.c_str());
      fprintf(fp, "  static constexpr auto GetType() { return \"%s\"; }\n", field.m_Type.c_str());
      fprintf(fp, "  static constexpr unsigned GetFieldNameHash() { return 0x%08X; }\n", crc32(field.m_Name));
      fprintf(fp, "  static constexpr unsigned GetTypeNameHash() { return 0x%08X; }\n", crc32(field.m_Type));
      fprintf(fp, "  static constexpr auto GetFieldIndex() { return %d%s; }\n", index, base_str.c_str());
      fprintf(fp, "  static constexpr auto GetMemberPtr() { return &%s::%s; }\n", cl.m_Name.c_str(), field.m_Name.c_str());
      fprintf(fp, "};\n\n");

      fprintf(fp, "template <typename Self>\n");
      fprintf(fp, "struct StormReflTypeInfo<%s>::field_data<%d%s, Self> : public StormReflTypeInfo<%s>::field_data_static<%d%s>\n", 
        cl.m_Name.c_str(), index, base_str.c_str(), cl.m_Name.c_str(), index, base_str.c_str());
      fprintf(fp, "{\n");
      fprintf(fp, "  Self & self;\n");
      fprintf(fp, "  field_data(Self & self) : self(self) {}\n");
      fprintf(fp, "  match_const_t<Self, %s> & Get() { return self.%s; }\n", field.m_Type.c_str(), field.m_Name.c_str());
      fprintf(fp, "  std::add_const_t<std::remove_reference_t<%s>> & Get() const { return self.%s; }\n", field.m_Type.c_str(), field.m_Name.c_str());
      fprintf(fp, "  void SetDefault() { self.%s = StormReflTypeInfo<%s>::GetDefault().%s; }\n", field.m_Name.c_str(), cl.m_Name.c_str(), field.m_Name.c_str());
      fprintf(fp, "};\n\n");

      if (field.m_Attrs.size() > 0)
      {
        fprintf(fp, "template <>\n");
        fprintf(fp, "struct StormReflTypeInfo<%s>::annotations<%d%s>\n", cl.m_Name.c_str(), index, base_str.c_str());
        fprintf(fp, "{\n");
        fprintf(fp, "  static constexpr int annotations_n = %d;\n", field.m_Attrs.size());
        fprintf(fp, "  template <int A> struct annoation { };\n");
        fprintf(fp, "};\n\n");

        for (std::size_t attr_index = 0; attr_index < field.m_Attrs.size(); attr_index++)
        {
          fprintf(fp, "template <>\n");
          fprintf(fp, "struct StormReflTypeInfo<%s>::annotations<%d%s>::annoation<%d>\n", cl.m_Name.c_str(), index, base_str.c_str(), attr_index);
          fprintf(fp, "{\n");
          fprintf(fp, "  static constexpr const char * str = \"%s\";\n", field.m_Attrs[attr_index].c_str());
          fprintf(fp, "};\n\n");
        }
      }
    }
  }


  for (auto & cl : class_funcs)
  {
    fprintf(fp, "template <>\n");
    fprintf(fp, "struct StormReflFuncInfo<%s>\n", cl.m_Name.c_str());
    fprintf(fp, "{\n");

    if (cl.m_Base.size() == 0)
    {
      fprintf(fp, "  static constexpr int funcs_n = %d;\n", cl.m_Funcs.size());
      fprintf(fp, "  template <int N> struct func_data_static {};\n");
    }
    else
    {
      fprintf(fp, "  using MyBase = %s;\n", cl.m_Base.c_str());
      fprintf(fp, "  static constexpr int funcs_n = %d + StormReflFuncInfo<MyBase>::funcs_n;\n", cl.m_Funcs.size());
      fprintf(fp, "  template <int N> struct func_data_static : public StormReflFuncInfo<MyBase>::func_data_static<N> {};\n");
    }

    fprintf(fp, "};\n\n");

    std::string base_str = cl.m_Base.size() == 0 ? "" :
      " + StormReflFuncInfo<" + cl.m_Base + ">::funcs_n";

    for (std::size_t index = 0; index < cl.m_Funcs.size(); index++)
    {
      auto & func = cl.m_Funcs[index];

      fprintf(fp, "template <>\n");
      fprintf(fp, "struct StormReflFuncInfo<%s>::func_data_static<%d%s>\n", cl.m_Name.c_str(), index, base_str.c_str());
      fprintf(fp, "{\n");
      fprintf(fp, "  using return_type = %s;\n", func.m_ReturnType.c_str());
      fprintf(fp, "  static constexpr int params_n = %d;\n", func.m_Params.size());
      fprintf(fp, "  static constexpr auto GetName() { return \"%s\"; }\n", func.m_Name.c_str());
      fprintf(fp, "  static constexpr auto GetReturnType() { return \"%s\"; }\n", func.m_ReturnType.c_str());
      fprintf(fp, "  static constexpr unsigned GetFunctionNameHash() { return 0x%08X; }\n", crc32(func.m_Name));
      fprintf(fp, "  static constexpr unsigned GetReturnTypeNameHash() { return 0x%08X; }\n", crc32(func.m_ReturnType));
      fprintf(fp, "  static constexpr auto GetFunctionIndex() { return %d%s; }\n", index, base_str.c_str());
      fprintf(fp, "  static constexpr auto GetFunctionPtr() { return &%s::%s; }\n", cl.m_Name.c_str(), func.m_Name.c_str());
      fprintf(fp, "  template <int i>\n");
      fprintf(fp, "  struct param_info { };\n");
      fprintf(fp, "};\n\n");

      for (std::size_t param_index = 0; param_index < func.m_Params.size(); param_index++)
      {
        auto & param = func.m_Params[param_index];
        fprintf(fp, "template <>\n");
        fprintf(fp, "struct StormReflFuncInfo<%s>::func_data_static<%d%s>::param_info<%d>\n",
          cl.m_Name.c_str(), index, base_str.c_str(), param_index);
        fprintf(fp, "{\n");
        fprintf(fp, "  using param_type = %s;\n", param.m_Type.c_str());
        fprintf(fp, "  static constexpr auto GetName() { return \"%s\"; }\n", param.m_Name.c_str());
        fprintf(fp, "  static constexpr auto GetType() { return \"%s\"; }\n", param.m_Type.c_str());
        fprintf(fp, "  static constexpr unsigned GetNameHash() { return 0x%08X; }\n", crc32(param.m_Name));
        fprintf(fp, "  static constexpr unsigned GetTypeNameHash() { return 0x%08X; }\n", crc32(param.m_Type));
        fprintf(fp, "};\n\n");
      }
    }
  }
  
  fclose(fp);

}
