//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#ifndef _PARSER_HELPER_INCLUDED_
#define _PARSER_HELPER_INCLUDED_

#include "compiler/translator/Compiler.h"
#include "compiler/translator/Diagnostics.h"
#include "compiler/translator/DirectiveHandler.h"
#include "compiler/translator/Intermediate.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/preprocessor/Preprocessor.h"

struct TMatrixFields {
    bool wholeRow;
    bool wholeCol;
    int row;
    int col;
};

//
// The following are extra variables needed during parsing, grouped together so
// they can be passed to the parser without needing a global.
//
struct TParseContext {
    TParseContext(TSymbolTable& symt, TExtensionBehavior& ext, TIntermediate& interm, sh::GLenum type, ShShaderSpec spec, int options, bool checksPrecErrors, const char* sourcePath, TInfoSink& is) :
            intermediate(interm),
            symbolTable(symt),
            shaderType(type),
            shaderSpec(spec),
            compileOptions(options),
            sourcePath(sourcePath),
            treeRoot(0),
            loopNestingLevel(0),
            structNestingLevel(0),
            currentFunctionType(NULL),
            functionReturnsValue(false),
            checksPrecisionErrors(checksPrecErrors),
            defaultMatrixPacking(EmpColumnMajor),
            defaultBlockStorage(EbsShared),
            diagnostics(is),
            shaderVersion(100),
            directiveHandler(ext, diagnostics, shaderVersion),
            preprocessor(&diagnostics, &directiveHandler),
            scanner(NULL) {  }
    TIntermediate& intermediate; // to hold and build a parse tree
    TSymbolTable& symbolTable;   // symbol table that goes with the language currently being parsed
    sh::GLenum shaderType;              // vertex or fragment language (future: pack or unpack)
    ShShaderSpec shaderSpec;              // The language specification compiler conforms to - GLES2 or WebGL.
    int shaderVersion;
    int compileOptions;
    const char* sourcePath;      // Path of source file or NULL.
    TIntermNode* treeRoot;       // root of parse tree being created
    int loopNestingLevel;        // 0 if outside all loops
    int structNestingLevel;      // incremented while parsing a struct declaration
    const TType* currentFunctionType;  // the return type of the function that's currently being parsed
    bool functionReturnsValue;   // true if a non-void function has a return
    bool checksPrecisionErrors;  // true if an error will be generated when a variable is declared without precision, explicit or implicit.
    bool fragmentPrecisionHigh;  // true if highp precision is supported in the fragment language.
    TLayoutMatrixPacking defaultMatrixPacking;
    TLayoutBlockStorage defaultBlockStorage;
    TString HashErrMsg;
    TDiagnostics diagnostics;
    TDirectiveHandler directiveHandler;
    pp::Preprocessor preprocessor;
    void* scanner;

    int getShaderVersion() const { return shaderVersion; }
    int numErrors() const { return diagnostics.numErrors(); }
    TInfoSink& infoSink() { return diagnostics.infoSink(); }
    void error(const TSourceLoc& loc, const char *reason, const char* token,
               const char* extraInfo="");
    void warning(const TSourceLoc& loc, const char* reason, const char* token,
                 const char* extraInfo="");
    void trace(const char* str);
    void recover();

    // This method is guaranteed to succeed, even if no variable with 'name' exists.
    const TVariable *getNamedVariable(const TSourceLoc &location, const TString *name, const TSymbol *symbol);

    bool parseVectorFields(const TString&, int vecSize, TVectorFields&, const TSourceLoc& line);
    bool parseMatrixFields(const TString&, int matCols, int matRows, TMatrixFields&, const TSourceLoc& line);

