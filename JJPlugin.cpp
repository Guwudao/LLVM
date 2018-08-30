

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

#include "clang/Basic/Diagnostic.h"

#include "clang/AST/DeclObjC.h"

using namespace clang;
using namespace std;
using namespace llvm;
using namespace clang::ast_matchers;


namespace JJPlugin {
    
    // MARK: - my handler
    class JJHandler : public MatchFinder::MatchCallback {
    private:
        CompilerInstance &ci;
        
    public:
        JJHandler(CompilerInstance &ci) :ci(ci) {}
        
        void checkInterfaceDecl(const ObjCInterfaceDecl *decl){
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
                int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, "听我劝一句：类名不能以小写字母开头");
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
                
                int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, "听我劝一句：类名中不能带有下划线");
                D.Report(loc, diagID).AddFixItHint(fixItHint);
            }
        }
        
        void checkPropertyDecl(const clang::ObjCPropertyDecl *propertyDecl)
        {
            ObjCPropertyDecl::PropertyAttributeKind attrKind = propertyDecl->getPropertyAttributes();
            SourceLocation location = propertyDecl->getLocation();
            string typeStr = propertyDecl->getType().getAsString();
            
            if (propertyDecl->getTypeSourceInfo()) {
                //                if(!(attrKind & ObjCPropertyDecl::OBJC_PR_nonatomic)){
                //                    diagWaringReport(location, "Are you sure to use atomic which might reduce the performance.", NULL);
                //                }
                
                if ((typeStr.find("NSString")!=string::npos)&& !(attrKind & ObjCPropertyDecl::OBJC_PR_copy)) {
                    diagWaringReport(location, "听我劝一句：NSString建议使用copy代替strong.", NULL);
                } else if ((typeStr.find("NSArray")!=string::npos)&& !(attrKind & ObjCPropertyDecl::OBJC_PR_copy)) {
                    diagWaringReport(location, "听我劝一句：NSArray建议使用copy代替strong.", NULL);
                }
                
                if(!typeStr.compare("int")){
                    diagWaringReport(location, "Use the built-in NSInteger instead of int.", NULL);
                } else if ((typeStr.find("<")!=string::npos && typeStr.find(">")!=string::npos) && !(attrKind & ObjCPropertyDecl::OBJC_PR_weak)) {
                    diagWaringReport(location, "听我劝一句：建议使用weak定义Delegate.", NULL);
                }
            }
        }
        
        
        template <unsigned N>
        void diagWaringReport(SourceLocation Loc, const char (&FormatString)[N], FixItHint *Hint)
        {
            DiagnosticsEngine &diagEngine = ci.getDiagnostics();
            unsigned DiagID = diagEngine.getCustomDiagID(clang::DiagnosticsEngine::Warning, FormatString);
            (Hint!=NULL) ? diagEngine.Report(Loc, DiagID) << *Hint : diagEngine.Report(Loc, DiagID);
        }
        
        void run(const MatchFinder::MatchResult &Result) {
            
            if (const ObjCInterfaceDecl *interfaceDecl = Result.Nodes.getNodeAs<ObjCInterfaceDecl>("ObjCInterfaceDecl")) {
                //类的检测
                checkInterfaceDecl(interfaceDecl);
            }
            
            if (const ObjCPropertyDecl *propertyDecl = Result.Nodes.getNodeAs<ObjCPropertyDecl>("objcPropertyDecl")) {
                //属性的检测
                checkPropertyDecl(propertyDecl);
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
            matcher.addMatcher(objcPropertyDecl().bind("objcPropertyDecl"), &handler);
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
        
        bool ParseArgs(const CompilerInstance &ci, const std::vector<std::string> &args) {
            return true;
        }
    };
}



static FrontendPluginRegistry::Add<JJPlugin::JJASTAction>
X("JJPlugin", "The JJPlugin is my first clang-plugin.");


