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
	builtInFunctions.c -- Code for various intrinsic user callable
		functions in the Macross assembler.

	Chip Morningstar -- Lucasfilm Ltd.

	11-January-1985
*/

#include "macrossTypes.h"
#include "macrossGlobals.h"
#include "buildStuff.h"
#include "errorStuff.h"
#include "expressionSemantics.h"
#include "garbage.h"
#include "lexer.h"
#include "lookups.h"
#include "operandStuff.h"
#include "semanticMisc.h"
#include "statementSemantics.h"

#include <string.h>


/* 
   Helper routines to return values into the Macross expression evaluation
   environment
 */

valueType *
makeBooleanValue(int test)
{

	return(newValue(ABSOLUTE_VALUE, test!=0, EXPRESSION_OPND));
}

  valueType *
makeFailureValue(void)
{

	return(newValue(FAIL, 0, EXPRESSION_OPND));
}

  valueType *
makeIntegerValue(int integer)
{

	return(newValue(ABSOLUTE_VALUE, integer, EXPRESSION_OPND));
}

  valueType *
makeOperandValue(operandType *operand)
{

	return(newValue(OPERAND_VALUE, operand, EXPRESSION_OPND));
}

  valueType *
makeStringValue(stringType *string)
{

	return(newValue(STRING_VALUE, string, STRING_OPND));
}

  valueType *
makeUndefinedValue(void)
{

	return(newValue(UNDEFINED_VALUE, 0, EXPRESSION_OPND));
}


/*
   The built-in-functions themselves: pointers to these are loaded into the
   values of the predefined Macross symbols denoting them at assembly
   initialization time from the built-in-function table.  Each symbol of the
   form "xxx" is assigned a pointer to "xxxBIF" (BIF == Built-In-Function).
 */

/* Return internal address mode of an operand */
  valueType *
addressModeBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*evaluatedParameter;


	if (parameterList != NULL) {
		evaluatedParameter = evaluateOperand(parameterList);
		return(makeIntegerValue(evaluatedParameter->addressMode));
	} else {
		return(makeIntegerValue(-1));
	}
}

/* Call a macro where the macro name is obtained dynamically from a string */
  valueType *
applyBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType		*stringValue;
	stringType		*macroToLookup;
	macroTableEntryType	*macroToCall;

	if (parameterList == NULL) {
		error(NO_ARGUMENTS_TO_BIF_ERROR, "apply");
		return(makeFailureValue());
	}
	stringValue = evaluateOperand(parameterList);
	if (stringValue->kindOfValue != STRING_VALUE) {
		error(BIF_ARGUMENT_IS_NOT_A_STRING_ERROR, "apply");
		return(makeFailureValue());
	}
	macroToLookup = (stringType *)stringValue->value;
	if ((macroToCall = lookupMacroName(macroToLookup,
			hashString(macroToLookup))) == 0) {
		error(APPLY_ARGUMENT_IS_NOT_MACRO_ERROR, macroToLookup);
		return(makeFailureValue());
	}
	assembleMacro(macroToCall, parameterList->nextOperand);
	return(makeBooleanValue(TRUE));
}

/* return the length of an array */
  valueType *
arrayLengthBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*testObjectValue;

	if (parameterList == NULL)
		return(makeIntegerValue(0));
	testObjectValue = evaluateOperand(parameterList);
	if (testObjectValue->kindOfValue == STRING_VALUE) {
		return(makeIntegerValue(strlen(testObjectValue->value)));
	} else if (testObjectValue->kindOfValue == ARRAY_VALUE) {
		return(makeIntegerValue(((arrayType *)(testObjectValue->
			value))->arraySize));
	} else {
		return(makeIntegerValue(0));
	}
}

/* The two ATASCII related BIF's refer to this table -- This really only
   makes sense for the 6502 version, but is harmless in other versions. */
static char	 atasciiTable[] = { /* 0xFFs will become 0x00s on output */
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
		0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
		0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
		0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0xFF,
};

