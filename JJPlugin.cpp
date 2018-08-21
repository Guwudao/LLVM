

#include <iostream>
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"

#include "llvm/Support/raw_ostream.h"
#include "clang/Sema/Sema.h"
#include "clang/AST/RecursiveASTVisitor.h"

using namespace clang;
using namespace std;
using namespace llvm;
using namespace clang::ast_matchers;

namespace JJPlugin {
    class JJHandler : public MatchFinder::MatchCallback {
    private:
        CompilerInstance &ci;
        
    public:
        JJHandler(CompilerInstance &ci) :ci(ci) {}
        
        bool JJVisitObjCMethodDecl(const ObjCMethodDecl *declaration)
        {
            cout << "JJVisitObjCMethodDecl -- JJVisitObjCMethodDecl -- JJVisitObjCMethodDecl " << endl;
            if (isUserSourceCode(declaration))
            {
                cout << "isUserSourceCode -- true" << endl;
//                checkMethodNameForUppercaseName(declaration);
//                checkMethodParamsNameForUppercaseName(declaration);
//                checkMethodBodyForOver500Lines(declaration);
            }
            
            return true;
        }
        
        //判断是否为用户源码
        //@return true 为用户源码，false 非用户源码
        bool isUserSourceCode (const Decl *decl)
        {
//            cout << "isUserSourceCode -- isUserSourceCode" << endl;
//            string filename = ci.getSourceManager().getFilename(decl->getSourceRange().getBegin()).str();
//            cout << "filename: -- " + filename << endl;
//            if (filename.empty())
//                cout << "filename.empty() -- filename.empty()" << endl;
//                return false;
//
//            //非XCode中的源码都认为是用户源码
//            if(filename.find("/Applications/Xcode.app/") == 0)
//                cout << "filename.find(\"/Applications/Xcode.app/\")" << endl;
//                return false;

            cout << "isUserSourceCode true -- true --true" << endl;
            return true;
        }
        
        
        //检测方法名是否存在大写开头
        void checkMethodNameForUppercaseName(const ObjCMethodDecl *decl)
        {
            cout << "检测方法名是否存在大写开头 -- 检测方法名是否存在大写开头 -- 检测方法名是否存在大写开头 " << endl;
            //检查名称的每部分，都不允许以大写字母开头
            Selector sel = decl -> getSelector();
            int selectorPartCount = decl -> getNumSelectorLocs();
            for (int i = 0; i < selectorPartCount; i++)
            {
                StringRef selName = sel.getNameForSlot(i);
                char c = selName[0];
                if (isUppercase(c))
                {
                    //fixItHint
                    std::string tempName = selName;
                    tempName[0] = toLowercase(c);
                    StringRef replacement(tempName);
                    SourceLocation nameStart = decl -> getSelectorLoc(i);
                    SourceLocation nameEnd = nameStart.getLocWithOffset(selName.size() - 1);
                    FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(nameStart, nameEnd), replacement);
                    
                    //Warning
                    DiagnosticsEngine &D = ci.getDiagnostics();
                    int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, "Selector name should not start with uppercase letter");
                    SourceLocation location = decl->getLocation();
                    D.Report(location, diagID).AddFixItHint(fixItHint);
                }
            }
        }
        
        
        void run(const MatchFinder::MatchResult &Result) {
            
                if (const ObjCMethodDecl *Mdecl = Result.Nodes.getNodeAs<ObjCMethodDecl>("ObjCMethodDecl")) {
//
//
//                    JJVisitObjCMethodDecl(Mdecl);
//
                    if (JJVisitObjCMethodDecl(Mdecl)) {
                        cout << "const ObjCMethodDecl *Mdecl" << endl;
                    }
//
//                    //检查名称的每部分，都不允许以大写字母开头
                    Selector sel = Mdecl -> getSelector();
                    int selectorPartCount = Mdecl -> getNumSelectorLocs();
                    for (int i = 0; i < selectorPartCount; i++)
                    {
                        StringRef selName = sel.getNameForSlot(i);
                        std::string tempName = selName;
//                        cout << tempName << endl;
                        char c = tempName[0];
                        if (isUppercase(c))
                        {
                            //fixItHint
                            tempName[0] = toLowercase(c);
                            StringRef replacement(tempName);
                            SourceLocation nameStart = Mdecl -> getSelectorLoc(i);
                            SourceLocation nameEnd = nameStart.getLocWithOffset(selName.size() - 1);
                            FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(nameStart, nameEnd), replacement);

                            //Error
                            DiagnosticsEngine &D = ci.getDiagnostics();
                            int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, "方法名应该以小写字母开头");
                            SourceLocation location = Mdecl->getLocation();
                            D.Report(location, diagID).AddFixItHint(fixItHint);
                        }
                    }
                }
            
            
            
            
            if (const ObjCInterfaceDecl *decl = Result.Nodes.getNodeAs<ObjCInterfaceDecl>("ObjCInterfaceDecl")) {
                
                StringRef className = decl->getName();
                
                char c = className[0];
                if (isLowercase(c)) {
                    std::string tempName = className;
                    cout << tempName << endl;
                    tempName[0] = toUppercase(c);
                    StringRef replacement(tempName);
                    SourceLocation nameStart = decl->getLocation();
                    SourceLocation nameEnd = nameStart.getLocWithOffset(className.size() - 1);
                    FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(nameStart, nameEnd), replacement);
                    
                    DiagnosticsEngine &D = ci.getDiagnostics();
                    int diagID = D.getCustomDiagID(DiagnosticsEngine::Error, "老子警告你：类名不能以小写字母开头");
                    SourceLocation location = decl->getLocation();
                    D.Report(location, diagID).AddFixItHint(fixItHint);
                }
                
                size_t pos = decl->getName().find('_');
                if (pos != StringRef::npos) {
                    DiagnosticsEngine &D = ci.getDiagnostics();
                    SourceLocation loc = decl->getLocation().getLocWithOffset(pos);
                    
                    std::string tempName = className;
                    std::string::iterator end_pos = std::remove(tempName.begin(), tempName.end(), '_');
                    tempName.erase(end_pos, tempName.end());
                    StringRef replacement(tempName);
                    SourceLocation nameStart = decl->getLocation();
                    SourceLocation nameEnd = nameStart.getLocWithOffset(className.size() - 1);
                    FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(nameStart, nameEnd), replacement);
                    
                    int diagID = D.getCustomDiagID(DiagnosticsEngine::Error, "老子警告你：类名中不能带有下划线");
                    D.Report(loc, diagID).AddFixItHint(fixItHint);
                }
                
            }
        }
    };
        
    class JJASTConsumer: public ASTConsumer {
    private:
        MatchFinder matcher;
        JJHandler handler;
        
    public:
        //调用CreateASTConsumer方法后就会加载Consumer里面的方法
        JJASTConsumer(CompilerInstance &ci) :handler(ci) {
            matcher.addMatcher(objcInterfaceDecl().bind("ObjCInterfaceDecl"), &handler);
            matcher.addMatcher(objcMethodDecl().bind("ObjCMethodDecl"), &handler);
        }
        
        //遍历完一次语法树就会调用一次下面方法
        void HandleTranslationUnit(ASTContext &context) {
            matcher.matchAST(context);
        }
    };
    
    class JJASTAction: public PluginASTAction {
    public:
        unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &ci, StringRef iFile) {
            return unique_ptr<JJASTConsumer> (new JJASTConsumer(ci));
        }
        
        bool ParseArgs(const CompilerInstance &ci, const vector<string> &args) {
            return true;
        }
    };
}

