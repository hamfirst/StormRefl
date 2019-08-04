

#include <clang/Driver/Options.h>
#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/QualTypeNames.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include "StormReflOutput.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;



clang::PrintingPolicy * g_PrintingPolicy = nullptr;
llvm::cl::opt<std::string> g_DependencyDir("depsdir", cl::desc("Intermediate directory for writing out the dependency list"));

class StormReflVisitor : public RecursiveASTVisitor<StormReflVisitor>
{
public:

public:
  StormReflVisitor(CompilerInstance & compiler_instance, 
    const std::string & source_file, 
    std::vector<ReflectedDataClass> & class_data, 
    std::vector<ReflectedFunctionalClass> & class_funcs,
    std::vector<ReflectedEnum> & enum_data)
    : m_CompilerInstance(compiler_instance), 
      m_ASTContext(compiler_instance.getASTContext()), 
      m_SourceManager(compiler_instance.getSourceManager()), 
      m_SourceFile(source_file), 
      m_ClassData(class_data),
      m_ClassFuncs(class_funcs),
      m_Enums(enum_data)
  { }

  bool VisitCXXRecordDecl(CXXRecordDecl * decl) 
  {
    if (decl->isLocalClass() || decl->isTemplateDecl() || decl->isCompleteDefinition() == false)
    {
      return true;
    }

    FullSourceLoc full_souce_loc(decl->getLocation(), m_SourceManager);
    if (full_souce_loc.isInSystemHeader())
    {
      return true;
    }

    auto file_entry = m_SourceManager.getFileEntryForID(full_souce_loc.getFileID());
    if (file_entry == nullptr || m_SourceFile != file_entry->getName())
    {
      return true;
    }

    ProcessDataClass(decl);
    ProcessFunctionClass(decl);
    return true;
  }

  void ProcessDataClass(CXXRecordDecl * decl)
  {
    auto decl_name = std::string(decl->getName());

    bool is_reflectable = false;
    bool is_no_base = false;
    for (const auto & sub_decl : decl->decls())
    {
      auto var_decl = dyn_cast<VarDecl>(sub_decl);
      if (var_decl)
      {
        if (var_decl->isStaticDataMember() && var_decl->getNameAsString() == "is_reflectable" && var_decl->hasInit())
        {
          auto qual_type = var_decl->getType();
          if (qual_type.getAsString() != "const _Bool")
          {
            continue;
          }

          auto init_expr = var_decl->getInit();

          bool is_true = false;
          if (init_expr->EvaluateAsBooleanCondition(is_true, m_ASTContext))
          {
            if (is_true)
            {
              is_reflectable = true;
              continue;
            }
          }
        }

        if (var_decl->isStaticDataMember() && var_decl->getNameAsString() == "no_refl_base" && var_decl->hasInit())
        {
          auto qual_type = var_decl->getType();
          if (qual_type.getAsString() != "const _Bool")
          {
            continue;
          }

          auto init_expr = var_decl->getInit();

          bool is_true = false;
          if (init_expr->EvaluateAsBooleanCondition(is_true, m_ASTContext))
          {
            if (is_true)
            {
              is_no_base = true;
              continue;
            }
          }
        }
      }
    }

    if (is_reflectable)
    {
      //printf("Class: %s\n", decl_name.c_str());

      ReflectedDataClass class_data = { decl_name };
      class_data.m_NoDefault = false;

      if(is_no_base == false)
      {
        for (auto &base : decl->bases())
        {
          class_data.m_Base = clang::TypeName::getFullyQualifiedName(base.getType(), m_ASTContext, *g_PrintingPolicy);
          break;
        }

        std::queue<QualType> bases;
        for (auto &base : decl->bases())
        {
          bases.emplace(base.getType());
        }

        while (bases.size() > 0)
        {
          auto base = bases.front();
          bases.pop();

          auto name = base.getBaseTypeIdentifier()->getName();
          auto qual_name = clang::TypeName::getFullyQualifiedName(base, m_ASTContext, *g_PrintingPolicy);

          class_data.m_BaseClasses.emplace_back(ReflectionDataBase{name, qual_name});

          auto record_type = base->getAs<RecordType>();
          if (record_type)
          {
            auto record_decl = record_type->getDecl();
            if (record_decl)
            {
              auto cxx_record = cast<CXXRecordDecl>(record_decl);
              if (cxx_record)
              {
                for (auto &new_base : cxx_record->bases())
                {
                  bases.emplace(new_base.getType());
                }
              }
            }
          }
        }
      }

      bool accessible = decl->isStruct() ? true : false;

      for (auto & class_decl : decl->decls())
      {
        auto access_spec = dyn_cast<AccessSpecDecl>(class_decl);
        if (access_spec)
        {
          accessible = access_spec->getAccess() == AS_public;
          continue;
        }

        auto field = dyn_cast<FieldDecl>(class_decl);
        if (field)
        {
          if (!accessible)
          {
            continue;
          }

          auto qual_type = field->getType();
          auto qual_cannon_type = field->getType().getCanonicalType();
          auto type_str = clang::TypeName::getFullyQualifiedName(qual_type, m_ASTContext, *g_PrintingPolicy);
          auto cannon_str = clang::TypeName::getFullyQualifiedName(qual_cannon_type, m_ASTContext, *g_PrintingPolicy);
          auto name_str = std::string(field->getName());

          bool ignore_field = false;
          bool is_array = false;

          auto type = qual_type.getTypePtrOrNull();
          if(type && dyn_cast<ConstantArrayType>(type))
          {
            is_array = true;
          }

          std::vector<std::string> annotations;
          if (field->hasAttrs())
          {
            for (const auto & attr : field->getAttrs())
            {
              auto annotation = dyn_cast<AnnotateAttr>(attr);
              if (annotation)
              {
                auto annotation_str = std::string(annotation->getAnnotation());
                if (annotation_str == "no_refl")
                {
                  ignore_field = true;
                  break;
                }
                else if (annotation_str == "no_default")
                {
                  class_data.m_NoDefault = true;
                  ignore_field = true;
                  break;
                }
                else
                {
                  annotations.push_back(annotation_str);
                }
              }
            }
          }

          if (!ignore_field)
          {
            class_data.m_Fields.emplace_back(ReflectedField{ name_str, type_str, cannon_str, annotations, is_array });
          }
        }

        //printf("Field: %s, %s\n", name_str.c_str(), type_str.c_str());
      }

      m_ClassData.emplace_back(class_data);
    }
  }

