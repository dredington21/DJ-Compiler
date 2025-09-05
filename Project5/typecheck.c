#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "symtbl.h"
#include "typecheck.h"

#define NO_TYPE -3
#define NULL_TYPE -2
#define NAT_TYPE -1
#define OBJECT_TYPE 0

// I have access to VarDecl, MethodDecl, and ClassDecl
// print error message and exit
  int printTypeError(char *message, int lineNumber) {
    printf(" Semantic analysis error: %s at line %d\n", message, lineNumber);
    exit(0);
  }
  // function to allow me to print node names
  const char* ASTNodeTypeNames[] = {
    "PROGRAM", 
    "CLASS_DECL_LIST", 
    "FINAL_CLASS_DECL",
    "NONFINAL_CLASS_DECL",
    "VAR_DECL_LIST", 
    "VAR_DECL",   
    "METHOD_DECL_LIST",
    "FINAL_METHOD_DECL",
    "NONFINAL_METHOD_DECL", 
    "NAT_TYPE", 
    "AST_ID", 
    "EXPR_LIST",
    "DOT_METHOD_CALL_EXPR",
    "METHOD_CALL_EXPR",
    "DOT_ID_EXPR",
    "ID_EXPR",
    "DOT_ASSIGN_EXPR",
    "ASSIGN_EXPR",
    "PLUS_EXPR",
    "MINUS_EXPR",
    "TIMES_EXPR",
    "EQUALITY_EXPR",
    "LESS_THAN_EXPR",
    "NOT_EXPR",
    "OR_EXPR",
    "ASSERT_EXPR",
    "IF_THEN_ELSE_EXPR",
    "WHILE_EXPR",
    "PRINT_EXPR",
    "READ_EXPR",
    "THIS_EXPR",
    "NEW_EXPR",
    "NULL_EXPR",
    "NAT_LITERAL_EXPR"
  };

// function to determine if there is a cycle
int hasCycle(int classType) {
    int current = classesST[classType].superclass;
    while (current != 0) {
        if (current == classType) return 1;  // cycle detected
        current = classesST[current].superclass;
    }
    return 0;  // no cycle
}