// MARK: ---
//Example clang plugin which simply prints the names of all the top-level decls
// in the input file.
namespace demonstrates {
    
    class PrintFunctionsConsumer : public ASTConsumer {
        CompilerInstance &Instance;
        std::set<std::string> ParsedTemplates;
        
    public:
        PrintFunctionsConsumer(CompilerInstance &Instance,
                               std::set<std::string> ParsedTemplates)
        : Instance(Instance), ParsedTemplates(ParsedTemplates) {}
        
        bool HandleTopLevelDecl(DeclGroupRef DG) override {
            for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; ++i) {
                const Decl *D = *i;
                if (const NamedDecl *ND = dyn_cast<NamedDecl>(D))
                    llvm::errs() << "top-level-decl: \"" << ND->getNameAsString() << "\"\n";
            }
            
            return true;
        }
        
        void HandleTranslationUnit(ASTContext& context) override {
            if (!Instance.getLangOpts().DelayedTemplateParsing)
                return;
            
            // This demonstrates how to force instantiation of some templates in
            // -fdelayed-template-parsing mode. (Note: Doing this unconditionally for
            // all templates is similar to not using -fdelayed-template-parsig in the
            // first place.)
            // The advantage of doing this in HandleTranslationUnit() is that all
            // codegen (when using -add-plugin) is completely finished and this can't
            // affect the compiler output.
            struct Visitor : public RecursiveASTVisitor<Visitor> {
                const std::set<std::string> &ParsedTemplates;
                Visitor(const std::set<std::string> &ParsedTemplates)
                : ParsedTemplates(ParsedTemplates) {}
                bool VisitFunctionDecl(FunctionDecl *FD) {
                    if (FD->isLateTemplateParsed() &&
                        ParsedTemplates.count(FD->getNameAsString()))
                        LateParsedDecls.insert(FD);
                    return true;
                }
                
                std::set<FunctionDecl*> LateParsedDecls;
            } v(ParsedTemplates);
            v.TraverseDecl(context.getTranslationUnitDecl());
            clang::Sema &sema = Instance.getSema();
            for (const FunctionDecl *FD : v.LateParsedDecls) {
                clang::LateParsedTemplate &LPT =
                *sema.LateParsedTemplateMap.find(FD)->second;
                sema.LateTemplateParser(sema.OpaqueParser, LPT);
                llvm::errs() << "late-parsed-decl: \"" << FD->getNameAsString() << "\"\n";
            }
        }
    };
    
    class PrintFunctionNamesAction : public PluginASTAction {
        std::set<std::string> ParsedTemplates;
    protected:
        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                       llvm::StringRef) override {
            return llvm::make_unique<PrintFunctionsConsumer>(CI, ParsedTemplates);
        }
        
        bool ParseArgs(const CompilerInstance &CI,
                       const std::vector<std::string> &args) override {
            for (unsigned i = 0, e = args.size(); i != e; ++i) {
                llvm::errs() << "PrintFunctionNames arg = " << args[i] << "\n";
                
                // Example error handling.
                DiagnosticsEngine &D = CI.getDiagnostics();
                if (args[i] == "-an-error") {
                    unsigned DiagID = D.getCustomDiagID(DiagnosticsEngine::Error,
                                                        "invalid argument '%0'");
                    D.Report(DiagID) << args[i];
                    return false;
                } else if (args[i] == "-parse-template") {
                    if (i + 1 >= e) {
                        D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                                   "missing -parse-template argument"));
                        return false;
                    }
                    ++i;
                    ParsedTemplates.insert(args[i]);
                }
            }
            if (!args.empty() && args[0] == "help")
                PrintHelp(llvm::errs());
            
            return true;
        }
        
        void PrintHelp(llvm::raw_ostream& ros) {
            ros << "Help for PrintFunctionNames plugin goes here\n";
        }
        
    };
    
}


static FrontendPluginRegistry::Add<JJPlugin::JJASTAction>
X("JJPlugin", "The JJPlugin is my first clang-plugin.");

static FrontendPluginRegistry::Add<demonstrates::PrintFunctionNamesAction>
Y("demonstrates","add demonstrates");