  void ProcessFunctionClass(CXXRecordDecl * decl)
  {
    auto decl_name = std::string(decl->getName());

    bool is_functional = false;
    bool is_no_base = false;
    for (const auto & sub_decl : decl->decls())
    {
      auto var_decl = dyn_cast<VarDecl>(sub_decl);
      if (var_decl)
      {
        if (var_decl->isStaticDataMember() && var_decl->getNameAsString() == "is_functional" && var_decl->hasInit())
        {
          auto qual_type = var_decl->getType();
          if (qual_type.getAsString() != "const _Bool")
          {
            continue;
          }

          auto init_expr = var_decl->getInit();

          bool is_true = false;
          if (init_expr->EvaluateAsBooleanCondition(is_true, m_ASTContext))
          {
            if (is_true)
            {
              is_functional = true;
              continue;
            }
          }
        }

        if (var_decl->isStaticDataMember() && var_decl->getNameAsString() == "no_refl_base" && var_decl->hasInit())
        {
          auto qual_type = var_decl->getType();
          if (qual_type.getAsString() != "const _Bool")
          {
            continue;
          }

          auto init_expr = var_decl->getInit();

          bool is_true = false;
          if (init_expr->EvaluateAsBooleanCondition(is_true, m_ASTContext))
          {
            if (is_true)
            {
              is_no_base = true;
              continue;
            }
          }
        }
      }
    }

    if (is_functional)
    {
      ReflectedFunctionalClass class_data = { decl_name };

      if(is_no_base == false)
      {
        for (const auto & base : decl->bases())
        {
          class_data.m_Base = base.getType().getBaseTypeIdentifier()->getName();
          break;
        }
      }

      bool accessible = decl->isStruct() ? true : false;

      for (auto & class_decl : decl->decls())
      {
        auto access_spec = dyn_cast<AccessSpecDecl>(class_decl);
        if (access_spec)
        {
          accessible = access_spec->getAccess() == AS_public;
          continue;
        }

        auto method = dyn_cast<CXXMethodDecl>(class_decl);
        if (method)
        {
          if (!accessible)
          {
            continue;
          }

          bool ignore_method = true;
          if (method->hasAttrs())
          {
            for (const auto & attr : method->getAttrs())
            {
              auto annotation = dyn_cast<AnnotateAttr>(attr);
              if (annotation)
              {
                auto annotation_str = std::string(annotation->getAnnotation());
                if (annotation_str == "refl_func")
                {
                  ignore_method = false;
                  break;
                }
              }
            }
          }

          if (ignore_method)
          {
            continue;
          }

          ReflectedFunc func = { method->getName() };
          auto func_qual_type = m_ASTContext.getMemberPointerType(method->getType(), method->getParent()->getTypeForDecl());

          func.m_FullSignature = clang::TypeName::getFullyQualifiedName(func_qual_type, m_ASTContext, *g_PrintingPolicy);
          func.m_ReturnType = clang::TypeName::getFullyQualifiedName(method->getReturnType(), m_ASTContext, *g_PrintingPolicy);

          for (auto param : method->parameters())
          {
            func.m_Params.emplace_back(ReflectedParam{ param->getName(), clang::TypeName::getFullyQualifiedName(param->getType(), m_ASTContext, *g_PrintingPolicy) });
          }

          class_data.m_Funcs.emplace_back(std::move(func));
        }
      }

      m_ClassFuncs.emplace_back(class_data);
    }
  }

