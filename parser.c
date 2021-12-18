/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */
#include <stdio.h>
#include <stdlib.h>

#include "reader.h"
#include "scanner.h"
#include "parser.h"
#include "semantics.h"
#include "error.h"
#include "debug.h"

Token *currentToken;
Token *lookAhead;

extern Type* intType;
extern Type* charType;
extern SymTab* symtab;

void scan(void) {
  Token* tmp = currentToken;
  currentToken = lookAhead;
  lookAhead = getValidToken();
  free(tmp);
}

void eat(TokenType tokenType) { //kiểm tra token hiện tại và lấy token tiếp theo
  if (lookAhead->tokenType == tokenType) {
    scan();
  } else missingToken(tokenType, lookAhead->lineNo, lookAhead->colNo);
}

// KW_PROGRAM TK_INDENT SB_SEMICOLON <Block> SB_PERIOD
void compileProgram(void) { // biên dịch chương trình theo văn phạm viết bằng BNF
  Object* program;

  eat(KW_PROGRAM); // token đầu tiên đọc được phải là PROGRAM
  eat(TK_IDENT); // token tiếp theo đọc được phải là một định danh

  program = createProgramObject(currentToken->string); // currentToken->string khi này là tên chương trình (chuỗi ký tự sau PROGRAM)
  enterBlock(program->progAttrs->scope); // cập nhật current scope

  eat(SB_SEMICOLON); // token tiếp theo đọc được phải là dấu ;

  compileBlock(); // biên dịch block của chương trình chính
  eat(SB_PERIOD); // token cuối cùng phải là dấu .

  exitBlock();
}

