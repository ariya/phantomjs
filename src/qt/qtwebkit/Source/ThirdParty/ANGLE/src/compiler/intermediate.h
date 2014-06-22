//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

//
// Definition of the in-memory high-level intermediate representation
// of shaders.  This is a tree that parser creates.
//
// Nodes in the tree are defined as a hierarchy of classes derived from 
// TIntermNode. Each is a node in a tree.  There is no preset branching factor;
// each node can have it's own type of list of children.
//

#ifndef __INTERMEDIATE_H
#define __INTERMEDIATE_H

#include "GLSLANG/ShaderLang.h"

#include <algorithm>
#include "compiler/Common.h"
#include "compiler/Types.h"
#include "compiler/ConstantUnion.h"

//
// Operators used by the high-level (parse tree) representation.
//
enum TOperator {
    EOpNull,            // if in a node, should only mean a node is still being built
    EOpSequence,        // denotes a list of statements, or parameters, etc.
    EOpFunctionCall,    
    EOpFunction,        // For function definition
    EOpParameters,      // an aggregate listing the parameters to a function

    EOpDeclaration,
    EOpPrototype,

    //
    // Unary operators
    //

    EOpNegative,
    EOpLogicalNot,
    EOpVectorLogicalNot,

    EOpPostIncrement,
    EOpPostDecrement,
    EOpPreIncrement,
    EOpPreDecrement,

    EOpConvIntToBool,
    EOpConvFloatToBool,
    EOpConvBoolToFloat,
    EOpConvIntToFloat,
    EOpConvFloatToInt,
    EOpConvBoolToInt,

    //
    // binary operations
    //

    EOpAdd,
    EOpSub,
    EOpMul,
    EOpDiv,
    EOpEqual,
    EOpNotEqual,
    EOpVectorEqual,
    EOpVectorNotEqual,
    EOpLessThan,
    EOpGreaterThan,
    EOpLessThanEqual,
    EOpGreaterThanEqual,
    EOpComma,

    EOpVectorTimesScalar,
    EOpVectorTimesMatrix,
    EOpMatrixTimesVector,
    EOpMatrixTimesScalar,

    EOpLogicalOr,
    EOpLogicalXor,
    EOpLogicalAnd,

    EOpIndexDirect,
    EOpIndexIndirect,
    EOpIndexDirectStruct,

    EOpVectorSwizzle,

    //
    // Built-in functions potentially mapped to operators
    //

    EOpRadians,
    EOpDegrees,
    EOpSin,
    EOpCos,
    EOpTan,
    EOpAsin,
    EOpAcos,
    EOpAtan,

    EOpPow,
    EOpExp,
    EOpLog,
    EOpExp2,
    EOpLog2,
    EOpSqrt,
    EOpInverseSqrt,

    EOpAbs,
    EOpSign,
    EOpFloor,
    EOpCeil,
    EOpFract,
    EOpMod,
    EOpMin,
    EOpMax,
    EOpClamp,
    EOpMix,
    EOpStep,
    EOpSmoothStep,

    EOpLength,
    EOpDistance,
    EOpDot,
    EOpCross,
    EOpNormalize,
    EOpFaceForward,
    EOpReflect,
    EOpRefract,

    EOpDFdx,            // Fragment only, OES_standard_derivatives extension
    EOpDFdy,            // Fragment only, OES_standard_derivatives extension
    EOpFwidth,          // Fragment only, OES_standard_derivatives extension

    EOpMatrixTimesMatrix,

    EOpAny,
    EOpAll,

    //
    // Branch
    //

    EOpKill,            // Fragment only
    EOpReturn,
    EOpBreak,
    EOpContinue,

    //
    // Constructors
    //

    EOpConstructInt,
    EOpConstructBool,
    EOpConstructFloat,
    EOpConstructVec2,
    EOpConstructVec3,
    EOpConstructVec4,
    EOpConstructBVec2,
    EOpConstructBVec3,
    EOpConstructBVec4,
    EOpConstructIVec2,
    EOpConstructIVec3,
    EOpConstructIVec4,
    EOpConstructMat2,
    EOpConstructMat3,
    EOpConstructMat4,
    EOpConstructStruct,

    //
    // moves
    //

    EOpAssign,
    EOpInitialize,
    EOpAddAssign,
    EOpSubAssign,
    EOpMulAssign,
    EOpVectorTimesMatrixAssign,
    EOpVectorTimesScalarAssign,
    EOpMatrixTimesScalarAssign,
    EOpMatrixTimesMatrixAssign,
    EOpDivAssign
};

