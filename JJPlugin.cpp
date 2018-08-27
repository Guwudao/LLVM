

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

using namespace clang;
using namespace std;
using namespace llvm;
using namespace clang::ast_matchers;


namespace JJPlugin {
    
    // MARK: - my handler
    class JJHandler : public MatchFinder::MatchCallback {
    private:
        CompilerInstance &ci;
//        ASTContext *context;
        
    public:
        JJHandler(CompilerInstance &ci) :ci(ci) {}
        
        bool isUserSourceCode (const Decl *decl)
        {
            string filename = ci.getSourceManager().getFilename(decl->getSourceRange().getBegin()).str();

            if (filename.empty())
                return false;

            //非XCode中的源码都认为是用户源码
            if(filename.find("/Applications/Xcode.app/") == 0)
                return false;

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
                    diagWaringReport(location, "NSString 应该使用 copy 代替 strong.", NULL);
                } else if ((typeStr.find("NSArray")!=string::npos)&& !(attrKind & ObjCPropertyDecl::OBJC_PR_copy)) {
                    diagWaringReport(location, "NSArray 应该使用 copy 代替 strong.", NULL);
                }
                
                if(!typeStr.compare("int")){
                    diagWaringReport(location, "Use the built-in NSInteger instead of int.", NULL);
                } else if ((typeStr.find("<")!=string::npos && typeStr.find(">")!=string::npos) && !(attrKind & ObjCPropertyDecl::OBJC_PR_weak)) {
                    diagWaringReport(location, "应该使用 weak 定义 Delegate.", NULL);
                }
            }
        }
        
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
        
        template <unsigned N>
        void diagWaringReport(SourceLocation Loc, const char (&FormatString)[N], FixItHint *Hint)
        {
            DiagnosticsEngine &diagEngine = ci.getDiagnostics();
            unsigned DiagID = diagEngine.getCustomDiagID(clang::DiagnosticsEngine::Warning, FormatString);
            (Hint!=NULL) ? diagEngine.Report(Loc, DiagID) << *Hint : diagEngine.Report(Loc, DiagID);
        }
        
        
        
        void run(const MatchFinder::MatchResult &Result) {
            
            if (const ObjCPropertyDecl *propertyDecl = Result.Nodes.getNodeAs<ObjCPropertyDecl>("objcPropertyDecl")) {
                
                cout << "objcPropertyDecl -- objcPropertyDecl" << endl;
                // 存储 Objective-C 类属性
                checkPropertyDecl(propertyDecl);
                
            } else if (const ObjCInterfaceDecl *interfaceDecl = Result.Nodes.getNodeAs<ObjCInterfaceDecl>("ObjCInterfaceDecl")) {
                
                cout << "ObjCInterfaceDecl -- ObjCInterfaceDecl" << endl;
                checkInterfaceDecl(interfaceDecl);
                
            } else if (const ObjCMethodDecl *decl = Result.Nodes.getNodeAs<ObjCMethodDecl>("ObjCMethodDecl")) {
                
                cout << "ObjCMethodDecl -- ObjCMethodDecl" << endl;
                checkMethodNameForUppercaseName(decl);
                
            } else if (const BinaryOperator *binaryOperator = Result.Nodes.getNodeAs<BinaryOperator>("binaryOperator_modelOfClass")) {
                
//                checkAppointedMethod(binaryOperator);
                
            } else if (const IfStmt *stmtIf = Result.Nodes.getNodeAs<IfStmt>("ifStmt_empty_then_body")) {
                
                SourceLocation location = stmtIf->getIfLoc();
                diagWaringReport(location, "Don't use empty body in IfStmt", NULL);
                
            } else if (const IfStmt *stmtIf = Result.Nodes.getNodeAs<IfStmt>("condition_always_true")) {
                
                SourceLocation location = stmtIf->getIfLoc();
                diagWaringReport(location, "Body will certainly be executed when condition true", NULL);
                
            } else if (const IfStmt *stmtIf = Result.Nodes.getNodeAs<IfStmt>("condition_always_false")) {
                
                SourceLocation location = stmtIf->getIfLoc();
                diagWaringReport(location, "Body will never be executed when condition false.", NULL);
                
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
        matcher.addMatcher(binaryOperator(hasDescendant(opaqueValueExpr(hasSourceExpression(objcMessageExpr(hasSelector("modelOfClass:"))))),isExpansionInMainFile()).bind("binaryOperator_modelOfClass"), &handler);
        matcher.addMatcher(ifStmt().bind("ifStmt_empty_then_body"), &handler);
        
//            matcher.addMatcher(ifStmt(isExpansionInMainFile(),hasCondition(integerLiteral(equals(1)))).bind("condition_always_true"), &handler);
//            matcher.addMatcher(ifStmt(isExpansionInMainFile(),hasCondition(floatLiteral(equals(0.0)))).bind("condition_always_false"), &handler);
//            matcher.addMatcher(ifStmt(isExpansionInMainFile(),hasCondition(integerLiteral(equals(0)))).bind("condition_always_false"), &handler);
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


