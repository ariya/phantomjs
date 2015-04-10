/*
 * Copyright (C) 2011, 2012 Google Inc. All rights reserved.
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

#include "config.h"
#include "CSSCalculationValue.h"

#include "CSSValueList.h"
#include "Length.h"
#include "StyleResolver.h"

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/StringBuilder.h>

static const int maxExpressionDepth = 100;

enum ParseState {
    OK,
    TooDeep,
    NoMoreTokens
};

namespace WebCore {

static CalculationCategory unitCategory(CSSPrimitiveValue::UnitTypes type)
{
    switch (type) {
    case CSSPrimitiveValue::CSS_NUMBER:
    case CSSPrimitiveValue::CSS_PARSER_INTEGER:
        return CalcNumber;
    case CSSPrimitiveValue::CSS_PERCENTAGE:
        return CalcPercent;
    case CSSPrimitiveValue::CSS_EMS:
    case CSSPrimitiveValue::CSS_EXS:
    case CSSPrimitiveValue::CSS_PX:
    case CSSPrimitiveValue::CSS_CM:
    case CSSPrimitiveValue::CSS_MM:
    case CSSPrimitiveValue::CSS_IN:
    case CSSPrimitiveValue::CSS_PT:
    case CSSPrimitiveValue::CSS_PC:
    case CSSPrimitiveValue::CSS_REMS:
    case CSSPrimitiveValue::CSS_CHS:
        return CalcLength;
#if ENABLE(CSS_VARIABLES)
    case CSSPrimitiveValue::CSS_VARIABLE_NAME:
        return CalcVariable;
#endif
    default:
        return CalcOther;
    }
}

static String buildCssText(const String& expression)
{
    StringBuilder result;
    result.append("calc");
    bool expressionHasSingleTerm = expression[0] != '(';
    if (expressionHasSingleTerm)
        result.append('(');
    result.append(expression);
    if (expressionHasSingleTerm)
        result.append(')');
    return result.toString(); 
}

String CSSCalcValue::customCssText() const
{
    return buildCssText(m_expression->customCssText());
}

bool CSSCalcValue::equals(const CSSCalcValue& other) const
{
    return compareCSSValuePtr(m_expression, other.m_expression);
}

#if ENABLE(CSS_VARIABLES)
String CSSCalcValue::customSerializeResolvingVariables(const HashMap<AtomicString, String>& variables) const
{
    return buildCssText(m_expression->serializeResolvingVariables(variables));
}

bool CSSCalcValue::hasVariableReference() const
{
    return m_expression->hasVariableReference();
}
#endif

double CSSCalcValue::clampToPermittedRange(double value) const
{
    return m_nonNegative && value < 0 ? 0 : value;
}    
    
double CSSCalcValue::doubleValue() const 
{ 
    return clampToPermittedRange(m_expression->doubleValue());
}

double CSSCalcValue::computeLengthPx(const RenderStyle* currentStyle, const RenderStyle* rootStyle, double multiplier, bool computingFontSize) const
{
    return clampToPermittedRange(m_expression->computeLengthPx(currentStyle, rootStyle, multiplier, computingFontSize));
}
    
CSSCalcExpressionNode::~CSSCalcExpressionNode() 
{
}
    
class CSSCalcPrimitiveValue : public CSSCalcExpressionNode {
    WTF_MAKE_FAST_ALLOCATED;
public:

    static PassRefPtr<CSSCalcPrimitiveValue> create(CSSPrimitiveValue* value, bool isInteger)
    {
        return adoptRef(new CSSCalcPrimitiveValue(value, isInteger));
    }
    
    virtual bool isZero() const
    {
        return !m_value->getDoubleValue();
    }

    virtual String customCssText() const
    {
        return m_value->cssText();
    }

#if ENABLE(CSS_VARIABLES)
    virtual String serializeResolvingVariables(const HashMap<AtomicString, String>& variables) const
    {
        return m_value->customSerializeResolvingVariables(variables);
    }
    
    virtual bool hasVariableReference() const
    {
        return m_value->isVariableName();
    }
#endif

    virtual PassOwnPtr<CalcExpressionNode> toCalcValue(const RenderStyle* style, const RenderStyle* rootStyle, double zoom) const
    {
        switch (m_category) {
        case CalcNumber:
            return adoptPtr(new CalcExpressionNumber(m_value->getFloatValue()));
        case CalcLength:
            return adoptPtr(new CalcExpressionNumber(m_value->computeLength<float>(style, rootStyle, zoom)));
        case CalcPercent:
        case CalcPercentLength:
            return adoptPtr(new CalcExpressionLength(StyleResolver::convertToFloatLength(m_value.get(), style, rootStyle, zoom)));
        // Only types that could be part of a Length expression can be converted
        // to a CalcExpressionNode. CalcPercentNumber makes no sense as a Length.
        case CalcPercentNumber:
#if ENABLE(CSS_VARIABLES)
        case CalcVariable:
#endif
        case CalcOther:
            ASSERT_NOT_REACHED();
        }
        return nullptr;
    }

    virtual double doubleValue() const
    {
        switch (m_category) {
        case CalcNumber:
        case CalcPercent:                
            return m_value->getDoubleValue();
        case CalcLength:
        case CalcPercentLength:
        case CalcPercentNumber:
#if ENABLE(CSS_VARIABLES)
        case CalcVariable:
#endif
        case CalcOther:
            ASSERT_NOT_REACHED();
            break;
        }
        return 0;
    }
    
    virtual double computeLengthPx(const RenderStyle* currentStyle, const RenderStyle* rootStyle, double multiplier, bool computingFontSize) const
    {
        switch (m_category) {
        case CalcLength:
            return m_value->computeLength<double>(currentStyle, rootStyle, multiplier, computingFontSize);
        case CalcPercent:
        case CalcNumber:
            return m_value->getDoubleValue();
        case CalcPercentLength:
        case CalcPercentNumber:
#if ENABLE(CSS_VARIABLES)
        case CalcVariable:
#endif
        case CalcOther:
            ASSERT_NOT_REACHED();
            break;
        }
        return 0;        
    }

    virtual bool equals(const CSSCalcExpressionNode& other) const
    {
        if (type() != other.type())
            return false;

        return compareCSSValuePtr(m_value, static_cast<const CSSCalcPrimitiveValue&>(other).m_value);
    }

    virtual Type type() const { return CssCalcPrimitiveValue; }
    
private:
    explicit CSSCalcPrimitiveValue(CSSPrimitiveValue* value, bool isInteger)
        : CSSCalcExpressionNode(unitCategory((CSSPrimitiveValue::UnitTypes)value->primitiveType()), isInteger)
        , m_value(value)
    {
    }

    RefPtr<CSSPrimitiveValue> m_value;
};

static const CalculationCategory addSubtractResult[CalcOther][CalcOther] = {
    { CalcNumber,        CalcOther,         CalcPercentNumber, CalcPercentNumber, CalcOther },
    { CalcOther,         CalcLength,        CalcPercentLength, CalcOther,         CalcPercentLength },
    { CalcPercentNumber, CalcPercentLength, CalcPercent,       CalcPercentNumber, CalcPercentLength },
    { CalcPercentNumber, CalcOther,         CalcPercentNumber, CalcPercentNumber, CalcOther },
    { CalcOther,         CalcPercentLength, CalcPercentLength, CalcOther,         CalcPercentLength },
};    

static CalculationCategory determineCategory(const CSSCalcExpressionNode& leftSide, const CSSCalcExpressionNode& rightSide, CalcOperator op)
{
    CalculationCategory leftCategory = leftSide.category();
    CalculationCategory rightCategory = rightSide.category();

    if (leftCategory == CalcOther || rightCategory == CalcOther)
        return CalcOther;

#if ENABLE(CSS_VARIABLES)
    if (leftCategory == CalcVariable || rightCategory == CalcVariable)
        return CalcVariable;
#endif

    switch (op) {
    case CalcAdd:
    case CalcSubtract:             
        return addSubtractResult[leftCategory][rightCategory];
    case CalcMultiply:
        if (leftCategory != CalcNumber && rightCategory != CalcNumber) 
            return CalcOther;
        return leftCategory == CalcNumber ? rightCategory : leftCategory;
    case CalcDivide:
        if (rightCategory != CalcNumber || rightSide.isZero())
            return CalcOther;
        return leftCategory;
    }
    
    ASSERT_NOT_REACHED();
    return CalcOther;
}

class CSSCalcBinaryOperation : public CSSCalcExpressionNode {

public:
    static PassRefPtr<CSSCalcBinaryOperation> create(PassRefPtr<CSSCalcExpressionNode> leftSide, PassRefPtr<CSSCalcExpressionNode> rightSide, CalcOperator op)
    {
        ASSERT(leftSide->category() != CalcOther && rightSide->category() != CalcOther);
        
        CalculationCategory newCategory = determineCategory(*leftSide, *rightSide, op);

        if (newCategory == CalcOther)
            return 0;

        return adoptRef(new CSSCalcBinaryOperation(leftSide, rightSide, op, newCategory));
    }
    
    virtual bool isZero() const
    {
        return !doubleValue();
    }

    virtual PassOwnPtr<CalcExpressionNode> toCalcValue(const RenderStyle* style, const RenderStyle* rootStyle, double zoom) const
    {
        OwnPtr<CalcExpressionNode> left(m_leftSide->toCalcValue(style, rootStyle, zoom));
        if (!left)
            return nullptr;
        OwnPtr<CalcExpressionNode> right(m_rightSide->toCalcValue(style, rootStyle, zoom));
        if (!right)
            return nullptr;
        return adoptPtr(new CalcExpressionBinaryOperation(left.release(), right.release(), m_operator));
    }

    virtual double doubleValue() const 
    {
        return evaluate(m_leftSide->doubleValue(), m_rightSide->doubleValue());
    }
    
    virtual double computeLengthPx(const RenderStyle* currentStyle, const RenderStyle* rootStyle, double multiplier, bool computingFontSize) const
    {
        const double leftValue = m_leftSide->computeLengthPx(currentStyle, rootStyle, multiplier, computingFontSize);
        const double rightValue = m_rightSide->computeLengthPx(currentStyle, rootStyle, multiplier, computingFontSize);
        return evaluate(leftValue, rightValue);
    }

    static String buildCssText(const String& leftExpression, const String& rightExpression, CalcOperator op)
    {
        StringBuilder result;
        result.append('(');
        result.append(leftExpression);
        result.append(' ');
        result.append(static_cast<char>(op));
        result.append(' ');
        result.append(rightExpression);
        result.append(')');
    
        return result.toString();  
    }

    virtual String customCssText() const
    {
        return buildCssText(m_leftSide->customCssText(), m_rightSide->customCssText(), m_operator);
    }

#if ENABLE(CSS_VARIABLES)
    virtual String serializeResolvingVariables(const HashMap<AtomicString, String>& variables) const
    {
        return buildCssText(m_leftSide->serializeResolvingVariables(variables), m_rightSide->serializeResolvingVariables(variables), m_operator);
    }

    virtual bool hasVariableReference() const
    {
        return m_leftSide->hasVariableReference() || m_rightSide->hasVariableReference();
    }
#endif

    virtual bool equals(const CSSCalcExpressionNode& exp) const
    {
        if (type() != exp.type())
            return false;

        const CSSCalcBinaryOperation& other = static_cast<const CSSCalcBinaryOperation&>(exp);
        return compareCSSValuePtr(m_leftSide, other.m_leftSide)
            && compareCSSValuePtr(m_rightSide, other.m_rightSide)
            && m_operator == other.m_operator;
    }

    virtual Type type() const { return CssCalcBinaryOperation; }

private:
    CSSCalcBinaryOperation(PassRefPtr<CSSCalcExpressionNode> leftSide, PassRefPtr<CSSCalcExpressionNode> rightSide, CalcOperator op, CalculationCategory category)
        : CSSCalcExpressionNode(category, leftSide->isInteger() && rightSide->isInteger())
        , m_leftSide(leftSide)
        , m_rightSide(rightSide)
        , m_operator(op)
    {
    }
    
    double evaluate(double leftValue, double rightValue) const
    {
        switch (m_operator) {
        case CalcAdd:
            return leftValue + rightValue;
        case CalcSubtract:
            return leftValue - rightValue;
        case CalcMultiply:
            return leftValue * rightValue;
        case CalcDivide:
            if (rightValue)
                return leftValue / rightValue;
            return std::numeric_limits<double>::quiet_NaN();
        }
        return 0;
    }
    
    const RefPtr<CSSCalcExpressionNode> m_leftSide;
    const RefPtr<CSSCalcExpressionNode> m_rightSide;
    const CalcOperator m_operator;
};

static ParseState checkDepthAndIndex(int* depth, unsigned index, CSSParserValueList* tokens)
{
    (*depth)++;
    if (*depth > maxExpressionDepth)
        return TooDeep;
    if (index >= tokens->size())
        return NoMoreTokens;
    return OK;
}

class CSSCalcExpressionNodeParser {
public:
    PassRefPtr<CSSCalcExpressionNode> parseCalc(CSSParserValueList* tokens)
    {
        unsigned index = 0;
        Value result;
        bool ok = parseValueExpression(tokens, 0, &index, &result);
        ASSERT_WITH_SECURITY_IMPLICATION(index <= tokens->size());
        if (!ok || index != tokens->size())
            return 0;
        return result.value;
    }

private:
    struct Value {
        RefPtr<CSSCalcExpressionNode> value;
    };

    char operatorValue(CSSParserValueList* tokens, unsigned index)
    {
        if (index >= tokens->size())
            return 0;
        CSSParserValue* value = tokens->valueAt(index);
        if (value->unit != CSSParserValue::Operator)
            return 0;

        return value->iValue;
    }

    bool parseValue(CSSParserValueList* tokens, unsigned* index, Value* result)
    {
        CSSParserValue* parserValue = tokens->valueAt(*index);
        if (parserValue->unit == CSSParserValue::Operator || parserValue->unit == CSSParserValue::Function)
            return false;

        RefPtr<CSSValue> value = parserValue->createCSSValue();
        if (!value || !value->isPrimitiveValue())
            return false;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value.get());
        result->value = CSSCalcPrimitiveValue::create(primitiveValue, parserValue->isInt);

        ++*index;
        return true;
    }

    bool parseValueTerm(CSSParserValueList* tokens, int depth, unsigned* index, Value* result)
    {
        if (checkDepthAndIndex(&depth, *index, tokens) != OK)
            return false;    
        
        if (operatorValue(tokens, *index) == '(') {
            unsigned currentIndex = *index + 1;
            if (!parseValueExpression(tokens, depth, &currentIndex, result))
                return false;

            if (operatorValue(tokens, currentIndex) != ')')
                return false;
            *index = currentIndex + 1;
            return true;
        }

        return parseValue(tokens, index, result);
    }

    bool parseValueMultiplicativeExpression(CSSParserValueList* tokens, int depth, unsigned* index, Value* result)
    {
        if (checkDepthAndIndex(&depth, *index, tokens) != OK)
            return false;    

        if (!parseValueTerm(tokens, depth, index, result))
            return false;

        while (*index < tokens->size() - 1) {
            char operatorCharacter = operatorValue(tokens, *index);
            if (operatorCharacter != CalcMultiply && operatorCharacter != CalcDivide)
                break;
            ++*index;

            Value rhs;
            if (!parseValueTerm(tokens, depth, index, &rhs))
                return false;

            result->value = CSSCalcBinaryOperation::create(result->value, rhs.value, static_cast<CalcOperator>(operatorCharacter));
            if (!result->value)
                return false;
        }

        ASSERT_WITH_SECURITY_IMPLICATION(*index <= tokens->size());
        return true;
    }

    bool parseAdditiveValueExpression(CSSParserValueList* tokens, int depth, unsigned* index, Value* result)
    {
        if (checkDepthAndIndex(&depth, *index, tokens) != OK)
            return false;    

        if (!parseValueMultiplicativeExpression(tokens, depth, index, result))
            return false;

        while (*index < tokens->size() - 1) {
            char operatorCharacter = operatorValue(tokens, *index);
            if (operatorCharacter != CalcAdd && operatorCharacter != CalcSubtract)
                break;
            ++*index;

            Value rhs;
            if (!parseValueMultiplicativeExpression(tokens, depth, index, &rhs))
                return false;

            result->value = CSSCalcBinaryOperation::create(result->value, rhs.value, static_cast<CalcOperator>(operatorCharacter));
            if (!result->value)
                return false;
        }

        ASSERT_WITH_SECURITY_IMPLICATION(*index <= tokens->size());
        return true;
    }

    bool parseValueExpression(CSSParserValueList* tokens, int depth, unsigned* index, Value* result)
    {
        return parseAdditiveValueExpression(tokens, depth, index, result);
    }
};

PassRefPtr<CSSCalcValue> CSSCalcValue::create(CSSParserString name, CSSParserValueList* parserValueList, CalculationPermittedValueRange range)
{    
    CSSCalcExpressionNodeParser parser;    
    RefPtr<CSSCalcExpressionNode> expression;
    
    if (equalIgnoringCase(name, "calc(") || equalIgnoringCase(name, "-webkit-calc("))
        expression = parser.parseCalc(parserValueList);    
    // FIXME calc (http://webkit.org/b/16662) Add parsing for min and max here

    return expression ? adoptRef(new CSSCalcValue(expression, range)) : 0;
}

} // namespace WebCore