    bool reservedErrorCheck(const TSourceLoc& line, const TString& identifier);
    void assignError(const TSourceLoc& line, const char* op, TString left, TString right);
    void unaryOpError(const TSourceLoc& line, const char* op, TString operand);
    void binaryOpError(const TSourceLoc& line, const char* op, TString left, TString right);
    bool precisionErrorCheck(const TSourceLoc& line, TPrecision precision, TBasicType type);
    bool lValueErrorCheck(const TSourceLoc& line, const char* op, TIntermTyped*);
    bool constErrorCheck(TIntermTyped* node);
    bool integerErrorCheck(TIntermTyped* node, const char* token);
    bool globalErrorCheck(const TSourceLoc& line, bool global, const char* token);
    bool constructorErrorCheck(const TSourceLoc& line, TIntermNode*, TFunction&, TOperator, TType*);
    bool arraySizeErrorCheck(const TSourceLoc& line, TIntermTyped* expr, int& size);
    bool arrayQualifierErrorCheck(const TSourceLoc& line, TPublicType type);
    bool arrayTypeErrorCheck(const TSourceLoc& line, TPublicType type);
    bool arrayErrorCheck(const TSourceLoc& line, const TString& identifier, const TPublicType &type, TVariable*& variable);
    bool voidErrorCheck(const TSourceLoc&, const TString&, const TPublicType&);
    bool boolErrorCheck(const TSourceLoc&, const TIntermTyped*);
    bool boolErrorCheck(const TSourceLoc&, const TPublicType&);
    bool samplerErrorCheck(const TSourceLoc& line, const TPublicType& pType, const char* reason);
    bool structQualifierErrorCheck(const TSourceLoc& line, const TPublicType& pType);
    bool locationDeclaratorListCheck(const TSourceLoc& line, const TPublicType &pType);
    bool parameterSamplerErrorCheck(const TSourceLoc& line, TQualifier qualifier, const TType& type);
    bool nonInitConstErrorCheck(const TSourceLoc& line, const TString& identifier, TPublicType& type, bool array);
    bool nonInitErrorCheck(const TSourceLoc& line, const TString& identifier, const TPublicType& type, TVariable*& variable);
    bool paramErrorCheck(const TSourceLoc& line, TQualifier qualifier, TQualifier paramQualifier, TType* type);
    bool extensionErrorCheck(const TSourceLoc& line, const TString&);
    bool singleDeclarationErrorCheck(TPublicType &publicType, const TSourceLoc& identifierLocation, const TString &identifier);
    bool layoutLocationErrorCheck(const TSourceLoc& location, const TLayoutQualifier &layoutQualifier);

    const TPragma& pragma() const { return directiveHandler.pragma(); }
    const TExtensionBehavior& extensionBehavior() const { return directiveHandler.extensionBehavior(); }
    bool supportsExtension(const char* extension);
    bool isExtensionEnabled(const char* extension) const;
    void handleExtensionDirective(const TSourceLoc& loc, const char* extName, const char* behavior);
    void handlePragmaDirective(const TSourceLoc& loc, const char* name, const char* value, bool stdgl);

    bool containsSampler(TType& type);
    bool areAllChildConst(TIntermAggregate* aggrNode);
    const TFunction* findFunction(const TSourceLoc& line, TFunction* pfnCall, int shaderVersion, bool *builtIn = 0);
    bool executeInitializer(const TSourceLoc& line, const TString& identifier, TPublicType& pType,
                            TIntermTyped* initializer, TIntermNode*& intermNode, TVariable* variable = 0);

    TPublicType addFullySpecifiedType(TQualifier qualifier, const TPublicType& typeSpecifier);
    TPublicType addFullySpecifiedType(TQualifier qualifier, TLayoutQualifier layoutQualifier, const TPublicType& typeSpecifier);
    TIntermAggregate* parseSingleDeclaration(TPublicType &publicType, const TSourceLoc& identifierLocation, const TString &identifier);
    TIntermAggregate* parseSingleArrayDeclaration(TPublicType &publicType, const TSourceLoc& identifierLocation, const TString &identifier, const TSourceLoc& indexLocation, TIntermTyped *indexExpression);
    TIntermAggregate* parseSingleInitDeclaration(TPublicType &publicType, const TSourceLoc& identifierLocation, const TString &identifier, const TSourceLoc& initLocation, TIntermTyped *initializer);
    TIntermAggregate* parseInvariantDeclaration(const TSourceLoc &invariantLoc, const TSourceLoc &identifierLoc, const TString *identifier, const TSymbol *symbol);

