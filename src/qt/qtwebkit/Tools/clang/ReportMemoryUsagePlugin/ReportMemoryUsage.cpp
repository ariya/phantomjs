/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace {

using namespace std;
typedef vector<string> Strings;

Strings instrumentationMethods;
const char instrumentingMethodName[] = "reportMemoryUsage";

class AddMemberCallVisitor : public RecursiveASTVisitor<AddMemberCallVisitor> {
public:

    bool VisitCallExpr(CallExpr*);
    const Strings& instrumentedMembers() const { return m_instrumentedMembers; }

private:
    Strings m_instrumentedMembers;
};

class ReportMemoryUsageVisitor : public RecursiveASTVisitor<ReportMemoryUsageVisitor> {
public:
    explicit ReportMemoryUsageVisitor(CompilerInstance& instance, ASTContext* context)
        : m_instance(instance)
        , m_context(context) { }

    bool VisitCXXMethodDecl(clang::CXXMethodDecl*);

private:
    void emitWarning(SourceLocation, const char* rawError);
    CXXMethodDecl* findInstrumentationMethod(CXXRecordDecl*);
    bool needsToBeInstrumented(const Type*);
    void checkMembersCoverage(const CXXRecordDecl* instrumentedClass, const Strings& instrumentedMembers, SourceLocation);

    CompilerInstance& m_instance;
    ASTContext* m_context;
};

class ReportMemoryUsageConsumer : public ASTConsumer {
public:
    explicit ReportMemoryUsageConsumer(CompilerInstance& instance, ASTContext* context)
        : m_visitor(instance, context)
    {
        instrumentationMethods.push_back("addMember");
        instrumentationMethods.push_back("addRawBuffer");
        instrumentationMethods.push_back("addWeakPointer");
        instrumentationMethods.push_back("ignoreMember");
    }

    virtual void HandleTranslationUnit(clang::ASTContext& context)
    {
        m_visitor.TraverseDecl(context.getTranslationUnitDecl());
    }

private:
    ReportMemoryUsageVisitor m_visitor;
};

class ReportMemoryUsageAction : public PluginASTAction {
protected:
    ASTConsumer* CreateASTConsumer(CompilerInstance& CI, llvm::StringRef)
    {
        return new ReportMemoryUsageConsumer(CI, &CI.getASTContext());
    }

    bool ParseArgs(const CompilerInstance& CI, const vector<string>& args)
    {
        if (args.size() && args[0] == "help")
            llvm::errs() << m_helpText;
        return true;
    }

    static const char* m_helpText;
};

const char* ReportMemoryUsageAction::m_helpText =
    "This plugin is checking native memory instrumentation code.\n"
    "The class is instrumented if it has reportMemoryUsage member function.\n"
    "Sample:\n"
    "class InstrumentedClass : public BaseInstrumentedClass {\n"
    "public:\n"
    "    void reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const\n"
    "    {\n"
    "        MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::DOM);\n"
    "        BaseInstrumentedClass::reportMemoryUsage(memoryObjectInfo);\n"
    "        info.addMember(m_notInstrumentedObject);\n"
    "        info.addMember(m_instrumentedObject);\n"
    "        info.addRawBuffer(m_pointer, m_length);\n"
    "        info.ignoreMember(m_pointerToInternalField);\n"
    "    }\n"
    "\n"
    "private:\n"
    "    NotInstrumentedClass* m_notInstrumentedPtr;\n"
    "    InstrumentedClass m_instrumentedObject;\n"
    "    long m_length;\n"
    "    double* m_pointer;\n"
    "    Data* m_pointerToInternalField;\n"
    "}\n";

bool AddMemberCallVisitor::VisitCallExpr(CallExpr* callExpr)
{
    CXXMemberCallExpr* methodCallExpr = dyn_cast<CXXMemberCallExpr>(callExpr);
    bool instrumented = false;
    if (methodCallExpr) {
        string methodName = methodCallExpr->getMethodDecl()->getNameAsString();
        Strings::iterator i = find(instrumentationMethods.begin(), instrumentationMethods.end(), methodName);
        instrumented = i != instrumentationMethods.end();
    }
    if (instrumented || !methodCallExpr) {
        for (CallExpr::arg_iterator i = callExpr->arg_begin(); i != callExpr->arg_end(); ++i) {
            Expr* expr = *i;
            while (ImplicitCastExpr::classof(expr))
                expr = static_cast<ImplicitCastExpr*>(expr)->getSubExpr();
            if (MemberExpr* memberExpr = dyn_cast<MemberExpr>(expr))
                m_instrumentedMembers.push_back(memberExpr->getMemberNameInfo().getAsString());
        }
    }
    return true;
}

bool ReportMemoryUsageVisitor::VisitCXXMethodDecl(clang::CXXMethodDecl* decl)
{
    if (decl->doesThisDeclarationHaveABody() && decl->getNameAsString() == instrumentingMethodName) {
        FullSourceLoc fullLocation = m_context->getFullLoc(decl->getLocStart());
        if (fullLocation.isValid()) {
            AddMemberCallVisitor visitor;
            visitor.TraverseStmt(decl->getBody());
            checkMembersCoverage(decl->getParent(), visitor.instrumentedMembers(), decl->getLocStart());
        }
    }
    return true;
}

void ReportMemoryUsageVisitor::emitWarning(SourceLocation loc, const char* rawError)
{
    FullSourceLoc full(loc, m_instance.getSourceManager());
    string err("[webkit-style] ");
    err += rawError;
    DiagnosticsEngine& diagnostic = m_instance.getDiagnostics();
    DiagnosticsEngine::Level level = diagnostic.getWarningsAsErrors() ? DiagnosticsEngine::Error : DiagnosticsEngine::Warning;
    unsigned id = diagnostic.getCustomDiagID(level, err);
    DiagnosticBuilder builder = diagnostic.Report(full, id);
}

CXXMethodDecl* ReportMemoryUsageVisitor::findInstrumentationMethod(CXXRecordDecl* record)
{
    for (CXXRecordDecl::method_iterator m = record->method_begin(); m != record->method_end(); ++m) {
        if (m->getNameInfo().getAsString() == instrumentingMethodName)
            return *m;
    }
    return 0;
}

bool ReportMemoryUsageVisitor::needsToBeInstrumented(const Type* type)
{
    if (type->isBuiltinType())
        return false;
    if (type->isEnumeralType())
        return false;
    if (type->isClassType()) {
        const RecordType* recordType = dyn_cast<RecordType>(type);
        if (recordType) {
            CXXRecordDecl* decl = dyn_cast<CXXRecordDecl>(recordType->getDecl());
            if (decl->getNameAsString() == "String")
                return true;
            if (!decl || !findInstrumentationMethod(decl))
                return false;
        }
    }
    if (type->isArrayType()) {
        const ArrayType* arrayType = dyn_cast<const ArrayType>(type);
        return needsToBeInstrumented(arrayType->getElementType().getTypePtr());
    }
    return true;
}

void ReportMemoryUsageVisitor::checkMembersCoverage(const CXXRecordDecl* instrumentedClass, const Strings& instrumentedMembers, SourceLocation location)
{
    for (CXXRecordDecl::field_iterator i = instrumentedClass->field_begin(); i != instrumentedClass->field_end(); ++i) {
        string fieldName = i->getNameAsString();
        if (find(instrumentedMembers.begin(), instrumentedMembers.end(), fieldName) == instrumentedMembers.end()) {
            if (!needsToBeInstrumented(i->getType().getTypePtr()))
                continue;
            emitWarning(i->getSourceRange().getBegin(), "class member needs to be instrumented in reportMemoryUsage function");
            emitWarning(location, "located here");
        }
    }
}

} // namespace

static FrontendPluginRegistry::Add<ReportMemoryUsageAction>
X("report-memory-usage", "Checks reportMemoryUsage function consistency");

