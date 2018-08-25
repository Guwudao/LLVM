

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

//for code checker
#include <stdio.h>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <vector>
#include "clang/Rewrite/Core/Rewriter.h"

using namespace clang;
using namespace std;
using namespace llvm;
using namespace clang::ast_matchers;


namespace JJPlugin {
    
    // MARK: - my handler
    class JJHandler : public MatchFinder::MatchCallback {
    private:
        CompilerInstance &ci;
        ASTContext *context;
        
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
            
//        matcher.addMatcher(ifStmt(isExpansionInMainFile(),hasThen(compoundStmt(statementCountIs(0)))).bind("ifStmt_empty_then_body"), &handler);
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
 

//namespace
//{
//    static vector<string> split(const string &s, char delim)
//    {
//        vector<string> elems;
//        stringstream ss;
//        ss.str(s);
//        string item;
//        while (getline(ss, item, delim)) {
//            elems.push_back(item);
//        }
//        return elems;
//    }
//
//    class CodeVisitor : public RecursiveASTVisitor<CodeVisitor>
//    {
//    private:
//        CompilerInstance &Instance;
//        ASTContext *Context;
//
//    public:
//
//        void setASTContext (ASTContext &context)
//        {
//            this -> Context = &context;
//        }
//
//    private:
//
//        /**
//         判断是否为用户源码
//
//         @param decl 声明
//         @return true 为用户源码，false 非用户源码
//         */
//        bool isUserSourceCode (Decl *decl)
//        {
//            string filename = Instance.getSourceManager().getFilename(decl->getSourceRange().getBegin()).str();
//
//            if (filename.empty())
//                return false;
//
//            //非XCode中的源码都认为是用户源码
//            if(filename.find("/Applications/Xcode.app/") == 0)
//                return false;
//
//            return true;
//        }
//
//        /**
//         检测类名是否存在小写开头
//
//         @param decl 类声明
//         */
//        void checkClassNameForLowercaseName(ObjCInterfaceDecl *decl)
//        {
//            StringRef className = decl -> getName();
//
//            //类名称必须以大写字母开头
//            char c = className[0];
//            if (isLowercase(c))
//            {
//                //修正提示
//                std::string tempName = className;
//                tempName[0] = toUppercase(c);
//                StringRef replacement(tempName);
//                SourceLocation nameStart = decl->getLocation();
//                SourceLocation nameEnd = nameStart.getLocWithOffset(className.size() - 1);
//                FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(nameStart, nameEnd), replacement);
//
//                //报告警告
//                DiagnosticsEngine &D = Instance.getDiagnostics();
//                int diagID = D.getCustomDiagID(DiagnosticsEngine::Error, "Class name should not start with lowercase letter");
//                SourceLocation location = decl->getLocation();
//                D.Report(location, diagID).AddFixItHint(fixItHint);
//            }
//        }
//
//        /**
//         检测类名是否包含下划线
//
//         @param decl 类声明
//         */
//        void checkClassNameForUnderscoreInName(ObjCInterfaceDecl *decl)
//        {
//            StringRef className = decl -> getName();
//
//            //类名不能包含下划线
//            size_t underscorePos = className.find('_');
//            if (underscorePos != StringRef::npos)
//            {
//                //修正提示
//                std::string tempName = className;
//                std::string::iterator end_pos = std::remove(tempName.begin(), tempName.end(), '_');
//                tempName.erase(end_pos, tempName.end());
//                StringRef replacement(tempName);
//                SourceLocation nameStart = decl->getLocation();
//                SourceLocation nameEnd = nameStart.getLocWithOffset(className.size() - 1);
//                FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(nameStart, nameEnd), replacement);
//
//                //报告错误
//                DiagnosticsEngine &diagEngine = Instance.getDiagnostics();
//                unsigned diagID = diagEngine.getCustomDiagID(DiagnosticsEngine::Error, "Class name with `_` forbidden");
//                SourceLocation location = decl->getLocation().getLocWithOffset(underscorePos);
//                diagEngine.Report(location, diagID).AddFixItHint(fixItHint);
//            }
//        }
//
//
//        /**
//         检测方法名是否存在大写开头
//
//         @param decl 方法声明
//         */
//        void checkMethodNameForUppercaseName(ObjCMethodDecl *decl)
//        {
//            //检查名称的每部分，都不允许以大写字母开头
//            Selector sel = decl -> getSelector();
//            int selectorPartCount = decl -> getNumSelectorLocs();
//            for (int i = 0; i < selectorPartCount; i++)
//            {
//                StringRef selName = sel.getNameForSlot(i);
//                char c = selName[0];
//                if (isUppercase(c))
//                {
//                    //修正提示
//                    std::string tempName = selName;
//                    tempName[0] = toLowercase(c);
//                    StringRef replacement(tempName);
//                    SourceLocation nameStart = decl -> getSelectorLoc(i);
//                    SourceLocation nameEnd = nameStart.getLocWithOffset(selName.size() - 1);
//                    FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(nameStart, nameEnd), replacement);
//
//                    //报告警告
//                    DiagnosticsEngine &D = Instance.getDiagnostics();
//                    int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, "Selector name should not start with uppercase letter");
//                    SourceLocation location = decl->getLocation();
//                    D.Report(location, diagID).AddFixItHint(fixItHint);
//                }
//            }
//        }
//
//        /**
//         检测方法中定义的参数名称是否存在大写开头
//
//         @param decl 方法声明
//         */
//        void checkMethodParamsNameForUppercaseName(ObjCMethodDecl *decl)
//        {
//            for (ObjCMethodDecl::param_iterator it = decl -> param_begin(); it != decl -> param_end(); it++)
//            {
//                ParmVarDecl *parmVarDecl = *it;
//                StringRef name = parmVarDecl -> getName();
//                char c = name[0];
//                if (isUppercase(c))
//                {
//                    //修正提示
//                    std::string tempName = name;
//                    tempName[0] = toLowercase(c);
//                    StringRef replacement(tempName);
//                    SourceLocation nameStart = parmVarDecl -> getLocation();
//                    SourceLocation nameEnd = nameStart.getLocWithOffset(name.size() - 1);
//                    FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(nameStart, nameEnd), replacement);
//
//                    //报告警告
//                    DiagnosticsEngine &D = Instance.getDiagnostics();
//                    int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, "Selector's param name should not start with uppercase letter");
//                    SourceLocation location = decl->getLocation();
//                    D.Report(location, diagID).AddFixItHint(fixItHint);
//                }
//            }
//        }
//
//        /**
//         检测方法实现是否超过500行代码
//
//         @param decl 方法声明
//         */
//        void checkMethodBodyForOver500Lines(ObjCMethodDecl *decl)
//        {
//            if (decl -> hasBody())
//            {
//                //存在方法体
//                Stmt *methodBody = decl -> getBody();
//
//                string srcCode;
//                srcCode.assign(Instance.getSourceManager().getCharacterData(methodBody->getSourceRange().getBegin()),
//                               methodBody->getSourceRange().getEnd().getRawEncoding() - methodBody->getSourceRange().getBegin().getRawEncoding() + 1);
//                vector<string> lines = split(srcCode, '\n');
//                if(lines.size() > 500)
//                {
//                    DiagnosticsEngine &D = Instance.getDiagnostics();
//                    unsigned diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, "Single method should not have body over 500 lines");
//                    D.Report(decl -> getSourceRange().getBegin(), diagID);
//                }
//            }
//        }
//
//        /**
//         检测属性名是否存在大写开头
//
//         @param decl 属性声明
//         */
//        void checkPropertyNameForUppercaseName(ObjCPropertyDecl *decl)
//        {
//            bool checkUppercaseNameIndex = 0;
//
//            StringRef name = decl -> getName();
//
//            if (name.find('_') == 0)
//            {
//                //表示以下划线开头
//                checkUppercaseNameIndex = 1;
//            }
//
//            //名称必须以小写字母开头
//            char c = name[checkUppercaseNameIndex];
//            if (isUppercase(c))
//            {
//                //修正提示
//                std::string tempName = name;
//                tempName[checkUppercaseNameIndex] = toLowercase(c);
//                StringRef replacement(tempName);
//                SourceLocation nameStart = decl->getLocation();
//                SourceLocation nameEnd = nameStart.getLocWithOffset(name.size() - 1);
//                FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(nameStart, nameEnd), replacement);
//
//                //报告错误
//                DiagnosticsEngine &D = Instance.getDiagnostics();
//                int diagID = D.getCustomDiagID(DiagnosticsEngine::Error, "Property name should not start with uppercase letter");
//                SourceLocation location = decl->getLocation();
//                D.Report(location, diagID).AddFixItHint(fixItHint);
//            }
//        }
//
//        /**
//         检测属性名是否包含下划线
//
//         @param decl 属性声明
//         */
//        void checkPropertyNameForUnderscoreInName(ObjCPropertyDecl *decl)
//        {
//            StringRef name = decl -> getName();
//
//            if (name.size() == 1)
//            {
//                //不需要检测
//                return;
//            }
//
//            //类名不能包含下划线
//            size_t underscorePos = name.find('_', 1);
//            if (underscorePos != StringRef::npos)
//            {
//                //修正提示
//                std::string tempName = name;
//                std::string::iterator end_pos = std::remove(tempName.begin() + 1, tempName.end(), '_');
//                tempName.erase(end_pos, tempName.end());
//                StringRef replacement(tempName);
//                SourceLocation nameStart = decl->getLocation();
//                SourceLocation nameEnd = nameStart.getLocWithOffset(name.size() - 1);
//                FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(nameStart, nameEnd), replacement);
//
//                //报告错误
//                DiagnosticsEngine &diagEngine = Instance.getDiagnostics();
//                unsigned diagID = diagEngine.getCustomDiagID(DiagnosticsEngine::Error, "Property name with `_` forbidden");
//                SourceLocation location = decl->getLocation().getLocWithOffset(underscorePos);
//                diagEngine.Report(location, diagID).AddFixItHint(fixItHint);
//            }
//        }
//
//
//        /**
//         检测委托属性是否有使用weak修饰
//
//         @param decl 属性声明
//         */
//        void checkDelegatePropertyForUsageWeak (ObjCPropertyDecl *decl)
//        {
//            QualType type = decl -> getType();
//            StringRef typeStr = type.getAsString();
//
//            //Delegate
//            if(typeStr.find("<") != string::npos && typeStr.find(">") != string::npos)
//            {
//                ObjCPropertyDecl::PropertyAttributeKind attrKind = decl -> getPropertyAttributes();
//
//                string typeSrcCode;
//                typeSrcCode.assign(Instance.getSourceManager().getCharacterData(decl -> getSourceRange().getBegin()),
//                                   decl -> getSourceRange().getEnd().getRawEncoding() - decl -> getSourceRange().getBegin().getRawEncoding());
//
//                if(!(attrKind & ObjCPropertyDecl::OBJC_PR_weak))
//                {
//                    DiagnosticsEngine &diagEngine = Instance.getDiagnostics();
//                    unsigned diagID = diagEngine.getCustomDiagID(DiagnosticsEngine::Warning, "Delegate should be declared as weak.");
//                    diagEngine.Report(decl -> getLocation(), diagID);
//                }
//            }
//        }
//
//
//        /**
//         检测常量名称是否存在小写开头
//
//         @param decl 常量声明
//         */
//        void checkConstantNameForLowercaseName (VarDecl *decl)
//        {
//            StringRef className = decl -> getName();
//
//            //类名称必须以大写字母开头
//            char c = className[0];
//            if (isLowercase(c))
//            {
//                //修正提示
//                std::string tempName = className;
//                tempName[0] = toUppercase(c);
//                StringRef replacement(tempName);
//                SourceLocation nameStart = decl->getLocation();
//                SourceLocation nameEnd = nameStart.getLocWithOffset(className.size() - 1);
//                FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(nameStart, nameEnd), replacement);
//
//                //报告警告
//                DiagnosticsEngine &D = Instance.getDiagnostics();
//                int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, "Constant name should not start with lowercase letter");
//                SourceLocation location = decl->getLocation();
//                D.Report(location, diagID).AddFixItHint(fixItHint);
//            }
//        }
//
//
//        /**
//         检测变量名称是否存在大写开头
//
//         @param decl 变量声明
//         */
//        void checkVarNameForUppercaseName (VarDecl *decl)
//        {
//            StringRef className = decl -> getName();
//
//            //类名称必须以大写字母开头
//            char c = className[0];
//            if (isUppercase(c))
//            {
//                //修正提示
//                std::string tempName = className;
//                tempName[0] = toLowercase(c);
//                StringRef replacement(tempName);
//                SourceLocation nameStart = decl->getLocation();
//                SourceLocation nameEnd = nameStart.getLocWithOffset(className.size() - 1);
//                FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(nameStart, nameEnd), replacement);
//
//                //报告警告
//                DiagnosticsEngine &D = Instance.getDiagnostics();
//                int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, "Variable name should not start with uppercase letter");
//                SourceLocation location = decl->getLocation();
//                D.Report(location, diagID).AddFixItHint(fixItHint);
//            }
//        }
//
//
//        /**
//         检测变量名称
//
//         @param decl 变量声明
//         */
//        void checkVarName(VarDecl *decl)
//        {
//            if (decl -> isStaticLocal())
//            {
//                //静态变量
//
//                if (decl -> getType().isConstant(*this -> Context))
//                {
//                    //常量
//                    checkConstantNameForLowercaseName(decl);
//                }
//                else
//                {
//                    //非常量
//                    checkVarNameForUppercaseName(decl);
//                }
//
//            }
//            else if (decl -> isLocalVarDecl())
//            {
//                //本地变量
//                if (decl -> getType().isConstant(*this -> Context))
//                {
//                    //常量
//                    checkConstantNameForLowercaseName(decl);
//                }
//                else
//                {
//                    //非常量
//                    checkVarNameForUppercaseName(decl);
//                }
//            }
//            else if (decl -> isFileVarDecl())
//            {
//                //文件定义变量
//                if (decl -> getType().isConstant(*this -> Context))
//                {
//                    //常量
//                    checkConstantNameForLowercaseName(decl);
//                }
//                else
//                {
//                    //非常量
//                    checkVarNameForUppercaseName(decl);
//                }
//            }
//        }
//
//    public:
//
//        CodeVisitor (CompilerInstance &Instance)
//        :Instance(Instance)
//        {
//
//        }
//
//        /**
//         观察ObjC的类声明
//
//         @param declaration 声明对象
//         @return 返回
//         */
//        bool VisitObjCInterfaceDecl(ObjCInterfaceDecl *declaration)
//        {
//            cout << "---------------- VisitObjCInterfaceDecl ----------------" << endl;
//            if (isUserSourceCode(declaration))
//            {
//                checkClassNameForLowercaseName(declaration);
//                checkClassNameForUnderscoreInName(declaration);
//            }
//
//            return true;
//        }
//
//
//        /**
//         观察类方法声明
//
//         @param declaration 声明对象
//         @return 返回
//         */
//        bool VisitObjCMethodDecl(ObjCMethodDecl *declaration)
//        {
//            cout << "---------------- VisitObjCMethodDecl ----------------" << endl;
//            if (isUserSourceCode(declaration))
//            {
//                checkMethodNameForUppercaseName(declaration);
//                checkMethodParamsNameForUppercaseName(declaration);
//                checkMethodBodyForOver500Lines(declaration);
//            }
//
//            return true;
//        }
//
//
//        /**
//         观察类属性声明
//
//         @param declaration 声明对象
//         @return 返回
//         */
//        bool VisitObjCPropertyDecl(ObjCPropertyDecl *declaration)
//        {
//            cout << "---------------- VisitObjCPropertyDecl ----------------" << endl;
//            if (isUserSourceCode(declaration))
//            {
//                checkPropertyNameForUppercaseName(declaration);
//                checkPropertyNameForUnderscoreInName(declaration);
//                checkDelegatePropertyForUsageWeak(declaration);
//            }
//
//            return true;
//        }
//
//        /**
//         观察变量声明
//
//         @param declaration 声明对象
//         @return 返回
//         */
//        bool VisitVarDecl(VarDecl *declaration)
//        {
//            cout << "---------------- VisitVarDecl ----------------" << endl;
//            if (isUserSourceCode(declaration))
//            {
//                checkVarName(declaration);
//            }
//
//            return true;
//        }
//
//        /**
//         观察枚举常量声明
//
//         @param declaration 声明对象
//         @return 返回
//         */
//        //        bool VisitEnumConstantDecl (EnumConstantDecl *declaration)
//        //        {
//        //            return true;
//        //        }
//
//        bool VisitObjCMessageExpr(ObjCMessageExpr *expr){
//            DiagnosticsEngine &D = Instance.getDiagnostics();
//            int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, "Meet Msg Expr : %0");
//            D.Report(expr->getBeginLoc(), diagID) << expr->getSelector().getAsString();
//            return true;
//        }
//    };
//
//
//
//    class JJConsumer : public ASTConsumer
//    {
//        CompilerInstance &Instance;
//        std::set<std::string> ParsedTemplates;
//    public:
//        JJConsumer(CompilerInstance &Instance,
//                     std::set<std::string> ParsedTemplates)
//        : Instance(Instance), ParsedTemplates(ParsedTemplates), visitor(Instance)
//        {
//
//        }
//
//        bool HandleTopLevelDecl(DeclGroupRef DG) override
//        {
//            return true;
//        }
//
//        void HandleTranslationUnit(ASTContext& context) override
//        {
//            visitor.setASTContext(context);
//            visitor.TraverseDecl(context.getTranslationUnitDecl());
//        }
//
//    private:
//        CodeVisitor visitor;
//    };
//
//    class JJASTAction : public PluginASTAction
//    {
//        std::set<std::string> ParsedTemplates;
//    protected:
//        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
//                                                       llvm::StringRef) override
//        {
//            return llvm::make_unique<JJConsumer>(CI, ParsedTemplates);
//        }
//
//        bool ParseArgs(const CompilerInstance &CI,
//                       const std::vector<std::string> &args) override
//        {
//            DiagnosticsEngine &D = CI.getDiagnostics();
//            D.Report(D.getCustomDiagID(DiagnosticsEngine::Warning,
//                                       "My plugin Started..."));
//
//            return true;
//        }
//    };
//}

static FrontendPluginRegistry::Add<JJPlugin::JJASTAction>
X("JJPlugin", "The JJPlugin is my first clang-plugin.");

//static clang::FrontendPluginRegistry::Add<JJASTAction>
//X("JJPlugin", "Code Checker");