void checkVarDeclList(VarDecl *MainBlockST, int numMainBlockLocals) {
    // Main block var names have no duplicates
    // main block expression list is well typed
    for(int i=0; i<numMainBlockLocals; i++){
        VarDecl *varDecl = &MainBlockST[i];

        for(int j=i+1; j<numMainBlockLocals; j++){
            VarDecl *varDecl2 = &MainBlockST[j];
            if(strcmp(varDecl->varName, varDecl2->varName) == 0){
                printTypeError("Duplicate variable name in main block", varDecl->varNameLineNumber);
            }
        }
        if(varDecl->type == NO_TYPE){
            printTypeError("Variable has no type in main block", varDecl->varNameLineNumber);
        }
    }
    if(mainExprs != NULL && mainExprs->children == NULL){
        ASTList *exprList = mainExprs->children;
        while(exprList != NULL){
            typeExpr(exprList->data, -1, -1);
            exprList = exprList->next;
        }
    }
}
void checkClasses() {
    // check class names are unique 
    for(int i=0; i<numClasses; i++){
        if(hasCycle(i)){
            printTypeError("Cyclic inheritance", classesST[i].superclassLineNumber);
        }
        for(int j=i+1; j<numClasses; j++){
            if(strcmp(classesST[i].className, classesST[j].className) == 0){
                printTypeError("Duplicate class name", classesST[i].classNameLineNumber);
            }
        }
    }
    // perform checks on classes
    for(int i=0; i<numClasses; i++){
        ClassDecl *classDecl = &classesST[i];
        // check superclasses
        if (classDecl->superclass >= 0) {
            if (classDecl->superclass >= i){
                printTypeError("Class cannot extend self", classDecl->superclassLineNumber);
            }
            ClassDecl *superClassDecl = &classesST[classDecl->superclass];
            if (superClassDecl->isFinal){
                printTypeError("Class cannot extend final class", classDecl->superclassLineNumber);
            }
        }
        // check field types are legal
        for(int j=0; j<classDecl->numVars; j++){
            if(classDecl->varList[j].type <= NULL_TYPE){
                printTypeError("Variable has no type", classDecl->varList[j].varNameLineNumber);
            }
        }
        // check method return types and parameter types
        for(int j=0; j<classDecl->numMethods; j++){
            MethodDecl *methodDecl = &classDecl->methodList[j];
            if(methodDecl->returnType <= NO_TYPE){
                printTypeError("Illegal method return type", methodDecl->returnTypeLineNumber);
            }
            if(methodDecl->paramType <= NO_TYPE){
                printTypeError("Illegal method parameter type", methodDecl->paramTypeLineNumber);   
            }
            // check method names are unique within class
            for (int k = j+1; k < classDecl->numMethods; k++) {
                if (strcmp(classDecl->methodList[j].methodName, classDecl->methodList[k].methodName) == 0){
                    printTypeError("Duplicate method name in class", classDecl->methodList[j].methodNameLineNumber);
                }
            }
            // check method overrides
            int current = classDecl->superclass;
            while (current >= 0) {
                ClassDecl *supercls = &classesST[current];
                for (int k = 0; k < supercls->numMethods; k++) {
                    MethodDecl *superMethodDecl = &supercls->methodList[k];
                    if (strcmp(superMethodDecl->methodName, methodDecl->methodName) == 0) {
                        if (superMethodDecl->isFinal) {
                            printTypeError("Method cannot override final method", methodDecl->methodNameLineNumber);
                        }
                        if (superMethodDecl->paramType != methodDecl->paramType || superMethodDecl->returnType != methodDecl->returnType) {
                            printTypeError("Ovveriddn method signature mismatch", methodDecl->paramTypeLineNumber);
                            
                        }
                        }
                    }
                    current = supercls->superclass;
                }
                // check param and local var names are unique
                for(int k=0; k<methodDecl->numLocals; k++){
                    if (strcmp(methodDecl->paramName, methodDecl->localST[k].varName) == 0){
                        printTypeError("Duplicate local variable name ", methodDecl->localST[k].varNameLineNumber);
                    }
                    for(int l=k+1; l<methodDecl->numLocals; l++){
                        if (strcmp(methodDecl->localST[k].varName, methodDecl->localST[l].varName) == 0){
                            printTypeError("Duplicate local variable name ", methodDecl->localST[k].varNameLineNumber);
                        }
                    }
                }  
                if(methodDecl->bodyExprs != NULL){
                    //printf( "i and j: %d %d\n", i, j);
                    int bodyType = typeExpr(methodDecl->bodyExprs->children->data, i, j);
                    //printf("Node type: %s\n", ASTNodeTypeNames[methodDecl->bodyExprs->children->data->typ]);
                    //printf("body type: %d\n", bodyType);
                    //printf("return type: %d\n", methodDecl->returnType);
                    if(!isSubtype(bodyType, methodDecl->returnType)){
                        printTypeError("Method body not subtype of return type", methodDecl->returnTypeLineNumber);
                    }
                }
            }
        }
        for(int i=0; i<numClasses; i++){
            if(hasCycle(i)){
                printTypeError("Cyclic inheritance", classesST[i].superclassLineNumber);
            }
        }
        // search superclasses for conflicts
        for(int i=0; i<numClasses; i++){
            ClassDecl *classDeclf = &classesST[i];
            for(int j=0; j<classDeclf->numVars; j++){
                int currf = classDeclf->superclass;
                while (currf >= 0) {
                    ClassDecl *superf = &classesST[currf];
                    for (int k = 0; k < superf->numVars; k++) {
                        if (strcmp(superf->varList[k].varName, classDeclf->varList[j].varName) == 0){
                            printTypeError("Variable declared here and in superclass", classDeclf->varList[j].varNameLineNumber);
                        }
                    }
                    currf = superf->superclass;
                }
            }
        }
    }

