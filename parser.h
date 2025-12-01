//
// Created by Marek on 30.11.2025.
//

#ifndef IFJ_COMPILER_PARSER_H
#define IFJ_COMPILER_PARSER_H


#include <stdbool.h>


#define MAX_STACK 100


// GF - global frame (takto budou označeny globální proměné) - Pavel Bobek
// LF - local frame (takto budou označené lokální proměné) - Pavel Bobek
// TF - temporary frame (použité jen v generátoru kódu) - Pavel Bobek
enum FrameType {
    GF, LF, TF
};
enum VariableType {
    INT, BOOL, FLOAT, STRING
};
//Přidat další typy tokenů - Pavel Bobek
enum TokenType {
    VARIABLE, OPERATOR
};
//Seřazené typy operátorů podle priority tj. menší číslo = větší priorita - Pavel Bobek
enum OperatorType {
    MUL = 1,
    DIV = 1,
    ADD = 2,
    SUB = 2,
    LESS_THAN = 3,
    GREATER_THAN = 3,
    LESS_EQUAL_THAN = 3,
    GREATER_EQUAL_THAN = 3,
    IS = 4,
    EQUALS = 5,
    NOT_EQUALS = 5
};


typedef struct {

    enum FrameType frameType;
    enum VariableType variableType;
    char* varName;

} variable_t;

typedef struct {

    enum TokenType tokenType;
    variable_t* variable;
    enum OperatorType operatorType;

} token_t;

//Uzel stromu který bude obsahovat jeden token a ukazatel na jeho pravého a levého syna - Pavel Bobek
typedef struct Ast_t {

    token_t* token;
    struct Ast_t* right;
    struct Ast_t* left;

} ast_t;

typedef struct symTable_t SymTable;


//Teorie je taková že syntaktický analyzátor bude volat lexikální analyzátor který vytvoří a vrátí postupně všechny tokeny v jedné instrukci (až po středník) - Pavel Bobek
//Potom z této jedné instrukce vytvoří strom (ast) a dá ho do pole stromů - Pavel Bobek
//Finální pole stromů pak pošle do generátoru kódu, který pak vytvoří vnitřní kód - Pavel Bobek


//Budu používat v generátoru kódu
typedef struct {

    token_t* operatorToken;
    token_t* varToken1;
    token_t* varToken2;
    token_t* resultToken;

} instruction_t;

//Přidal jsem ještě moji implementaci zásobníku z prvního projektu do IAL - Pavel Bobek
typedef struct {
    instruction_t **InstructionArray;
    int topIndex;
} Stack;

int Stack_Init( Stack * );
bool Stack_IsEmpty( const Stack * );
bool Stack_IsFull( const Stack * );
void Stack_Top( const Stack *, instruction_t ** );
void Stack_Pop( Stack * );
void Stack_Push( Stack *, instruction_t * );
void Stack_Dispose( Stack * );

//Zatím funkce pro generátor kódu - Pavel Bobek
void getCode(const instruction_t* instruction, SymTable* symTable);
ast_t* getLastNode(ast_t* ast);
int createStack(ast_t** ast, int count, Stack* stack);
void ast_delete(ast_t* ast, ast_t* astDelete);
char* getVarCode(const token_t* token);

#endif //IFJ_COMPILER_PARSER_H