// KW_CONST <ConstDecl> <ConstDecls> <Block2>
// <Block2>
void compileBlock(void) {
  Object* constObj;
  ConstantValue* constValue;

  if (lookAhead->tokenType == KW_CONST) { // trong trường hợp token hiện tại là KW_CONST
    eat(KW_CONST);

    do {
      eat(TK_IDENT);
      // Check if a constant identifier is fresh in the block
      checkFreshIdent(currentToken->string); // kiểm tra định danh có trùng tên không trong scope hiện tại
      // Create a constant object
      constObj = createConstantObject(currentToken->string);
      
      eat(SB_EQ);

      // Get the constant value
      constValue = compileConstant(); // lấy giá trị của constant truyền vào
      constObj->constAttrs->value = constValue;

      // Declare the constant object ~ đưa đối tượng vào trong bảng ký hiệu
      declareObject(constObj);
      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock2();
  } 
  else compileBlock2(); // Trong trường hợp token hiện tại không phải là KW_CONST
}

// KW_TYPE <TypeDecl> <TypeDecls> <Block3>
// <Block3>
void compileBlock2(void) {
  Object* typeObj;
  Type* actualType;

  if (lookAhead->tokenType == KW_TYPE) {
    eat(KW_TYPE);

    do {
      eat(TK_IDENT);
      // Check if a type identifier is fresh in the block
      checkFreshIdent(currentToken->string);
      // create a type object
      typeObj = createTypeObject(currentToken->string);
      
      eat(SB_EQ);
      // Get the actual type
      actualType = compileType();
      typeObj->typeAttrs->actualType = actualType;
      // Declare the type object
      declareObject(typeObj);
      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock3();
  } 
  else compileBlock3();
}

// KW_VAR <Vardecl> <VarDecls> <Block4>
// <Block4>
void compileBlock3(void) {
  Object* varObj;
  Type* varType;

  if (lookAhead->tokenType == KW_VAR) {
    eat(KW_VAR);

    do {
      eat(TK_IDENT);
      // Check if a variable identifier is fresh in the block
      checkFreshIdent(currentToken->string);
      // Create a variable object      
      varObj = createVariableObject(currentToken->string);

      eat(SB_COLON);
      // Get the variable type
      varType = compileType();
      varObj->varAttrs->type = varType;
      // Declare the variable object
      declareObject(varObj);
      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock4();
  } 
  else compileBlock4();
}

// <SubDecls><Block5> | <Block5>
void compileBlock4(void) {
  compileSubDecls();
  compileBlock5();
}

// KW_BEGIN <Statements> KW_END
void compileBlock5(void) {
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

// <FuncDecl> <SubDecls>
// <ProcDecl> <SubDecls>
void compileSubDecls(void) { // Biên dịch các hàm hoặc thủ tục
  while ((lookAhead->tokenType == KW_FUNCTION) || (lookAhead->tokenType == KW_PROCEDURE)) {
    if (lookAhead->tokenType == KW_FUNCTION)
      compileFuncDecl();
    else compileProcDecl();
  }
}

// KW_FUNCTION TK_IDENT <Params> SB_COLON <BasicType> SB_SEMICOLON <Block> SB_SEMICOLON
void compileFuncDecl(void) {
  Object* funcObj;
  Type* returnType;

  eat(KW_FUNCTION);
  eat(TK_IDENT);
  // Check if a function identifier is fresh in the block
  checkFreshIdent(currentToken->string);
  // create the function object
  funcObj = createFunctionObject(currentToken->string);
  // declare the function object
  declareObject(funcObj);
  // enter the function's block
  enterBlock(funcObj->funcAttrs->scope);
  // parse the function's parameters
  compileParams();
  eat(SB_COLON);
  // get the funtion's return type
  returnType = compileBasicType();
  funcObj->funcAttrs->returnType = returnType;

  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_SEMICOLON);
  // exit the function block
  exitBlock();
}

// KW_PROCEDURE TK_IDENT <Params> SB_SEMICOLON <Block> SB_SEMICOLON
void compileProcDecl(void) {
  // TODO: check if the procedure identifier is fresh in the block
  Object* procObj;

  eat(KW_PROCEDURE);
  eat(TK_IDENT);
  // Check if a procedure identifier is fresh in the block
  checkFreshIdent(currentToken->string);
  // create a procedure object
  procObj = createProcedureObject(currentToken->string);
  // declare the procedure object
  declareObject(procObj);
  // enter the procedure's block
  enterBlock(procObj->procAttrs->scope);
  // parse the procedure's parameters
  compileParams();

  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_SEMICOLON);
  // exit the block
  exitBlock();
}

ConstantValue* compileUnsignedConstant(void) {
  ConstantValue* constValue;
  Object* obj;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    // check if the constant identifier is declared
    obj = checkDeclaredConstant(currentToken->string);
    constValue = duplicateConstantValue(obj->constAttrs->value);

    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

// SB_PLUS <Constant2>
// SB_MINUS <Constant2>
// <Constant2>
// TK_CHAR
ConstantValue* compileConstant(void) {
  ConstantValue* constValue;

  switch (lookAhead->tokenType) {
    case SB_PLUS:
      eat(SB_PLUS);
      constValue = compileConstant2();
      break;
    case SB_MINUS:
      eat(SB_MINUS);
      constValue = compileConstant2();
      constValue->intValue = - constValue->intValue;
      break;
    case TK_CHAR:
      eat(TK_CHAR);
      constValue = makeCharConstant(currentToken->string[0]); // chỉ lấy ký tự, không lấy '\0'
      break;
    default:
      constValue = compileConstant2();
      break;
  }
  return constValue;
}

// TK_IDENT
// TK_NUMBER
ConstantValue* compileConstant2(void) {
  ConstantValue* constValue;
  Object* obj;

  switch (lookAhead->tokenType) {
    case TK_NUMBER:
      eat(TK_NUMBER);
      constValue = makeIntConstant(currentToken->value);
      break;
    case TK_IDENT:
      eat(TK_IDENT);
      // check if the integer constant identifier is declared
      obj = checkDeclaredConstant(currentToken->string);
      if (obj->constAttrs->value->type == TP_INT)
        constValue = duplicateConstantValue(obj->constAttrs->value);
      else
        error(ERR_UNDECLARED_INT_CONSTANT,currentToken->lineNo, currentToken->colNo);
      break;
    default:
      error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
      break;
  }
  return constValue;
}

// KW_INTEGER
// KW_CHAR
// TK_IDENT
// KW_ARRAY SB_LSEL TK_NUMBER SB_RSEL KW_OF <Type>
Type* compileType(void) { //trả về kiểu tương ứng hoặc nếu là định danh thì tìm kiếm xem định danh đó đã được định nghĩa chưa
  Type* type;
  Type* elementType;
  int arraySize;
  Object* obj;

  switch (lookAhead->tokenType) {
    case KW_INTEGER: 
      eat(KW_INTEGER);
      type =  makeIntType();
      break;
    case KW_CHAR: 
      eat(KW_CHAR); 
      type = makeCharType();
      break;
    case KW_ARRAY:
      eat(KW_ARRAY);
      eat(SB_LSEL);
      eat(TK_NUMBER);

      arraySize = currentToken->value;

      eat(SB_RSEL);
      eat(KW_OF);
      elementType = compileType();
      type = makeArrayType(arraySize, elementType);
      break;
    case TK_IDENT:
      eat(TK_IDENT);
      // check if the type idntifier is declared
      obj = checkDeclaredType(currentToken->string);
      type = duplicateType(obj->typeAttrs->actualType);
      break;
    default:
      error(ERR_INVALID_TYPE, lookAhead->lineNo, lookAhead->colNo);
      break;
  }
  return type;
}

// KW_INTEGER
// KW_CHAR
Type* compileBasicType(void) {
  Type* type;

  switch (lookAhead->tokenType) {
  case KW_INTEGER: 
    eat(KW_INTEGER); 
    type = makeIntType();
    break;
  case KW_CHAR: 
    eat(KW_CHAR); 
    type = makeCharType();
    break;
  default:
    error(ERR_INVALID_BASICTYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

// SB_LPAR <Param> <Params2> SB_RPAR
void compileParams(void) {
  if (lookAhead->tokenType == SB_LPAR) {
    eat(SB_LPAR);
    compileParam();
    while (lookAhead->tokenType == SB_SEMICOLON) {
      eat(SB_SEMICOLON);
      compileParam();
    }
    eat(SB_RPAR);
  }
}

// TK_IDENT SB_COLON <BasicType>
// KW_VAR TK_IDENT SB_COLON <BasicType>
void compileParam(void) {
  Object* param;
  Type* type;
  enum ParamKind paramKind;

  switch (lookAhead->tokenType) {
    case TK_IDENT: // truyền tham trị
      paramKind = PARAM_VALUE; 
      break;
    case KW_VAR: // nếu có var thì truyền tham chiếu
      eat(KW_VAR);
      paramKind = PARAM_REFERENCE;
      break;
    default:
      error(ERR_INVALID_PARAMETER, lookAhead->lineNo, lookAhead->colNo);
      break;
  }

  eat(TK_IDENT);
  // check if the parameter identifier is fresh in the block
  checkFreshIdent(currentToken->string);
  param = createParameterObject(currentToken->string, paramKind, symtab->currentScope->owner);
  eat(SB_COLON);
  type = compileBasicType();
  param->paramAttrs->type = type;
  declareObject(param);
}

// <Statement> <Statements2>
// <Statements2> ::= SB_SEMICOLON <Statement> <Statements2>
void compileStatements(void) {
  compileStatement();
  while (lookAhead->tokenType == SB_SEMICOLON) {
    eat(SB_SEMICOLON);
    compileStatement();
  }
}

// <AssignSt>
// <CallSt>
// <GroupSt>
// <IfSt>
// <WhileSt>
// <ForSt>
void compileStatement(void) {
  switch (lookAhead->tokenType) {
    case TK_IDENT:
      compileAssignSt();
      break;
    case KW_CALL:
      compileCallSt();
      break;
    case KW_BEGIN:
      compileGroupSt();
      break;
    case KW_IF:
      compileIfSt();
      break;
    case KW_WHILE:
      compileWhileSt();
      break;
    case KW_FOR:
      compileForSt();
      break;
    // EmptySt needs to check FOLLOW tokens
    case SB_SEMICOLON:
    case KW_END:
    case KW_ELSE:
      break;
    // Error occurs
    default:
      error(ERR_INVALID_STATEMENT, lookAhead->lineNo, lookAhead->colNo);
      break;
  }
}

void compileLValue(void) {
  Object* var;

  eat(TK_IDENT);
  // check if the identifier is a function identifier, or a variable identifier, or a parameter  
  var = checkDeclaredLValueIdent(currentToken->string);
  if (var->kind == OBJ_VARIABLE) // trường hợp biến mảng
    compileIndexes();
}

// <Variable> SB_ASSIGN <Expression>
// TK_IDENT SB_ASSIGN <Expression>
void compileAssignSt(void) {
  compileLValue();
  eat(SB_ASSIGN);
  compileExpression();
}

// KW_CALL TK_IDENT <Arguments>
void compileCallSt(void) {
  eat(KW_CALL);
  eat(TK_IDENT);
  // check if the identifier is a declared procedure
  checkDeclaredProcedure(currentToken->string);
  compileArguments();
}

// KW_BEGIN <Statements> KW_END
void compileGroupSt(void) {
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

// KW_IF <Condition> KW_THEN <Statement> <ElseSt>
void compileIfSt(void) {
  eat(KW_IF);
  compileCondition();
  eat(KW_THEN);
  compileStatement();
  if (lookAhead->tokenType == KW_ELSE) 
    compileElseSt();
}

// KW_ELSE <Statement>
void compileElseSt(void) {
  eat(KW_ELSE);
  compileStatement();
}

// KW_WHILE <Condition> KW_DO <Statement>
void compileWhileSt(void) {
  eat(KW_WHILE);
  compileCondition();
  eat(KW_DO);
  compileStatement();
}

// KW_FOR TK_IDENT SB_ASSIGN <Expression> KW_TO <Expression> KW_DO <Statement>
void compileForSt(void) {
  eat(KW_FOR);
  eat(TK_IDENT);

  // check if the identifier is a variable
  checkDeclaredVariable(currentToken->string);

  eat(SB_ASSIGN);
  compileExpression();

  eat(KW_TO);
  compileExpression();

  eat(KW_DO);
  compileStatement();
}

// <Expression>
void compileArgument(void) {
  compileExpression();
}

// SB_LPAR <Expression> <Argument2> SB_RPAR
void compileArguments(void) {
  switch (lookAhead->tokenType) {
  case SB_LPAR:
    eat(SB_LPAR);
    compileArgument();

    while (lookAhead->tokenType == SB_COMMA) {
      eat(SB_COMMA);
      compileArgument();
    }
    
    eat(SB_RPAR);
    break;
    // Check FOLLOW set 
  case SB_TIMES:
  case SB_SLASH:
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    break;
  default:
    error(ERR_INVALID_ARGUMENTS, lookAhead->lineNo, lookAhead->colNo);
  }
}

// <Expression> SB_EQ/SB_NEQ/SB_LE/SB_LT/SB_GE/SB_GT <Expresson>
void compileCondition(void) {
  compileExpression();

  switch (lookAhead->tokenType) {
    case SB_EQ:
      eat(SB_EQ);
      break;
    case SB_NEQ:
      eat(SB_NEQ);
      break;
    case SB_LE:
      eat(SB_LE);
      break;
    case SB_LT:
      eat(SB_LT);
      break;
    case SB_GE:
      eat(SB_GE);
      break;
    case SB_GT:
      eat(SB_GT);
      break;
    default:
      error(ERR_INVALID_COMPARATOR, lookAhead->lineNo, lookAhead->colNo);
  }

  compileExpression();
}

// SB_PLUS <Expression2>
// SB_MINUS <Expression2>
// <Expression2>
void compileExpression(void) {
  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    compileExpression2();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    compileExpression2();
    break;
  default:
    compileExpression2();
  }
}

// <Term> <Expression3>
void compileExpression2(void) {
  compileTerm();
  compileExpression3();
}

// SB_PLUS <Term> <Expression3>
// SB_MINUS <Term> <Expression3>
void compileExpression3(void) {
  switch (lookAhead->tokenType) {
    case SB_PLUS:
      eat(SB_PLUS);
      compileTerm();
      compileExpression3();
      break;
    case SB_MINUS:
      eat(SB_MINUS);
      compileTerm();
      compileExpression3();
      break;
      // check the FOLLOW set
    case KW_TO:
    case KW_DO:
    case SB_RPAR:
    case SB_COMMA:
    case SB_EQ:
    case SB_NEQ:
    case SB_LE:
    case SB_LT:
    case SB_GE:
    case SB_GT:
    case SB_RSEL:
    case SB_SEMICOLON:
    case KW_END:
    case KW_ELSE:
    case KW_THEN:
      break;
    default:
      error(ERR_INVALID_EXPRESSION, lookAhead->lineNo, lookAhead->colNo);
  }
}

// <Factor> <Term2>
void compileTerm(void) {
  compileFactor();
  compileTerm2();
}

// SB_TIMES <Factor> <Term2>
// SB_SLASH <Factor> <Term2>
void compileTerm2(void) {
  switch (lookAhead->tokenType) {
  case SB_TIMES:
    eat(SB_TIMES);
    compileFactor();
    compileTerm2();
    break;
  case SB_SLASH:
    eat(SB_SLASH);
    compileFactor();
    compileTerm2();
    break;
    // check the FOLLOW set
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    break;
  default:
    error(ERR_INVALID_TERM, lookAhead->lineNo, lookAhead->colNo);
  }
}

// <UnsignedConstant>
// <Variable>
// <FunctionApplication>
// SB_LPAR <Expression> SB_RPAR

// <UnsignedConstant> ::= TK_NUMBER
// <UnsignedConstant> ::= TK_IDENT
// <UnsignedConstant> ::= TK_CHAR
// <Variable> ::= TK_IDENT <Indexes>
// <FunctionApplication> ::= TK_IDENT <Arguments>
// 
void compileFactor(void) {
  Object* obj;

  switch (lookAhead->tokenType) {
    case TK_NUMBER:
      eat(TK_NUMBER);
      break;
    case TK_CHAR:
      eat(TK_CHAR);
      break;
    case TK_IDENT:
      eat(TK_IDENT);
      // check if the identifier is declared
      obj = checkDeclaredIdent(currentToken->string);

      switch (obj->kind) {
        case OBJ_CONSTANT:
          break;
        case OBJ_VARIABLE:
          compileIndexes();
          break;
        case OBJ_PARAMETER:
          break;
        case OBJ_FUNCTION:
          compileArguments();
          break;
        default: 
          error(ERR_INVALID_FACTOR,currentToken->lineNo, currentToken->colNo);
          break;
      }
      break;
    default:
      error(ERR_INVALID_FACTOR, lookAhead->lineNo, lookAhead->colNo);
  }
}

// SB_LSEL <Expression> SB_RSEL <Indexes>
void compileIndexes(void) {
  while (lookAhead->tokenType == SB_LSEL) {
    eat(SB_LSEL);
    compileExpression();
    eat(SB_RSEL);
  }
}

int compile(char *fileName) {
  if (openInputStream(fileName) == IO_ERROR)  // báo lỗi nếu không đọc được file
    return IO_ERROR;

  currentToken = NULL;
  lookAhead = getValidToken(); // lấy được token PROGRAM
  initSymTab(); // xây dựng bảng ký hiệu / khởi tạo globalObjectList

  compileProgram();

  printObject(symtab->program,0);

  cleanSymTab();

  free(currentToken);
  free(lookAhead);
  closeInputStream();
  return IO_SUCCESS;

}