//  Helper functions

// returns nonzero if sub is a subtype of super
int isSubtype(int sub, int super) {
    // finish implementing
    //printf("sub: %d, super: %d\n", sub, super);
    if(sub== NULL_TYPE && (super == NULL_TYPE || super >= OBJECT_TYPE)) return 1;
    if(sub == super) return 1;
    // this needs to be properly implemented
    if (sub >= OBJECT_TYPE && super == NAT_TYPE) return 1;
    if (sub == NAT_TYPE && super >= OBJECT_TYPE) return 1;
    
    if (sub >= OBJECT_TYPE) { 
       int parent = classesST[sub].superclass;
       //printf("parent: %d\n", parent);
       while (parent >= 0) {
           if (parent == super) return 1;
           parent = classesST[parent].superclass;
       }
    }
    // return not subtype
    return 0;
}


// join two types 
int join(int t1, int t2) {
    if (isSubtype(t1, t2)) return t2;
    else if (isSubtype(t2, t1)) return t1;
    return join (classesST[t1].superclass, t2);

}
// helper function for setStatic 
void setStatic(ASTree *t, int classNum, int memberNum) {
    t->staticClassNum = classNum;
    t->staticMemberNum = memberNum;
}
// function to find a variable
VarDecl *lookupVar(char *name, int classContainingExpr, int methodContainingExpr) {
    // check main block if not inside class
    if (classContainingExpr < 0) {
        for(int i=0; i<numMainBlockLocals; i++){
            if(strcmp(mainBlockST[i].varName, name) == 0) return &mainBlockST[i];
        }
        return NULL;
    }
    // check method parameter
    if(methodContainingExpr >= 0) {
        MethodDecl *method = &classesST[classContainingExpr].methodList[methodContainingExpr];
        if(strcmp(method->paramName, name) == 0){
            static VarDecl paramAsVar;
            paramAsVar.varName = method->paramName;
            paramAsVar.varNameLineNumber = method->paramNameLineNumber;
            paramAsVar.type = method->paramType;
            paramAsVar.typeLineNumber = method->paramTypeLineNumber;
            return &paramAsVar;
        }
    
        // check method locals
        for(int i=0; i<method->numLocals; i++){
            if(strcmp(method->localST[i].varName, name) == 0) return &method->localST[i];
            }
    }
    // check class fields
    ClassDecl *cls = &classesST[classContainingExpr];
    for(int i=0; i<cls->numVars; i++){
        if(strcmp(cls->varList[i].varName, name) == 0) return &cls->varList[i];
    }
    // not found
    return NULL; 
}


/* Entry point for type checking */
void typecheckProgram() {
    // level 3
    checkClasses();
    //level 2
    checkVarDeclList(mainBlockST, numMainBlockLocals);
    //level 1
    typeExprs(mainExprs, -1, -1);
}

