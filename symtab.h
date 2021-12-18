/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#ifndef __SYMTAB_H__
#define __SYMTAB_H__

#include "token.h"

enum TypeClass { // các kiểu dữ liệu có thể có trong mọi chương trình
  TP_INT,
  TP_CHAR,
  TP_ARRAY
};

enum ObjectKind { // các đối tượng cần lưu thông tin
  OBJ_CONSTANT,
  OBJ_VARIABLE,
  OBJ_TYPE,
  OBJ_FUNCTION,
  OBJ_PROCEDURE,
  OBJ_PARAMETER,
  OBJ_PROGRAM
};

enum ParamKind {
  PARAM_VALUE,
  PARAM_REFERENCE
};

struct Type_ { // lưu trữ thông tin về kiểu
  enum TypeClass typeClass;
  // hai cái dưới chỉ sử dụng khi kiểu là kiểu mảng
  int arraySize; // miền chỉ số (chỉ số chạy từ 1 hoặc 0)
  struct Type_ *elementType; // kiểu của các phần tử
};

typedef struct Type_ Type;
typedef struct Type_ BasicType;


struct ConstantValue_ {
  enum TypeClass type;
  union { // giá trị của hằng
    int intValue;
    char charValue;
  };
};

typedef struct ConstantValue_ ConstantValue;

struct Scope_;
struct ObjectNode_;
struct Object_;

// cụ thể trong thuộc tính cần lưu cho từng đối tượng
struct ConstantAttributes_ {
  ConstantValue* value; // giá trị hằng
};

struct VariableAttributes_ {
  Type *type; // kiểu của biến: interger, character hay array
  struct Scope_ *scope; // phạm vi của biến
};

struct TypeAttributes_ {
  Type *actualType;
};

struct ProcedureAttributes_ {
  struct ObjectNode_ *paramList;
  struct Scope_* scope;
};

struct FunctionAttributes_ {
  struct ObjectNode_ *paramList; // danh sách tham số, các tham số hình thức được đưa vào cả paramList và scope->objList (lưu các đối tượng được định nghĩa trong block)
  Type* returnType;
  struct Scope_ *scope;
};

struct ProgramAttributes_ {
  struct Scope_ *scope;
};

struct ParameterAttributes_ {
  enum ParamKind kind; // tham biến hay tham trị
  Type* type;
  struct Object_ *function; // đối tượng là chủ tham số đó (hàm hoặc thủ tục)
};

typedef struct ConstantAttributes_ ConstantAttributes;
typedef struct TypeAttributes_ TypeAttributes;
typedef struct VariableAttributes_ VariableAttributes;
typedef struct FunctionAttributes_ FunctionAttributes;
typedef struct ProcedureAttributes_ ProcedureAttributes;
typedef struct ProgramAttributes_ ProgramAttributes;
typedef struct ParameterAttributes_ ParameterAttributes;

struct Object_ { // các thuộc tính cần lưu cho mỗi đối tượng
  char name[MAX_IDENT_LEN];
  enum ObjectKind kind;
  union { // thông tin đi kèm với cụ thể kiểu đối tượng bên trên (mỗi đối tượng chỉ thuộc một trong các kiểu bên dưới)
    ConstantAttributes* constAttrs;
    VariableAttributes* varAttrs;
    TypeAttributes* typeAttrs;
    FunctionAttributes* funcAttrs;
    ProcedureAttributes* procAttrs;
    ProgramAttributes* progAttrs;
    ParameterAttributes* paramAttrs;
  };
};

typedef struct Object_ Object;

struct ObjectNode_ { // danh sách liên kết, các node là Object
  Object *object;
  struct ObjectNode_ *next;
};

typedef struct ObjectNode_ ObjectNode;

struct Scope_ { // phạm vi của một block
  ObjectNode *objList; // danh sách các đối tượng trong block (bảng ký hiệu cho block / các đối tượng khác trong block này)
  Object *owner; // block này là hàm, thủ tục hay chương trình
  struct Scope_ *outer; // phạm vi bao ngoài
};

typedef struct Scope_ Scope;

struct SymTab_ {
  Object* program; // khối chương trình chính
  Scope* currentScope; // phạm vi hiện tại
  ObjectNode *globalObjectList; // các đối tượng toàn cục, có thể sử dụng mà không cần khai báo
};

typedef struct SymTab_ SymTab;

// các hàm tạo kiểu, xin cấp phát bộ nhớ
Type* makeIntType(void);
Type* makeCharType(void);
Type* makeArrayType(int arraySize, Type* elementType);
Type* duplicateType(Type* type); // copy thông tin của kiểu đầu vào, giả sử khi khai báo a = b

int compareType(Type* type1, Type* type2);
void freeType(Type* type);

// Các hàm tạo hằng, xin cấp phát bộ nhớ để lưu thông tin về hằng
ConstantValue* makeIntConstant(int i);
ConstantValue* makeCharConstant(char ch);
ConstantValue* duplicateConstantValue(ConstantValue* v);

Scope* createScope(Object* owner, Scope* outer);

// hàm tạo các đối tượng để bổ sung vào bảng ký hiệu
Object* createProgramObject(char *programName);
Object* createConstantObject(char *name);
Object* createTypeObject(char *name);
Object* createVariableObject(char *name);
Object* createFunctionObject(char *name);
Object* createProcedureObject(char *name);
Object* createParameterObject(char *name, enum ParamKind kind, Object* owner);

Object* findObject(ObjectNode *objList, char *name);

void initSymTab(void);
void cleanSymTab(void);
void enterBlock(Scope* scope); // cập nhật lại current block
void exitBlock(void); // chuyển lại current block ra phạm vi bên ngoài
void declareObject(Object* obj); // đưa một đối tượng vào bảng ký hiệu

#endif