  bool VisitEnumDecl(EnumDecl * decl)
  {
    if (decl->isComplete() == false)
    {
      return true;
    }

    FullSourceLoc full_souce_loc(decl->getLocation(), m_SourceManager);
    if (full_souce_loc.isInSystemHeader())
    {
      return true;
    }

    auto file_entry = m_SourceManager.getFileEntryForID(full_souce_loc.getFileID());
    if (file_entry == nullptr || m_SourceFile != file_entry->getName())
    {
      return true;
    }

    bool ignore_enum = true;
    if (decl->hasAttrs())
    {
      for (const auto & attr : decl->getAttrs())
      {
        auto annotation = dyn_cast<AnnotateAttr>(attr);
        if (annotation)
        {
          auto annotation_str = std::string(annotation->getAnnotation());
          if (annotation_str == "refl_enum")
          {
            ignore_enum = false;
            break;
          }
        }
      }
    }

    if (ignore_enum)
    {
      return true;
    }

    auto qual_type = decl->getTypeForDecl()->getLocallyUnqualifiedSingleStepDesugaredType();
    auto type_str = clang::TypeName::getFullyQualifiedName(qual_type, m_ASTContext, *g_PrintingPolicy);

    ReflectedEnum data = { type_str, decl->isScopedUsingClassTag() };

    for (auto && enum_elem : decl->enumerators())
    {
      data.m_Elems.emplace_back(ReflectedEnumElem{ enum_elem->getName(), enum_elem->getQualifiedNameAsString() });
    }

    m_Enums.push_back(data);
    return true;
  }

private:
  CompilerInstance & m_CompilerInstance;
  ASTContext & m_ASTContext;
  SourceManager & m_SourceManager;

  std::string m_SourceFile;

  std::vector<ReflectedDataClass> & m_ClassData;
  std::vector<ReflectedFunctionalClass> & m_ClassFuncs;
  std::vector<ReflectedEnum> & m_Enums;
};

class StormReflASTConsumer : public ASTConsumer 
{
public:
  StormReflASTConsumer(CompilerInstance & compiler_instance, 
    const std::string & source_file, 
    std::vector<ReflectedDataClass> & class_data, 
    std::vector<ReflectedFunctionalClass> & class_funcs,
    std::vector<ReflectedEnum> & enum_data)
    : m_CompilerInstance(compiler_instance), m_Visitor(std::make_unique<StormReflVisitor>(compiler_instance, source_file, class_data, class_funcs, enum_data))
  { }

  virtual void HandleTranslationUnit(ASTContext & context) 
  {
    if (m_CompilerInstance.hasDiagnostics())
    {
      if (m_CompilerInstance.getDiagnostics().hasUncompilableErrorOccurred())
      {
        return;
      }
    }

    auto translation_unit = context.getTranslationUnitDecl();
    if (translation_unit)
    {
      m_Visitor->TraverseDecl(translation_unit);
    }
  }

private:
  CompilerInstance & m_CompilerInstance;
  std::unique_ptr<StormReflVisitor> m_Visitor;
};