extern const char* getOperatorString(TOperator op);

class TIntermTraverser;
class TIntermAggregate;
class TIntermBinary;
class TIntermUnary;
class TIntermConstantUnion;
class TIntermSelection;
class TIntermTyped;
class TIntermSymbol;
class TIntermLoop;
class TInfoSink;

//
// Base class for the tree nodes
//
class TIntermNode {
public:
    POOL_ALLOCATOR_NEW_DELETE(GlobalPoolAllocator)

    TIntermNode() {
        // TODO: Move this to TSourceLoc constructor
        // after getting rid of TPublicType.
        line.first_file = line.last_file = 0;
        line.first_line = line.last_line = 0;
    }
    virtual ~TIntermNode() { }

    const TSourceLoc& getLine() const { return line; }
    void setLine(const TSourceLoc& l) { line = l; }

    virtual void traverse(TIntermTraverser*) = 0;
    virtual TIntermTyped* getAsTyped() { return 0; }
    virtual TIntermConstantUnion* getAsConstantUnion() { return 0; }
    virtual TIntermAggregate* getAsAggregate() { return 0; }
    virtual TIntermBinary* getAsBinaryNode() { return 0; }
    virtual TIntermUnary* getAsUnaryNode() { return 0; }
    virtual TIntermSelection* getAsSelectionNode() { return 0; }
    virtual TIntermSymbol* getAsSymbolNode() { return 0; }
    virtual TIntermLoop* getAsLoopNode() { return 0; }

protected:
    TSourceLoc line;
};

//
// This is just to help yacc.
//
struct TIntermNodePair {
    TIntermNode* node1;
    TIntermNode* node2;
};

//
// Intermediate class for nodes that have a type.
//
class TIntermTyped : public TIntermNode {
public:
    TIntermTyped(const TType& t) : type(t)  { }
    virtual TIntermTyped* getAsTyped() { return this; }

    void setType(const TType& t) { type = t; }
    const TType& getType() const { return type; }
    TType* getTypePointer() { return &type; }

    TBasicType getBasicType() const { return type.getBasicType(); }
    TQualifier getQualifier() const { return type.getQualifier(); }
    TPrecision getPrecision() const { return type.getPrecision(); }
    int getNominalSize() const { return type.getNominalSize(); }
    
    bool isMatrix() const { return type.isMatrix(); }
    bool isArray()  const { return type.isArray(); }
    bool isVector() const { return type.isVector(); }
    bool isScalar() const { return type.isScalar(); }
    const char* getBasicString() const { return type.getBasicString(); }
    const char* getQualifierString() const { return type.getQualifierString(); }
    TString getCompleteString() const { return type.getCompleteString(); }

    int totalRegisterCount() const { return type.totalRegisterCount(); }
    int elementRegisterCount() const { return type.elementRegisterCount(); }
    int getArraySize() const { return type.getArraySize(); }

protected:
    TType type;
};

//
// Handle for, do-while, and while loops.
//
enum TLoopType {
    ELoopFor,
    ELoopWhile,
    ELoopDoWhile
};

class TIntermLoop : public TIntermNode {
public:
    TIntermLoop(TLoopType aType,
                TIntermNode *aInit, TIntermTyped* aCond, TIntermTyped* aExpr,
                TIntermNode* aBody) :
            type(aType),
            init(aInit),
            cond(aCond),
            expr(aExpr),
            body(aBody),
            unrollFlag(false) { }

    virtual TIntermLoop* getAsLoopNode() { return this; }
    virtual void traverse(TIntermTraverser*);

    TLoopType getType() const { return type; }
    TIntermNode* getInit() { return init; }
    TIntermTyped* getCondition() { return cond; }
    TIntermTyped* getExpression() { return expr; }
    TIntermNode* getBody() { return body; }

    void setUnrollFlag(bool flag) { unrollFlag = flag; }
    bool getUnrollFlag() { return unrollFlag; }

protected:
    TLoopType type;
    TIntermNode* init;  // for-loop initialization
    TIntermTyped* cond; // loop exit condition
    TIntermTyped* expr; // for-loop expression
    TIntermNode* body;  // loop body

    bool unrollFlag; // Whether the loop should be unrolled or not.
};

//
// Handle break, continue, return, and kill.
//
class TIntermBranch : public TIntermNode {
public:
    TIntermBranch(TOperator op, TIntermTyped* e) :
            flowOp(op),
            expression(e) { }

    virtual void traverse(TIntermTraverser*);