// Returns the type of the expression AST in the given context.
int typeExpr(ASTree *t, int classContainingExpr, int methodContainingExpr) {
    
    if(t == NULL){
        printf("Internal TC error\n");
        exit(0);
    }
    // Initialize variables
    
    ASTree *left = NULL;
    ASTree *right = NULL;
    ASTree *expr= NULL;
    ASTree *thenExpr = NULL;
    ASTree *elseExpr = NULL;
    ASTree *body = NULL;
    ASTree *objectExpr = NULL;
    ASTree *idNode = NULL;
    ASTree *argExpr = NULL;
    VarDecl *v = NULL;
    MethodDecl *foundMethod = NULL;
    ClassDecl *cls = NULL;
    int leftType = -3;
    int rightType = -3;
    int exprType = -3;
    int thenType = -3;
    int elseType = -3;
    int classNum = -3;
    int objType = -3;
    int argType = -3;
    int currentClass = -3;
    // start switch here
    switch (t->typ) {
        // program, class, field, and method declarations:
        case NAT_TYPE:
            // do nothing?
            break;
        case AST_ID:
            // not sure if this is called when ID exists
            if(t->idVal == NULL) printTypeError("Identifier has no name ", t->lineNumber);
            v = lookupVar(t->idVal, classContainingExpr, methodContainingExpr);
            if(v == NULL) printTypeError("Undeclared var", t->lineNumber);
            return v->type;

        // expressions:
        case DOT_METHOD_CALL_EXPR:
            if(t->children == NULL || t->children->next == NULL || t->children->next->next == NULL) printTypeError(" Dot Method call missing arguments", t->lineNumber);
            objectExpr = t->children->data;
            idNode = t->children->next->data;
            argExpr = t->children->next->next->data;
            
            objType = typeExpr(objectExpr, classContainingExpr, methodContainingExpr);
            if (objType < 0) printTypeError("Dot method call on non-object", t->lineNumber);
            // if idNode child is AST_ID this is good if ID_EXPR we would need to check its children
            if (idNode->idVal == NULL) printTypeError("Dot method call has no name", t->lineNumber);
            // retrieve method name
            char *methodName = idNode->idVal;
            foundMethod = NULL;
            int searchClass = objType;
            while (searchClass >=0 && foundMethod == NULL) {
                cls = &classesST[searchClass];
                for(int i=0; i<cls->numMethods; i++){
                    if(strcmp(cls->methodList[i].methodName, methodName) == 0){
                        foundMethod = &cls->methodList[i];
                        break;
                    }
                }
                searchClass = cls->superclass;
            }
            if (foundMethod == NULL) printTypeError("Undeclared method", t->lineNumber);
            argType = typeExpr(argExpr, classContainingExpr, methodContainingExpr);
            if (!isSubtype(argType, foundMethod->paramType)) printTypeError("Dot method call argument type mismatch", t->lineNumber);
            //printf("Dot method call type: %d\n", foundMethod->returnType);
            //printf("found Method name: %s\n", foundMethod->methodName);

            setStatic(t, classContainingExpr, methodContainingExpr);
            // if in main block set static values to 0 for next stage
            if (classContainingExpr == -1 && methodContainingExpr == -1) setStatic(t, classContainingExpr+1, methodContainingExpr+1);
            return foundMethod->returnType;

        case METHOD_CALL_EXPR:
            if(t->children == NULL || t->children->data == NULL) printTypeError("Method has no name", t->lineNumber);

            if (classContainingExpr < 0) printTypeError("Method call outside class", t->lineNumber);
            currentClass = classContainingExpr;
            foundMethod = NULL;
            // search for method
            cls = &classesST[currentClass];
            while (currentClass >=0 && foundMethod == NULL) {
                
                for(int i=0; i<cls->numMethods; i++){
                    if(strcmp(cls->methodList[i].methodName, t->idVal) == 0){
                        foundMethod = &cls->methodList[i];
                        break;
                    }
                }
                currentClass = cls->superclass;
            }
            if (foundMethod == NULL) printTypeError("Undeclared method", t->lineNumber);
            if (t->children == NULL || t->children->data == NULL) printTypeError("Method call missing arguments", t->lineNumber);
           
            expr = t->children->data;
            exprType = typeExpr(expr, classContainingExpr, methodContainingExpr);
            if (!isSubtype(exprType, foundMethod->paramType)) printTypeError("Method call type mismatch", t->lineNumber);
            setStatic(t, classContainingExpr, methodContainingExpr);
            if (classContainingExpr == -1 && methodContainingExpr == -1) setStatic(t, classContainingExpr+1, methodContainingExpr+1);
            return foundMethod->returnType;

        case DOT_ID_EXPR:
            if(t->children == NULL || t->children->next == NULL) printTypeError("Missing parts in . expression", t->lineNumber);
            objectExpr = t->children->data;
            idNode = t->children->next->data;
            if (idNode->idVal == NULL) printTypeError("Dot method call has no name", t->lineNumber);

            objType = typeExpr(objectExpr, classContainingExpr, methodContainingExpr);
            if (objType < 0) printTypeError("Dot method call on non-object", t->lineNumber);
            v = NULL;
            currentClass = objType;
            while (currentClass >=0 && v == NULL) {
                cls = &classesST[currentClass];
                for(int i=0; i<cls->numVars; i++){
                    if(strcmp(cls->varList[i].varName, idNode->idVal) == 0){
                        v = &cls->varList[i];
                        break;
                    }
                }
                currentClass = cls->superclass;  
            }
            if (v == NULL) printTypeError("Undeclared var in dot expression", t->lineNumber);
            setStatic(t, classContainingExpr, methodContainingExpr);
            if (classContainingExpr == -1 && methodContainingExpr == -1) setStatic(t, classContainingExpr+1, methodContainingExpr+1);
            return v->type;

        case ID_EXPR:
            if(t->children == NULL || t->children->data == NULL) printTypeError("Identifier has no name", t->lineNumber);

            v = lookupVar(t->children->data->idVal, classContainingExpr, methodContainingExpr);
            if(v == NULL) printTypeError("Undeclared var", t->lineNumber);
            setStatic(t, classContainingExpr, methodContainingExpr);
            if (classContainingExpr == -1 && methodContainingExpr == -1) setStatic(t, classContainingExpr+1, methodContainingExpr+1);
            return v->type;
            
        case DOT_ASSIGN_EXPR:
            if (t->children == NULL || t->children->next == NULL || t->children->next->next == NULL) printTypeError("Dot assign missing args", t->lineNumber);
            
            objectExpr = t->children->data;
            idNode = t->children->next->data;
            right = t->children->next->next->data;

            objType = typeExpr(objectExpr, classContainingExpr, methodContainingExpr);
            if (objType < 0) printTypeError("Dot assign on non-object", t->lineNumber);
            if(idNode->idVal == NULL) printTypeError("Dot assign has no name", t->lineNumber);

            v = NULL;
            currentClass = objType;
            while (currentClass >=0 && v == NULL) {
                cls = &classesST[currentClass];
                for(int i=0; i<cls->numVars; i++){
                    if(strcmp(cls->varList[i].varName, idNode->idVal) == 0){
                        v = &cls->varList[i];
                        break;
                    }
                }
                currentClass = cls->superclass;  
            }
            if (v == NULL) printTypeError("Undeclared var in dot expression", t->lineNumber);
            rightType = typeExpr(right, classContainingExpr, methodContainingExpr);
            if (!isSubtype(rightType, v->type)) printTypeError("Dot assign type mismatch", t->lineNumber);
            setStatic(t, classContainingExpr, methodContainingExpr);
            if (classContainingExpr == -1 && methodContainingExpr == -1) setStatic(t, classContainingExpr+1, methodContainingExpr+1);
            return v->type;

        case ASSIGN_EXPR:
            if (t->children == NULL || t->children->next == NULL) printTypeError("Assignment missing lhs and rhs", t->lineNumber);
            //printf("Assign left type: %s\n", ASTNodeTypeNames[t->children->data->typ]);
           // printf("Assign right type: %s\n", ASTNodeTypeNames[t->children->next->data->typ]);
            left = t->children->data;
            right = t->children->next->data;
            leftType = typeExpr(left, classContainingExpr, methodContainingExpr);
            rightType = typeExpr(right, classContainingExpr, methodContainingExpr);
           // printf("Left type: %d, Right type: %d\n", leftType, rightType);
            if (!isSubtype(rightType, leftType)) printTypeError("Assignment type mismatch", t->lineNumber);
            //printf("right type: %d\n", rightType);
            setStatic(t, classContainingExpr, methodContainingExpr);
            if (classContainingExpr == -1 && methodContainingExpr == -1) setStatic(t, classContainingExpr+1, methodContainingExpr+1);
            return leftType;

            // Scary operators above ^ all need to set statics
        case PLUS_EXPR:
            if (t->children == NULL || t->children->next == NULL) printTypeError("Plus operands missing", t->lineNumber);

            left = t->children->data;
            right = t->children->next->data; 

            leftType = typeExpr(left, classContainingExpr, methodContainingExpr);
            rightType = typeExpr(right, classContainingExpr, methodContainingExpr);

            if (leftType != NAT_TYPE || rightType != NAT_TYPE) printTypeError("Plus operands not NAT", t->lineNumber);
            return NAT_TYPE;
        
        case MINUS_EXPR:
            if (t->children == NULL || t->children->next == NULL) printTypeError("Minus operands missing", t->lineNumber);

            left = t->children->data;
            right = t->children->next->data;

            leftType = typeExpr(left, classContainingExpr, methodContainingExpr);
            rightType = typeExpr(right, classContainingExpr, methodContainingExpr);

            if (leftType != NAT_TYPE || rightType != NAT_TYPE) printTypeError("Minus operands not NAT", t->lineNumber);
            return NAT_TYPE;

        case TIMES_EXPR:
            if (t->children == NULL || t->children->next == NULL) printTypeError("Times operands missing", t->lineNumber);

            left = t->children->data;
            right = t->children->next->data;

            leftType = typeExpr(left, classContainingExpr, methodContainingExpr);
            rightType = typeExpr(right, classContainingExpr, methodContainingExpr);

            if (leftType != NAT_TYPE || rightType != NAT_TYPE) printTypeError("Times operands not NAT", t->lineNumber);
            return NAT_TYPE;

        case EQUALITY_EXPR:
            // T1 needs to be a subtype of T2 or T2 a subtype of T1
            if(t->children == NULL || t->children->next == NULL) printTypeError("Equality operands missing", t->lineNumber);
            left = t->children->data;
            right = t->children->next->data;
            leftType = typeExpr(left, classContainingExpr, methodContainingExpr);
            rightType = typeExpr(right, classContainingExpr, methodContainingExpr);
            if (isSubtype(leftType, rightType) || isSubtype(rightType, leftType)) return NAT_TYPE;
            else printTypeError("Equality operands not NAT or subtypes", t->lineNumber);
            return NO_TYPE; // should never get here
            
        case LESS_THAN_EXPR:
            if(t->children == NULL || t->children->next == NULL) printTypeError("Less than operands missing", t->lineNumber);
            left = t->children->data;
            right = t->children->next->data;
            leftType = typeExpr(left, classContainingExpr, methodContainingExpr);
            rightType = typeExpr(right, classContainingExpr, methodContainingExpr);
            if (leftType != NAT_TYPE || rightType != NAT_TYPE) printTypeError("Less than operands not NAT", t->lineNumber);
            return NAT_TYPE;

        case NOT_EXPR:
            if(t->children == NULL) printTypeError("Not operand missing", t->lineNumber);
            expr = t->children->data;
            exprType = typeExpr(expr, classContainingExpr, methodContainingExpr); 
            if (exprType != NAT_TYPE) printTypeError("Not operand not NAT", t->lineNumber);
            return NAT_TYPE;

        case OR_EXPR:
            // going to assume NAT_TYPE here
            if(t->children == NULL || t->children->next == NULL) printTypeError("Or operands missing", t->lineNumber);
            left = t->children->data;
            right = t->children->next->data;
            leftType = typeExpr(left, classContainingExpr, methodContainingExpr);
            rightType = typeExpr(right, classContainingExpr, methodContainingExpr);
            if (leftType != NAT_TYPE || rightType != NAT_TYPE) printTypeError("Or operands not NAT", t->lineNumber);
            return NAT_TYPE;
            
        case ASSERT_EXPR:
            if(t->children == NULL) printTypeError("Assert operand missing", t->lineNumber);
            expr = t->children->data;
            exprType = typeExpr(expr, classContainingExpr, methodContainingExpr); 
            if (exprType != NAT_TYPE) printTypeError("Assert operand not NAT", t->lineNumber);
            return NAT_TYPE; 

        case IF_THEN_ELSE_EXPR:
            if(t->children == NULL || t->children->next == NULL || t->children->next->next == NULL) printTypeError("If-then-else operands missing", t->lineNumber);
            expr = t->children->data;
            thenExpr = t->children->next->data->children->data;
            elseExpr = t->children->next->next->data->children->data;
            rightType = typeExpr(t->children->next->next->data->children->data->children->data, classContainingExpr, methodContainingExpr);
            //printf("rightType: %d\n", rightType);
            //printf("Node thenExpr: %s\n", ASTNodeTypeNames[thenExpr->typ]);
            //printf("Node elseExpr: %s\n", ASTNodeTypeNames[elseExpr->typ]);
            exprType = typeExpr(expr, classContainingExpr, methodContainingExpr);
            if (exprType != NAT_TYPE) printTypeError("If-then-else condition not NAT", t->lineNumber);
            thenType = typeExpr(thenExpr, classContainingExpr, methodContainingExpr);
            elseType = typeExpr(elseExpr, classContainingExpr, methodContainingExpr);
            if (thenType == NAT_TYPE && thenType == elseType) return NAT_TYPE;
            if (thenType >=0 && elseType >=0) return join(thenType, elseType);
            if (isSubtype(thenType, elseType)) return elseType;
            if (isSubtype(elseType, thenType)) return thenType;
            //printf("then: %d, else: %d\n", thenType, elseType);
            printTypeError("If-then-else operands not NAT or joinable", t->lineNumber);
            return NO_TYPE; // should never get here

        case WHILE_EXPR:
            if(t->children == NULL || t->children->next == NULL) printTypeError("While operands missing", t->lineNumber);
            expr = t->children->data;
            body = t->children->next->data;
            exprType = typeExpr(expr, classContainingExpr, methodContainingExpr);
            if (exprType != NAT_TYPE) printTypeError("While condition not NAT", t->lineNumber);
            // type check body but don't care about actual type of it
            typeExpr(body, classContainingExpr, methodContainingExpr);
            return NAT_TYPE;

        case PRINT_EXPR:
            exprType = typeExpr(t->children->data, classContainingExpr, methodContainingExpr);
            if (exprType != NAT_TYPE) printTypeError("non-nat type in printNat", t->lineNumber);
            return NAT_TYPE;

        case READ_EXPR:
            if(t->children->next != NULL) printTypeError("Read nat should not have arguements", t->lineNumber);
            return NAT_TYPE;
            
        case THIS_EXPR:
            // check for level 3
            if(classContainingExpr <= -1) printTypeError("'this' used outside of class", t->lineNumber);
            return classContainingExpr;
            
        case NEW_EXPR:
            // new C() has type C 
            if(t->children->data->idVal == NULL) printTypeError("Missing class name", t->lineNumber);
            classNum = classNameToNumber(t->children->data->idVal);
            if(classNum < 0) printTypeError("Unknown class name", t->lineNumber);
            return classNum;

        case NULL_EXPR:
            return -2;

        case NAT_LITERAL_EXPR:
            //printf("This is a a nat!\n");
            return -1;

        default:
            fprintf(stderr, "Unknown AST node type at line %d\n", t->lineNumber);
            return -4;
    }
    
}

/* Returns the type of the EXPR_LIST AST in the given context. */
int typeExprs(ASTree *t, int classContainingExprs, int methodContainingExprs) {
    int returnType;
    
    ASTList *childListIterator = t->children;
    while (childListIterator != NULL) {
      returnType = typeExpr(childListIterator->data, classContainingExprs, methodContainingExprs);
      childListIterator = childListIterator->next;
    }
    return returnType;
  }
  