/* Convert a string to ATASCII */
  valueType *
atasciiBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	int		 i;
	valueType	*stringValue;
	stringType	*string;
	stringType	*newString;


	if (parameterList == NULL)
		return(makeStringValue(""));
	stringValue = evaluateOperand(parameterList);
	if (stringValue->kindOfValue != STRING_VALUE) {
		error(BIF_ARGUMENT_IS_NOT_A_STRING_ERROR, "atascii");
		return(makeFailureValue());
	}
	string = (stringType *)stringValue->value;
	newString = (stringType *)malloc(strlen(string)+1);
	for (i=0; string[i]!='\0'; i++)
		newString[i] = atasciiTable[string[i]];
	newString[i] = '\0';
	return(makeStringValue(newString));
}

/* Convert a string to ATASCII while setting high-order color bits */
  valueType *
atasciiColorBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	int		 i;
	valueType	*stringValue;
	stringType	*string;
	valueType	*colorValue;
	int		 color;
	stringType	*newString;
	byte		 testChar;


	if (parameterList == NULL)
		return(makeStringValue(""));
	stringValue = evaluateOperand(parameterList);
	parameterList = parameterList->nextOperand;
	if (parameterList == NULL) {
		color = 0;
	} else {
		colorValue = evaluateOperand(parameterList);
		if (colorValue->kindOfValue != ABSOLUTE_VALUE) {
			error(BAD_COLOR_ARGUMENT_TO_ATASCII_COLOR_ERROR);
			return(makeFailureValue());
		}
		color = colorValue->value;
	}
	color = (color & 0x03) << 6;
	if (stringValue->kindOfValue != STRING_VALUE) {
		error(BIF_ARGUMENT_IS_NOT_A_STRING_ERROR, "atasciiColor");
		return(makeFailureValue());
	}
	string = (stringType *)stringValue->value;
	newString = (stringType *)malloc(strlen(string)+1);
	for (i=0; string[i]!='\0'; i++) {
		testChar = atasciiTable[string[i]];
		if (testChar == 0xFF)
			testChar = 0;
		testChar = (testChar & 0x3F) | color;
		if (testChar == 0)
			testChar = 0xFF;
		newString[i] = testChar;
	}
	newString[i] = '\0';
	return(makeStringValue(newString));
}

/* Turn debug mode off and on */
  valueType *
debugModeOffBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	debug = FALSE;
	return(makeBooleanValue(FALSE));
}

  valueType *
debugModeOnBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	debug = TRUE;
	return(makeBooleanValue(TRUE));
}

/* Turn display of code emission off and on */
  valueType *
emitModeOffBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	emitPrint = FALSE;
	return(makeBooleanValue(FALSE));
}

  valueType *
emitModeOnBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	emitPrint = TRUE;
	return(makeBooleanValue(TRUE));
}

/* Check if an operand is absolute (as opposed to relocatable) */
  valueType *
isAbsoluteValueBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*evaluatedParameter;


	if (parameterList != NULL) {
		evaluatedParameter = evaluateOperand(parameterList);
		return(makeBooleanValue(evaluatedParameter->kindOfValue ==
			ABSOLUTE_VALUE));
	} else {
		return(makeBooleanValue(FALSE));
	}
}

/* Check if operand is code block */
  valueType *
isBlockBIF(operandListType *parameterList, fixupKindType kindOfFixup) /* questionable */
                 	               
               		             
{
	valueType	*evaluatedParameter;


	if (parameterList != NULL) {
		evaluatedParameter = evaluateOperand(parameterList);
		return(makeBooleanValue(evaluatedParameter->kindOfValue ==
			BLOCK_VALUE));
	} else {
		return(makeBooleanValue(FALSE));
	}
}

/* Check if operand is a BIF */
  valueType *
isBuiltInFunctionBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*evaluatedParameter;


	if (parameterList != NULL) {
		evaluatedParameter = evaluateOperand(parameterList);
		return(makeBooleanValue(evaluatedParameter->kindOfValue ==
			BUILT_IN_FUNCTION_VALUE));
	} else {
		return(makeBooleanValue(FALSE));
	}
}

/* Check if operand is a condition code */
  valueType *
isConditionCodeBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*evaluatedParameter;


	if (parameterList != NULL) {
		evaluatedParameter = evaluateOperand(parameterList);
		return(makeBooleanValue(evaluatedParameter->kindOfValue ==
			CONDITION_VALUE));
	} else {
		return(makeBooleanValue(FALSE));
	}
}

/* Check if a symbol is defined */
  valueType *
isDefinedBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	expressionType		*expression;
	symbolInContextType	*context;

	if (parameterList != NULL) {
		if (parameterList->kindOfOperand == EXPRESSION_OPND &&
				(expression = parameterList->theOperand.
				expressionUnion) != NULL && expression->
				kindOfTerm == IDENTIFIER_EXPR &&
				effectiveSymbol(expression->expressionTerm.
				identifierUnion, &context) != NULL &&
				context != NULL && (context->value->
				kindOfValue == UNDEFINED_VALUE || context->
				value->kindOfValue == FAIL)) {
			if ((context->usage == UNKNOWN_SYMBOL || context->
					usage == NESTED_UNKNOWN_SYMBOL) &&
					context->referenceCount == 0)
				context->usage = DEAD_SYMBOL;
			return(makeBooleanValue(FALSE));
		} else {
			return(makeBooleanValue(TRUE));
		}
	} else {
		return(makeBooleanValue(FALSE));
	}
}

/* Check if a symbol is externally visible */
  valueType *
isExternalBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	expressionType		*expression;
	symbolInContextType	*context;

	if (parameterList != NULL && parameterList->kindOfOperand ==
			EXPRESSION_OPND) {
		expression = parameterList->theOperand.expressionUnion;
		return(makeBooleanValue(expression->kindOfTerm ==
			IDENTIFIER_EXPR && (context = getWorkingContext(
			expression->expressionTerm.identifierUnion))!=NULL &&
			(context->attributes & GLOBAL_ATT)!=0));
	} else {
		return(makeBooleanValue(FALSE));
	}
}

/* Check if an operand is a struct field */
  valueType *
isFieldBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*evaluatedParameter;


	if (parameterList != NULL) {
		evaluatedParameter = evaluateOperand(parameterList);
		return(makeBooleanValue(evaluatedParameter->kindOfValue ==
			FIELD_VALUE));
	} else {
		return(makeBooleanValue(FALSE));
	}
}

/* Check if an operand is a user-defined function */
  valueType *
isFunctionBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*evaluatedParameter;


	if (parameterList != NULL) {
		evaluatedParameter = evaluateOperand(parameterList);
		return(makeBooleanValue(evaluatedParameter->kindOfValue ==
			FUNCTION_VALUE));
	} else {
		return(makeBooleanValue(FALSE));
	}
}

/* Check if an operand value is relocatable */
  valueType *
isRelocatableValueBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*evaluatedParameter;


	if (parameterList != NULL) {
		evaluatedParameter = evaluateOperand(parameterList);
		return(makeBooleanValue(evaluatedParameter->kindOfValue ==
			RELOCATABLE_VALUE || evaluatedParameter->
			kindOfValue == DATA_VALUE));
	} else {
		return(makeBooleanValue(FALSE));
	}
}

/* Check if an operand is a string */
  valueType *
isStringBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*evaluatedParameter;


	if (parameterList != NULL) {
		evaluatedParameter = evaluateOperand(parameterList);
		return(makeBooleanValue(evaluatedParameter->kindOfValue ==
			STRING_VALUE));
	} else {
		return(makeBooleanValue(FALSE));
	}
}

/* Check if an operand is a struct */
  valueType *
isStructBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*evaluatedParameter;


	if (parameterList != NULL) {
            evaluatedParameter = evaluateOperand(parameterList);
		return(makeBooleanValue(evaluatedParameter->kindOfValue ==
			STRUCT_VALUE));
	} else {
		return(makeBooleanValue(FALSE));
	}
}

/* Check if an operand is a symbol */
  valueType *
isSymbolBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	expressionType		*expression;

	if (parameterList != NULL && parameterList->kindOfOperand ==
			EXPRESSION_OPND) {
		expression = parameterList->theOperand.expressionUnion;
		return(makeBooleanValue(expression->kindOfTerm ==
				IDENTIFIER_EXPR));
	} else {
		return(makeBooleanValue(FALSE));
	}
}

/* Turn listing off and on */
  valueType *
listingOffBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	if (listingOn) {
		if (!listingControlCounter)
			saveListingOff();
		listingControlCounter++;
	}
	return(makeBooleanValue(FALSE));
}

  valueType *
listingOnBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	if (listingOn) {
		if (--listingControlCounter < 0) {
			listingControlCounter = 0;
			return(makeBooleanValue(TRUE));
		} else if (listingControlCounter) {
			return(makeBooleanValue(FALSE));
		} else {
			saveListingOn();
			return(makeBooleanValue(TRUE));
		}
	} else {
		return(makeBooleanValue(FALSE));
	}
}

/* Generate an array on the fly */
  valueType *
makeArrayBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*lengthValue;
	int		 length;
	valueType	*result;
	valueType      **arrayContents;
	int		 i;

	if (parameterList == NULL) {
		error(NO_ARGUMENTS_TO_BIF_ERROR, "makeArray");
		return(makeFailureValue());
	}
	lengthValue = evaluateOperand(parameterList);
	if (lengthValue->kindOfValue != ABSOLUTE_VALUE) {
		error(BIF_NTH_ARGUMENT_IS_NOT_ABSOLUTE_VALUE_ERROR,
			"makeArray", 1);
		return(makeFailureValue());
	} else if (lengthValue->value < 0) {
		error(NEGATIVE_ARRAY_SIZE_TO_MAKEARRAY_ERROR);
		return(makeFailureValue());
	}
	length = lengthValue->value;
	result = newValue(ARRAY_VALUE, allocArray(length, &arrayContents),
		EXPRESSION_OPND);
	parameterList = parameterList->nextOperand;
	for (i=0; i<length; ++i) {
		if (parameterList != NULL) {
			arrayContents[i] = evaluateOperand(parameterList);
			parameterList = parameterList->nextOperand;
		} else {
			arrayContents[i] = NULL;
		}
	}
	if (parameterList != NULL) {
		error(TOO_MANY_INITIALIZATION_ARGS_TO_MAKEARRAY_ERROR);
		return(makeFailureValue());
	}
	return(result);
}

/* Return the Nth character of a string (as an integer) */
  valueType *
nthCharBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*positionValue;
	int		 position;
	valueType	*stringValue;
	stringType	*string;


	if (parameterList == NULL)
		return(makeFailureValue());
	stringValue = evaluateOperand(parameterList);
	if (stringValue->kindOfValue != STRING_VALUE) {
		error(BIF_ARGUMENT_IS_NOT_A_STRING_ERROR, "nthChar");
		return(makeFailureValue());
	}
	parameterList = parameterList->nextOperand;
	if (parameterList == NULL) {
		position = 0;
	} else {
		positionValue = evaluateOperand(parameterList);
		if (positionValue->kindOfValue != ABSOLUTE_VALUE) {
			error(BAD_POSITION_ARGUMENT_TO_NTH_CHAR_ERROR);
			return(makeFailureValue());
		}
		position = positionValue->value;
	}

	string = (stringType *)stringValue->value;
	if (position >= strlen(string)) {
		error(BAD_POSITION_ARGUMENT_TO_NTH_CHAR_ERROR);
		return(makeFailureValue());
	}
	return(makeIntegerValue(string[position]));
}

