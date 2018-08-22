

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
        ASTContext *context;
        
    public:
        JJHandler(CompilerInstance &ci) :ci(ci) {}
        
    private:
        bool JJVisitObjCMethodDecl(const ObjCMethodDecl *declaration)
        {
            cout << "JJVisitObjCMethodDecl -- JJVisitObjCMethodDecl -- JJVisitObjCMethodDecl " << endl;
            if (isUserSourceCode(declaration))
            {
                checkMethodNameForUppercaseName(declaration);
                cout << "isUserSourceCode -- true" << endl;
//                checkMethodParamsNameForUppercaseName(declaration);
//                checkMethodBodyForOver500Lines(declaration);
            }
            
            return true;
        }
        
        bool VisitStmt(Stmt *s)
        {
            string filename = context->getSourceManager().getFilename(s->getSourceRange().getBegin()).str();
            
            if (filename.empty())
                cout << "filename.empty() -- filename.empty()" << endl;
            return false;
            
            if(filename.find("/Applications/Xcode.app/") == 0)
                cout << "filename.find(\"/Applications/Xcode.app/\")" << endl;
            return false;
            
            return true;
        }
        
        //判断是否为用户源码
        //@return true 为用户源码，false 非用户源码
        bool isUserSourceCode (const Decl *decl)
        {
//            cout << "isUserSourceCode -- isUserSourceCode" << endl;
//            string filename = context->getSourceManager().getFilename(decl->getSourceRange().getBegin()).str();
            
//            string filename = ci.getSourceManager().getFilename(decl->getSourceRange().getBegin()).str();
//            StringRef name = ci.getSourceManager().getFilename(decl->getSourceRange().getBegin());
            
//            cout << "filename: -- " + filename << endl;
//            if (name.empty())
//                cout << "filename.empty() -- filename.empty()" << endl;
//                return false;
////
            //非XCode中的源码都认为是用户源码
//            if(name.find("/Applications/Xcode.app/") == 0)
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
                std::string tempName = selName;
                char c = tempName[0];
                if (isUppercase(c))
                {
                    //fixItHint
//                    std::string tempName = selName;
                    tempName[0] = toLowercase(c);
                    StringRef replacement(tempName);
                    SourceLocation nameStart = decl -> getSelectorLoc(i);
                    SourceLocation nameEnd = nameStart.getLocWithOffset(selName.size() - 1);
                    FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(nameStart, nameEnd), replacement);
                    
                    //Warning
                    DiagnosticsEngine &D = ci.getDiagnostics();
                    int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, "方法名建议用小写字母开头");
                    SourceLocation location = decl->getLocation();
                    D.Report(location, diagID).AddFixItHint(fixItHint);
                }
            }
        }
        
        
        void run(const MatchFinder::MatchResult &Result) {
            
                if (const ObjCMethodDecl *Mdecl = Result.Nodes.getNodeAs<ObjCMethodDecl>("ObjCMethodDecl")) {

                    JJVisitObjCMethodDecl(Mdecl);

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

namespace CodingStyle {
    
    class CodingStyleASTVisitor : public RecursiveASTVisitor<CodingStyleASTVisitor>
    {
    private:
        ASTContext *context;
        string objcClsImpl;
        bool objcIsInstanceMethod;
        string objcSelector;
        string objcMethodSrcCode;
    };
    
    class CodingStyleASTConsumer : public ASTConsumer
    {
    private:
        CodingStyleASTVisitor visitor;
        void HandleTranslationUnit(ASTContext &context);
    };
    
    class CodingStyleASTAction : public PluginASTAction
    {
    public:
        unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &Compiler,llvm::StringRef InFile);
        bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string>& args);
    };
}


static FrontendPluginRegistry::Add<JJPlugin::JJASTAction>
X("JJPlugin", "The JJPlugin is my first clang-plugin.");

static clang::FrontendPluginRegistry::Add<CodingStyle::CodingStyleASTAction>
Y("ClangCodingStylePlugin", "ClangCodingStylePlugin");