class FindIncludes : public PPCallbacks
{
public:
  FindIncludes(SourceManager & source_manager, const std::string & source_file, std::vector<std::string> & includes, std::vector<std::string> & dependencies)
    : m_SourceManager(source_manager), m_SourceFile(source_file), m_Includes(includes), m_Dependencies(dependencies)
  { }

  void InclusionDirective(
    SourceLocation hash_loc,
    const Token &include_token,
    StringRef file_name,
    bool is_angled,
    CharSourceRange filename_range,
    const FileEntry *file,
    StringRef search_path,
    StringRef relative_path,
    const clang::Module *imported,
    clang::SrcMgr::CharacteristicKind file_type) override
  {
    FullSourceLoc full_souce_loc(hash_loc, m_SourceManager);

    if (full_souce_loc.isInSystemHeader())
    {
      return;
    }

    if (g_DependencyDir.length() > 0 && file != nullptr)
    {
      auto file_path = file->tryGetRealPathName();
      if (file_path.size() > 0)
      {
        m_Dependencies.emplace_back(file_path);
      }
    }

    auto file_entry = m_SourceManager.getFileEntryForID(full_souce_loc.getFileID());
    if (m_SourceFile == file_entry->getName())
    {
      m_Includes.push_back(relative_path);
    }
  }

  auto & GetIncludeList() { return m_Includes; }

private:

  SourceManager & m_SourceManager;
  std::string m_SourceFile;
  std::vector<std::string> & m_Includes;
  std::vector<std::string> & m_Dependencies;
};

class StormReflFrontendAction: public ASTFrontendAction
{
public:
  virtual bool BeginSourceFileAction(CompilerInstance & compiler_instance) override
  {
    auto file = getCurrentFile();
    m_SourceFile = file;

    std::unique_ptr<FindIncludes> find_includes_callback = std::make_unique<FindIncludes>(compiler_instance.getSourceManager(), std::string(file), m_Includes, m_Depenencies);

    auto & preprocessor = compiler_instance.getPreprocessor();
    preprocessor.addPPCallbacks(std::move(find_includes_callback));

    return true;
  }

  virtual void EndSourceFileAction() override
  {
    if (m_SourceFile.size() == 0)
    {
      return;
    }

    auto & compiler_instance = getCompilerInstance();
    if (compiler_instance.hasDiagnostics())
    {
      if (compiler_instance.getDiagnostics().hasUncompilableErrorOccurred())
      {
        return;
      }
    }

    OutputReflectedFile(m_SourceFile, m_ClassFuncs, m_ClassData, m_EnumData, m_Includes);

    if (g_DependencyDir.length() > 0)
    {
      OutputDependencyFile(m_SourceFile, g_DependencyDir, m_Depenencies);
    }

    m_Includes.clear();
    m_Depenencies.clear();
    m_ClassData.clear();
    m_SourceFile.clear();
  }

  virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance & compiler_instance, StringRef) override
  {
    return std::make_unique<StormReflASTConsumer>(compiler_instance, m_SourceFile, m_ClassData, m_ClassFuncs, m_EnumData);
  }

private:

  std::string m_SourceFile;
  std::vector<std::string> m_Includes;
  std::vector<std::string> m_Depenencies;
  std::vector<ReflectedDataClass> m_ClassData;
  std::vector<ReflectedFunctionalClass> m_ClassFuncs;
  std::vector<ReflectedEnum> m_EnumData;
};


int main(int argc, const char **argv) 
{
  clang::LangOptions lang_opts;
  lang_opts.CPlusPlus = true;
  clang::PrintingPolicy policy(lang_opts);
  policy.adjustForCPlusPlus();

  g_PrintingPolicy = &policy;

  cl::OptionCategory option_category("StormRefl");
  CommonOptionsParser op(argc, argv, option_category);
  if (op.getSourcePathList().size() == 0)
  {
    return 0;
  }

  ClangTool Tool(op.getCompilations(), op.getSourcePathList());
  int result = Tool.run(newFrontendActionFactory<StormReflFrontendAction>().get());
  return result;
}