    TOperator getFlowOp() { return flowOp; }
    TIntermTyped* getExpression() { return expression; }

protected:
    TOperator flowOp;
    TIntermTyped* expression;  // non-zero except for "return exp;" statements
};

//
// Nodes that correspond to symbols or constants in the source code.
//
class TIntermSymbol : public TIntermTyped {
public:
    // if symbol is initialized as symbol(sym), the memory comes from the poolallocator of sym. If sym comes from
    // per process globalpoolallocator, then it causes increased memory usage per compile
    // it is essential to use "symbol = sym" to assign to symbol
    TIntermSymbol(int i, const TString& sym, const TType& t) : 
            TIntermTyped(t), id(i)  { symbol = sym; originalSymbol = sym; } 

    int getId() const { return id; }
    const TString& getSymbol() const { return symbol; }

    void setId(int newId) { id = newId; }
    void setSymbol(const TString& sym) { symbol = sym; }

    const TString& getOriginalSymbol() const { return originalSymbol; }

    virtual void traverse(TIntermTraverser*);
    virtual TIntermSymbol* getAsSymbolNode() { return this; }

protected:
    int id;
    TString symbol;
    TString originalSymbol;
};

class TIntermConstantUnion : public TIntermTyped {
public:
    TIntermConstantUnion(ConstantUnion *unionPointer, const TType& t) : TIntermTyped(t), unionArrayPointer(unionPointer) { }

    ConstantUnion* getUnionArrayPointer() const { return unionArrayPointer; }
    
    int getIConst(size_t index) const { return unionArrayPointer ? unionArrayPointer[index].getIConst() : 0; }
    float getFConst(size_t index) const { return unionArrayPointer ? unionArrayPointer[index].getFConst() : 0.0f; }
    bool getBConst(size_t index) const { return unionArrayPointer ? unionArrayPointer[index].getBConst() : false; }

    virtual TIntermConstantUnion* getAsConstantUnion()  { return this; }
    virtual void traverse(TIntermTraverser*);

    TIntermTyped* fold(TOperator, TIntermTyped*, TInfoSink&);

protected:
    ConstantUnion *unionArrayPointer;
};

//
// Intermediate class for node types that hold operators.
//
class TIntermOperator : public TIntermTyped {
public:
    TOperator getOp() const { return op; }
    void setOp(TOperator o) { op = o; }

    bool modifiesState() const;
    bool isConstructor() const;

protected:
    TIntermOperator(TOperator o) : TIntermTyped(TType(EbtFloat, EbpUndefined)), op(o) {}
    TIntermOperator(TOperator o, TType& t) : TIntermTyped(t), op(o) {}   
    TOperator op;
};

//
// Nodes for all the basic binary math operators.
//
class TIntermBinary : public TIntermOperator {
public:
    TIntermBinary(TOperator o) : TIntermOperator(o), addIndexClamp(false) {}

    virtual TIntermBinary* getAsBinaryNode() { return this; }
    virtual void traverse(TIntermTraverser*);

    void setLeft(TIntermTyped* n) { left = n; }
    void setRight(TIntermTyped* n) { right = n; }
    TIntermTyped* getLeft() const { return left; }
    TIntermTyped* getRight() const { return right; }
    bool promote(TInfoSink&);

    void setAddIndexClamp() { addIndexClamp = true; }
    bool getAddIndexClamp() { return addIndexClamp; }

protected:
    TIntermTyped* left;
    TIntermTyped* right;

    // If set to true, wrap any EOpIndexIndirect with a clamp to bounds.
    bool addIndexClamp;
};

//
// Nodes for unary math operators.
//
class TIntermUnary : public TIntermOperator {
public:
    TIntermUnary(TOperator o, TType& t) : TIntermOperator(o, t), operand(0), useEmulatedFunction(false) {}
    TIntermUnary(TOperator o) : TIntermOperator(o), operand(0), useEmulatedFunction(false) {}

    virtual void traverse(TIntermTraverser*);
    virtual TIntermUnary* getAsUnaryNode() { return this; }

    void setOperand(TIntermTyped* o) { operand = o; }
    TIntermTyped* getOperand() { return operand; }    
    bool promote(TInfoSink&);

    void setUseEmulatedFunction() { useEmulatedFunction = true; }
    bool getUseEmulatedFunction() { return useEmulatedFunction; }

protected:
    TIntermTyped* operand;

    // If set to true, replace the built-in function call with an emulated one
    // to work around driver bugs.
    bool useEmulatedFunction;
};

typedef TVector<TIntermNode*> TIntermSequence;
typedef TVector<int> TQualifierList;