/* Pass stuff through to stdio's 'printf' function */
  valueType *
printfBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	stringType	*formatString;
	valueType	*stringValue;
	int		 argumentCount;
	int		 argument[20];
	valueType	*result;


	result = makeFailureValue();
	if (parameterList != NULL) {
		stringValue = evaluateOperand(parameterList);
		if (stringValue->kindOfValue != STRING_VALUE) {
			error(PRINTF_FORMAT_IS_NOT_A_STRING_ERROR);
			return(result);
		}
		formatString = (stringType *)stringValue->value;
		parameterList = parameterList->nextOperand;
		argumentCount = 0;
		while (parameterList != NULL && argumentCount < 20) {
			argument[argumentCount++] = evaluateOperand(
                            parameterList)->value;
			parameterList = parameterList->nextOperand;
		}
		/* cretinous hack */
		printf(formatString, argument[0], argument[1], argument[2],
			argument[3], argument[4], argument[5], argument[6],
			argument[7], argument[8], argument[9], argument[10],
			argument[11], argument[12], argument[13],
			argument[14], argument[15], argument[16],
			argument[17], argument[18], argument[19]);
	}
	return(result);
}

/* Concatenate two strings */
  valueType *
strcatBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*stringValue;
	stringType	*string1;
	stringType	*string2;
	stringType	*newString;

	if (parameterList == NULL)
		return(makeStringValue(""));
	stringValue = evaluateOperand(parameterList);
	if (stringValue->kindOfValue != STRING_VALUE) {
		error(BIF_NTH_ARGUMENT_IS_NOT_A_STRING_ERROR, "strcat", 1);
		return(makeFailureValue());
	}
	string1 = (stringType *)stringValue->value;
	parameterList = parameterList->nextOperand;
	if (parameterList == NULL)
		return(makeStringValue(string1));
	stringValue = evaluateOperand(parameterList);
	if (stringValue->kindOfValue != STRING_VALUE) {
		error(BIF_NTH_ARGUMENT_IS_NOT_A_STRING_ERROR, "strcat", 2);
		return(makeFailureValue());
	}
	string2 = (stringType *)stringValue->value;
	newString = (stringType *)malloc(strlen(string1)+strlen(string2)+1);
	strcpy(newString, string1);
	strcat(newString, string2);
	return(makeStringValue(newString));
}

/* Compare two strings */
  valueType *
strcmpBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*stringValue;
	stringType	*string1;
	stringType	*string2;
	stringType	*newString;

	if (parameterList == NULL) {
		error(NO_ARGUMENTS_TO_BIF_ERROR, "strcmp");
		return(makeFailureValue());
	}
	stringValue = evaluateOperand(parameterList);
	if (stringValue->kindOfValue != STRING_VALUE) {
		error(BIF_NTH_ARGUMENT_IS_NOT_A_STRING_ERROR, "strcmp", 1);
		return(makeFailureValue());
	}
	string1 = (stringType *)stringValue->value;
	parameterList = parameterList->nextOperand;
	if (parameterList == NULL)
		return(makeStringValue(string1));
	stringValue = evaluateOperand(parameterList);
	if (stringValue->kindOfValue != STRING_VALUE) {
		error(BIF_NTH_ARGUMENT_IS_NOT_A_STRING_ERROR, "strcmp", 2);
		return(makeFailureValue());
	}
	string2 = (stringType *)stringValue->value;
	return(makeIntegerValue(strcmp(string1, string2)));
}

/* Compare two strings in a case-independent fashion */
  valueType *
strcmplcBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*stringValue;
	stringType	*string1;
	stringType	*string2;
	stringType	*newString;

	if (parameterList == NULL) {
		error(NO_ARGUMENTS_TO_BIF_ERROR, "strcmplc");
		return(makeFailureValue());
	}
	stringValue = evaluateOperand(parameterList);
	if (stringValue->kindOfValue != STRING_VALUE) {
		error(BIF_NTH_ARGUMENT_IS_NOT_A_STRING_ERROR, "strcmplc", 1);
		return(makeFailureValue());
	}
	string1 = (stringType *)stringValue->value;
	parameterList = parameterList->nextOperand;
	if (parameterList == NULL)
		return(makeStringValue(string1));
	stringValue = evaluateOperand(parameterList);
	if (stringValue->kindOfValue != STRING_VALUE) {
		error(BIF_NTH_ARGUMENT_IS_NOT_A_STRING_ERROR, "strcmplc", 2);
		return(makeFailureValue());
	}
	string2 = (stringType *)stringValue->value;
	return(makeIntegerValue(strcmplc(string1, string2)));
}

/* Return the length of a string */
  valueType *
strlenBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*stringValue;

	if (parameterList == NULL)
		return(makeIntegerValue(-1));
	stringValue = evaluateOperand(parameterList);
	if (stringValue->kindOfValue != STRING_VALUE) {
		error(BIF_ARGUMENT_IS_NOT_A_STRING_ERROR, "strlen");
		return(makeFailureValue());
	} else
		return(makeIntegerValue(strlen(stringValue->value)));
}

/* Return a substring of a string */
  valueType *
substrBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*stringValue;
	stringType	*string;
	valueType	*startValue;
	int		 start;
	valueType	*lengthValue;
	int		 length;
	stringType	*newString;
	int		 originalLength;
	bool		 backwards;

	if (parameterList == NULL) {
		error(NO_ARGUMENTS_TO_BIF_ERROR, "substr");
		return(makeFailureValue());
	}
	stringValue = evaluateOperand(parameterList);
	if (stringValue->kindOfValue != STRING_VALUE) {
		error(BIF_ARGUMENT_IS_NOT_A_STRING_ERROR, "substr");
		return(makeFailureValue());
	}
	string = (stringType *)stringValue->value;
	originalLength = strlen(string);

	parameterList = parameterList->nextOperand;
	if (parameterList == NULL) {
		error(TOO_FEW_ARGUMENTS_TO_BIF_ERROR, "substr");
		return(makeFailureValue());
	}
	startValue = evaluateOperand(parameterList);
	if (startValue->kindOfValue != ABSOLUTE_VALUE) {
		error(BIF_NTH_ARGUMENT_IS_NOT_ABSOLUTE_VALUE_ERROR, "substr",
			2);
		return(makeFailureValue());
	}
	start = startValue->value;
	if (start < 0) {
		start = -start - 1;
		backwards = TRUE;
	} else
		backwards = FALSE;

	parameterList = parameterList->nextOperand;
	if (parameterList == NULL) {
		length = originalLength - start;
		if (backwards)
			length = -length;
	} else {
		lengthValue = evaluateOperand(parameterList);
		if (lengthValue->kindOfValue != ABSOLUTE_VALUE) {
			error(BIF_NTH_ARGUMENT_IS_NOT_ABSOLUTE_VALUE_ERROR,
				"substr", 3);
			return(makeFailureValue());
		}
		length = lengthValue->value;
	}
	if (length < 0) {
		length = -length;
		if (backwards)
			start = start + length - 1;
		else
			start = start - length + 1;
	}
	if (backwards)
		start = originalLength - start - 1;
	if (originalLength <= start || originalLength < length + start ||
			start < 0 ){
		error(BAD_SUBSTRING_INDICES_ERROR);
		return(makeFailureValue());
	}
	newString = (stringType *)malloc(length+1);
	strncpy(newString, string+start, length);
	newString[length] = '\0';
	return(makeStringValue(newString));
}

/* Turn a string into a symbol */
  valueType *
symbolLookupBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType		*stringValue;
	stringType		*identifierToLookup;

	if (parameterList == NULL) {
		error(NO_ARGUMENTS_TO_BIF_ERROR, "symbolLookup");
		return(makeFailureValue());
	}
	stringValue = evaluateOperand(parameterList);
	if (stringValue->kindOfValue != STRING_VALUE) {
		error(BIF_ARGUMENT_IS_NOT_A_STRING_ERROR, "symbolLookup");
		return(makeFailureValue());
	}
	identifierToLookup = (stringType *)stringValue->value;
	return(evaluateIdentifier(lookupOrEnterSymbol(identifierToLookup,
		UNKNOWN_SYMBOL), FALSE, kindOfFixup));
}

/* Define a string as a symbol */
  valueType *
symbolDefineBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType		*stringValue;
	statementType		*syntheticDefineStatement;

	if (parameterList == NULL) {
		error(NO_ARGUMENTS_TO_BIF_ERROR, "symbolDefine");
		return(makeFailureValue());
	}
	stringValue = evaluateOperand(parameterList);
	if (stringValue->kindOfValue != STRING_VALUE) {
		error(BIF_ARGUMENT_IS_NOT_A_STRING_ERROR, "symbolDefine");
		return(makeFailureValue());
	}
	parameterList = parameterList->nextOperand;
	if (parameterList == NULL) {
		syntheticDefineStatement = buildDefineStatement(stringValue->
			value, NULL);
	} else if (parameterList->kindOfOperand != EXPRESSION_OPND) {
		error(SYMBOL_DEFINE_VALUE_NOT_EXPRESSION_OPND_ERROR);
		return(makeFailureValue());
	} else {
		syntheticDefineStatement = buildDefineStatement(stringValue->
			value, parameterList->theOperand.expressionUnion);
	}
	assembleDefineStatement(syntheticDefineStatement->statementBody.defineUnion);
	freeStatement(syntheticDefineStatement);
	return(makeBooleanValue(TRUE));
}

/* Turn a symbol into a string */
  valueType *
symbolNameBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	expressionType		*expression;
	symbolInContextType	*context;
	environmentType		*saveEnvironment;

	saveEnvironment = currentEnvironment;
	while (parameterList != NULL && parameterList->kindOfOperand ==
			EXPRESSION_OPND) {
		expression = parameterList->theOperand.expressionUnion;
		if (expression->kindOfTerm == IDENTIFIER_EXPR && (context =
				getWorkingContext(expression->expressionTerm.
				identifierUnion)) != NULL) {
			if (context->value->kindOfValue != OPERAND_VALUE ||
					context->usage != ARGUMENT_SYMBOL) {
				currentEnvironment = saveEnvironment;
				return(makeStringValue(symbName(expression->
					expressionTerm.identifierUnion)));
			} else {
				currentEnvironment = currentEnvironment->
					previousEnvironment;
				parameterList = (operandListType *)context->
					value->value;
			}
		} else {
			break;
		}
	}
	currentEnvironment = saveEnvironment;
	error(CANT_FIND_SYMBOL_ERROR);
	return(makeStringValue(""));
}

/* Return internal form of what sort of symbol a symbol is */
  valueType *
symbolUsageBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	expressionType		*expression;
	symbolInContextType	*context;

	if (parameterList != NULL && parameterList->kindOfOperand ==
			EXPRESSION_OPND) {
		expression = parameterList->theOperand.expressionUnion;
		if (expression->kindOfTerm == IDENTIFIER_EXPR && (context =
				getWorkingContext(expression->expressionTerm.
				identifierUnion))!=NULL)
			return(makeIntegerValue(context->usage));
	}
	return(makeIntegerValue(-1));
}

/* Return internal form of what sort of value a value is */
  valueType *
valueTypeBIF(operandListType *parameterList, fixupKindType kindOfFixup)
{
	valueType	*evaluatedParameter;

	if (parameterList != NULL) {
		evaluatedParameter = evaluateOperand(parameterList);
		return(makeIntegerValue(evaluatedParameter->kindOfValue));
	} else {
		return(makeIntegerValue(-1));
	}
}