    TIntermAggregate* parseDeclarator(TPublicType &publicType, TIntermAggregate *aggregateDeclaration, TSymbol *identifierSymbol, const TSourceLoc& identifierLocation, const TString &identifier);
    TIntermAggregate* parseArrayDeclarator(TPublicType &publicType, const TSourceLoc& identifierLocation, const TString &identifier, const TSourceLoc& arrayLocation, TIntermNode *declaratorList, TIntermTyped *indexExpression);
    TIntermAggregate* parseInitDeclarator(TPublicType &publicType, TIntermAggregate *declaratorList, const TSourceLoc& identifierLocation, const TString &identifier, const TSourceLoc& initLocation, TIntermTyped *initializer);
    void parseGlobalLayoutQualifier(const TPublicType &typeQualifier);
    TFunction *addConstructorFunc(TPublicType publicType);
    TIntermTyped* addConstructor(TIntermNode*, const TType*, TOperator, TFunction*, const TSourceLoc&);
    TIntermTyped* foldConstConstructor(TIntermAggregate* aggrNode, const TType& type);
    TIntermTyped* addConstVectorNode(TVectorFields&, TIntermTyped*, const TSourceLoc&);
    TIntermTyped* addConstMatrixNode(int , TIntermTyped*, const TSourceLoc&);
    TIntermTyped* addConstArrayNode(int index, TIntermTyped* node, const TSourceLoc& line);
    TIntermTyped* addConstStruct(const TString &identifier, TIntermTyped *node, const TSourceLoc& line);
    TIntermTyped* addIndexExpression(TIntermTyped *baseExpression, const TSourceLoc& location, TIntermTyped *indexExpression);
    TIntermTyped* addFieldSelectionExpression(TIntermTyped *baseExpression, const TSourceLoc& dotLocation, const TString &fieldString, const TSourceLoc& fieldLocation);

    TFieldList *addStructDeclaratorList(const TPublicType& typeSpecifier, TFieldList *fieldList);
    TPublicType addStructure(const TSourceLoc& structLine, const TSourceLoc& nameLine, const TString *structName, TFieldList* fieldList);

    TIntermAggregate* addInterfaceBlock(const TPublicType& typeQualifier, const TSourceLoc& nameLine, const TString& blockName, TFieldList* fieldList, 
                                        const TString* instanceName, const TSourceLoc& instanceLine, TIntermTyped* arrayIndex, const TSourceLoc& arrayIndexLine);

    TLayoutQualifier parseLayoutQualifier(const TString &qualifierType, const TSourceLoc& qualifierTypeLine);
    TLayoutQualifier parseLayoutQualifier(const TString &qualifierType, const TSourceLoc& qualifierTypeLine, const TString &intValueString, int intValue, const TSourceLoc& intValueLine);
    TLayoutQualifier joinLayoutQualifiers(TLayoutQualifier leftQualifier, TLayoutQualifier rightQualifier);
    TPublicType joinInterpolationQualifiers(const TSourceLoc &interpolationLoc, TQualifier interpolationQualifier,
                                            const TSourceLoc &storageLoc, TQualifier storageQualifier);

    // Performs an error check for embedded struct declarations.
    // Returns true if an error was raised due to the declaration of
    // this struct.
    bool enterStructDeclaration(const TSourceLoc& line, const TString& identifier);
    void exitStructDeclaration();

    bool structNestingErrorCheck(const TSourceLoc& line, const TField& field);
};

int PaParseStrings(size_t count, const char* const string[], const int length[],
                   TParseContext* context);

#endif // _PARSER_HELPER_INCLUDED_