//
// Nodes that operate on an arbitrary sized set of children.
//
class TIntermAggregate : public TIntermOperator {
public:
    TIntermAggregate() : TIntermOperator(EOpNull), userDefined(false), useEmulatedFunction(false) { }
    TIntermAggregate(TOperator o) : TIntermOperator(o), useEmulatedFunction(false) { }
    ~TIntermAggregate() { }

    virtual TIntermAggregate* getAsAggregate() { return this; }
    virtual void traverse(TIntermTraverser*);

    TIntermSequence& getSequence() { return sequence; }

    void setName(const TString& n) { name = n; }
    const TString& getName() const { return name; }

    void setUserDefined() { userDefined = true; }
    bool isUserDefined() const { return userDefined; }

    void setOptimize(bool o) { optimize = o; }
    bool getOptimize() { return optimize; }
    void setDebug(bool d) { debug = d; }
    bool getDebug() { return debug; }

    void setUseEmulatedFunction() { useEmulatedFunction = true; }
    bool getUseEmulatedFunction() { return useEmulatedFunction; }

protected:
    TIntermAggregate(const TIntermAggregate&); // disallow copy constructor
    TIntermAggregate& operator=(const TIntermAggregate&); // disallow assignment operator
    TIntermSequence sequence;
    TString name;
    bool userDefined; // used for user defined function names

    bool optimize;
    bool debug;

    // If set to true, replace the built-in function call with an emulated one
    // to work around driver bugs.
    bool useEmulatedFunction;
};

//
// For if tests.  Simplified since there is no switch statement.
//
class TIntermSelection : public TIntermTyped {
public:
    TIntermSelection(TIntermTyped* cond, TIntermNode* trueB, TIntermNode* falseB) :
            TIntermTyped(TType(EbtVoid, EbpUndefined)), condition(cond), trueBlock(trueB), falseBlock(falseB) {}
    TIntermSelection(TIntermTyped* cond, TIntermNode* trueB, TIntermNode* falseB, const TType& type) :
            TIntermTyped(type), condition(cond), trueBlock(trueB), falseBlock(falseB) {}

    virtual void traverse(TIntermTraverser*);

    bool usesTernaryOperator() const { return getBasicType() != EbtVoid; }
    TIntermNode* getCondition() const { return condition; }
    TIntermNode* getTrueBlock() const { return trueBlock; }
    TIntermNode* getFalseBlock() const { return falseBlock; }
    TIntermSelection* getAsSelectionNode() { return this; }

protected:
    TIntermTyped* condition;
    TIntermNode* trueBlock;
    TIntermNode* falseBlock;
};

enum Visit
{
    PreVisit,
    InVisit,
    PostVisit
};

//
// For traversing the tree.  User should derive from this, 
// put their traversal specific data in it, and then pass
// it to a Traverse method.
//
// When using this, just fill in the methods for nodes you want visited.
// Return false from a pre-visit to skip visiting that node's subtree.
//
class TIntermTraverser
{
public:
    POOL_ALLOCATOR_NEW_DELETE(GlobalPoolAllocator)

    TIntermTraverser(bool preVisit = true, bool inVisit = false, bool postVisit = false, bool rightToLeft = false) : 
            preVisit(preVisit),
            inVisit(inVisit),
            postVisit(postVisit),
            rightToLeft(rightToLeft),
            depth(0),
            maxDepth(0) {}
    virtual ~TIntermTraverser() {};

    virtual void visitSymbol(TIntermSymbol*) {}
    virtual void visitConstantUnion(TIntermConstantUnion*) {}
    virtual bool visitBinary(Visit visit, TIntermBinary*) {return true;}
    virtual bool visitUnary(Visit visit, TIntermUnary*) {return true;}
    virtual bool visitSelection(Visit visit, TIntermSelection*) {return true;}
    virtual bool visitAggregate(Visit visit, TIntermAggregate*) {return true;}
    virtual bool visitLoop(Visit visit, TIntermLoop*) {return true;}
    virtual bool visitBranch(Visit visit, TIntermBranch*) {return true;}

    int getMaxDepth() const {return maxDepth;}
    void incrementDepth() {depth++; maxDepth = std::max(maxDepth, depth); }
    void decrementDepth() {depth--;}

    // Return the original name if hash function pointer is NULL;
    // otherwise return the hashed name.
    static TString hash(const TString& name, ShHashFunction64 hashFunction);

    const bool preVisit;
    const bool inVisit;
    const bool postVisit;
    const bool rightToLeft;

protected:
    int depth;
    int maxDepth;
};

#endif // __INTERMEDIATE_H
