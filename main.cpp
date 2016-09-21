#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Core/QualTypeNames.h"

#include "StormReflOutput.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;



class StormReflVisitor : public RecursiveASTVisitor<StormReflVisitor>
{
public:

public:
  StormReflVisitor(CompilerInstance & compiler_instance, const std::string & source_file, std::vector<ReflectedDataClass> & class_data, std::vector<ReflectedFunctionalClass> & class_funcs)
    : m_CompilerInstance(compiler_instance), 
      m_ASTContext(compiler_instance.getASTContext()), 
      m_SourceManager(compiler_instance.getSourceManager()), 
      m_SourceFile(source_file), 
      m_ClassData(class_data),
      m_ClassFuncs(class_funcs)
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
              break;
            }
          }
        }
      }
    }

    if (is_reflectable)
    {
      //printf("Class: %s\n", decl_name.c_str());

      ReflectedDataClass class_data = { decl_name };

      for (const auto & base : decl->bases())
      {
        class_data.m_Base = base.getType().getBaseTypeIdentifier()->getName();
        break;
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

          auto qual_type = field->getType().getCanonicalType();
          auto type_str = clang::TypeName::getFullyQualifiedName(qual_type, m_ASTContext);
          auto name_str = std::string(field->getName());

          bool ignore_field = false;

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
                else
                {
                  annotations.push_back(annotation_str);
                }
              }
            }
          }

          if (!ignore_field)
          {
            class_data.m_Fields.emplace_back(ReflectedField{ name_str, type_str, annotations });
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
              break;
            }
          }
        }
      }
    }

    if (is_functional)
    {
      ReflectedFunctionalClass class_data = { decl_name };

      for (const auto & base : decl->bases())
      {
        class_data.m_Base = base.getType().getBaseTypeIdentifier()->getName();
        break;
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
          func.m_ReturnType = clang::TypeName::getFullyQualifiedName(method->getReturnType(), m_ASTContext);

          for (auto param : method->parameters())
          {
            func.m_Params.emplace_back(ReflectedParam{ param->getName(), clang::TypeName::getFullyQualifiedName(param->getType(), m_ASTContext) });
          }

          class_data.m_Funcs.emplace_back(std::move(func));
        }
      }

      m_ClassFuncs.emplace_back(class_data);
    }
  }

private:
  CompilerInstance & m_CompilerInstance;
  ASTContext & m_ASTContext;
  SourceManager & m_SourceManager;

  std::string m_SourceFile;

  std::vector<ReflectedDataClass> & m_ClassData;
  std::vector<ReflectedFunctionalClass> & m_ClassFuncs;
};

class StormReflASTConsumer : public ASTConsumer 
{
public:
  StormReflASTConsumer(CompilerInstance & compiler_instance, const std::string & source_file, std::vector<ReflectedDataClass> & class_data, std::vector<ReflectedFunctionalClass> & class_funcs)
    : m_CompilerInstance(compiler_instance), m_Visitor(std::make_unique<StormReflVisitor>(compiler_instance, source_file, class_data, class_funcs))
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
  FindIncludes(SourceManager & source_manager, const std::string & source_file, std::vector<std::string> & includes)
    : m_SourceManager(source_manager), m_SourceFile(source_file), m_Includes(includes)
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
    const clang::Module *imported) override
  {
    FullSourceLoc full_souce_loc(hash_loc, m_SourceManager);

    if (full_souce_loc.isInSystemHeader())
    {
      return;
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
};

class StormReflFrontendAction: public ASTFrontendAction
{
public:
  virtual bool BeginSourceFileAction(CompilerInstance & compiler_instance, StringRef file) override
  {
    m_SourceFile = file;

    std::unique_ptr<FindIncludes> find_includes_callback = std::make_unique<FindIncludes>(compiler_instance.getSourceManager(), std::string(file), m_Includes);

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

    OutputReflectedFile(m_SourceFile, m_ClassFuncs, m_ClassData, m_Includes);

    m_Includes.clear();
    m_ClassData.clear();
    m_SourceFile.clear();
  }

  virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance & compiler_instance, StringRef) override
  {
    return std::make_unique<StormReflASTConsumer>(compiler_instance, m_SourceFile, m_ClassData, m_ClassFuncs);
  }

private:

  std::string m_SourceFile;
  std::vector<std::string> m_Includes;
  std::vector<ReflectedDataClass> m_ClassData;
  std::vector<ReflectedFunctionalClass> m_ClassFuncs;
};

int main(int argc, const char **argv) 
{
  CommonOptionsParser op(argc, argv, cl::OptionCategory("StormRefl"));

  ClangTool Tool(op.getCompilations(), op.getSourcePathList());
  int result = Tool.run(newFrontendActionFactory<StormReflFrontendAction>().get());
  return result;
}
