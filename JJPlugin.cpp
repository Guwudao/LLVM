

#include <iostream>
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"

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
        
        void run(const MatchFinder::MatchResult &Result) {
            if (const ObjCInterfaceDecl *decl = Result.Nodes.getNodeAs<ObjCInterfaceDecl>("ObjCInterfaceDecl")) {
                
                StringRef className = decl->getName();
                
                char c = className[0];
                if (isLowercase(c)) {
                    std::string tempName = className;
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
                    SourceLocation end = loc.getLocWithOffset(1);
                    StringRef replacement;
                    FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(loc), replacement);
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
        JJASTConsumer(CompilerInstance &ci) :handler(ci) {
            matcher.addMatcher(objcInterfaceDecl().bind("ObjCInterfaceDecl"), &handler);
        }
        
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

static FrontendPluginRegistry::Add<JJPlugin::JJASTAction>
X("JJPlugin", "The JJPlugin is my first clang-plugin.");

