/*
 * Copyright (c) 1987 Fujitsu
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/*
	instantiate.c -- Slinky linker routines to walk expression tree and
		replace symbol ordinals with symbol pointers.

	Chip Morningstar -- Lucasfilm Ltd.

	11-November-1985
*/

#include "slinkyTypes.h"
#include "slinkyGlobals.h"
#include "slinkyExpressions.h"
#include "errorStuff.h"
#include "expr.h"
#include "instantiate.h"
#include "../macross/y.tab.h"

#define overFunction() (pc+=sizeof(functionType *))
#define overNumber() (pc+=sizeof(addressType))
#define overByte() pc++
#define nextByte(byt) (byt = *pc++)

  void
putNumber(int number)
{
	int	i;
	for (i=0; i<sizeof(int); ++i) {
		*pc++ = number & 0xFF;
		number >>= 8;
	}
}

  void
instantiateSymbol(void)
{
	int	index;

	index = evaluateNumber();
	pc -= sizeof(symbolType *);
	putNumber(currentSymbolTable[index]);
}

  void
instantiateFunction(void)
{
	int	index;

	index = evaluateNumber();
	pc -= sizeof(functionType *);
	putNumber(&(currentFunctionTable[index]));
}

  void
putSymbolPointersIntoArray(void)
{
	instantiateSymbol();
	putSymbolPointersIntoExpression();
}

  void
putSymbolPointersIntoAssert(void)
{
	putSymbolPointersIntoExpression();
	skipString();
}

  void
putSymbolPointersIntoBinop(void)
{
	overByte();
	putSymbolPointersIntoExpression();
	putSymbolPointersIntoExpression();
}

  void
putSymbolPointersIntoBlock(void)
{
	while (*pc != END_TAG)
		putSymbolPointersIntoExpression();
	overByte();
}

  void
putSymbolPointersIntoBuiltinFunctionCall(void)
{
	int	argCount;

	overNumber();
	nextByte(argCount);
	while (argCount-- > 0)
		putSymbolPointersIntoExpression();
}

  void
putSymbolPointersIntoFunctionCall(void)
{
	int	argCount;

	instantiateFunction();
	nextByte(argCount);
	while (argCount-- > 0)
		putSymbolPointersIntoExpression();
}

  void
putSymbolPointersIntoMdefine(void)
{
	instantiateSymbol();
	putSymbolPointersIntoExpression();
}

  void
putSymbolPointersIntoMdoUntil(void)
{
	putSymbolPointersIntoExpression();
	putSymbolPointersIntoExpression();
}

  void
putSymbolPointersIntoMdoWhile(void)
{
	putSymbolPointersIntoExpression();
	putSymbolPointersIntoExpression();
}

  void
putSymbolPointersIntoMfor(void)
{
	putSymbolPointersIntoExpression();
	putSymbolPointersIntoExpression();
	putSymbolPointersIntoExpression();
	putSymbolPointersIntoExpression();
}

  void
putSymbolPointersIntoMif(void)
{
	putSymbolPointersIntoExpression();
	putSymbolPointersIntoExpression();
	putSymbolPointersIntoExpression();
}

  void
putSymbolPointersIntoClause(void)
{
	while (*pc != BLOCK_TAG)
		putSymbolPointersIntoExpression();
	putSymbolPointersIntoBlock();
}

  void
putSymbolPointersIntoMswitch(void)
{
	putSymbolPointersIntoExpression();
	while (*pc != END_TAG)
		putSymbolPointersIntoClause();
	overByte();
}

  void
putSymbolPointersIntoMvariable(void)
{
	instantiateSymbol();
	putSymbolPointersIntoExpression();
}

  void
putSymbolPointersIntoMwhile(void)
{
	putSymbolPointersIntoExpression();
	putSymbolPointersIntoExpression();
}

  void
putSymbolPointersIntoPostop(void)
{
	overByte();
	putSymbolPointersIntoExpression();
}

  void
putSymbolPointersIntoPreop(void)
{
	overByte();
	putSymbolPointersIntoExpression();
}

  void
putSymbolPointersIntoUnop(void)
{
	overByte();
	putSymbolPointersIntoExpression();
}

  void
putSymbolPointersIntoExpression(void)
{
	if (pc == NULL)
		return;
	switch (*pc++) {
	    case IDENTIFIER_TAG:
		instantiateSymbol();
		break;

	    case FUNCTION_CALL_TAG:
		putSymbolPointersIntoFunctionCall();
		break;

	    case BUILTIN_FUNCTION_CALL_TAG:
		putSymbolPointersIntoBuiltinFunctionCall();
		break;

	    case NUMBER_TAG:
	    case RELOCATABLE_TAG:
		overNumber();
		break;

	    case CONDITION_CODE_TAG:
		overByte();
		break;

	    case SUBEXPRESSION_TAG:
		putSymbolPointersIntoExpression();
		break;

	    case UNOP_TAG:
		putSymbolPointersIntoUnop();
		break;

	    case BINOP_TAG:
		putSymbolPointersIntoBinop();
		break;

	    case PREOP_TAG:
		putSymbolPointersIntoPreop();
		break;

	    case POSTOP_TAG:
		putSymbolPointersIntoPostop();
		break;

	    case HERE_TAG:
		break;

	    case STRING_TAG:
		skipString();
		break;

	    case ARRAY_TAG:
		putSymbolPointersIntoArray();
		break;

	    case VALUE_TAG:
		overNumber();
		break;

	    case NULL_TAG:
		break;

	    case BLOCK_TAG:
		putSymbolPointersIntoBlock();
		break;

	    case MDEFINE_TAG:
		putSymbolPointersIntoMdefine();
		break;

	    case MVARIABLE_TAG:
		putSymbolPointersIntoMvariable();
		break;

	    case MIF_TAG:
		putSymbolPointersIntoMif();
		break;

	    case MFOR_TAG:
		putSymbolPointersIntoMfor();
		break;

	    case MWHILE_TAG:
		putSymbolPointersIntoMwhile();
		break;

	    case MDOWHILE_TAG:
		putSymbolPointersIntoMdoWhile();
		break;

	    case MDOUNTIL_TAG:
		putSymbolPointersIntoMdoUntil();
		break;

	    case PERFORM_TAG:
		putSymbolPointersIntoExpression();
		break;

	    case GROUP_TAG:
		putSymbolPointersIntoBlock();
		break;

	    case ASSERT_TAG:
		putSymbolPointersIntoAssert();
		break;

	    case MSWITCH_TAG:
		putSymbolPointersIntoMswitch();
		break;

	    case CLAUSE_TAG:
		putSymbolPointersIntoClause();
		break;

	    case END_TAG:
		break;
	}
}
