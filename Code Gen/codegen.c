#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "codegen.h"
#include "symtbl.h"

#define MAX_DISM_ADDR 65535

// global for the DISM output file
FILE *fout;
// Global to remember the next unique label number to use
unsigned int labelNumber = 0;
int needVtable = 0; // flag to indicate if we need a vtable

// declare mutually recursive functions (defs and docs appera below)
void codeGenExpr (ASTree *t, int ClassNumber, int MethodNumber);
void codeGenExprs(ASTree *expList, int ClassNumber, int MethodNumber);
// print message and exit under an exceptional condition
void internalCGerror(char *msg) {
    fprintf(stderr, "Internal Code Generator Error: %s\n", msg);
    exit(1);
}


void addCode(char *code, ...) {
    va_list args;
    va_start(args, code);
    
    // Check if the first character is '#'
    if (code[0] != '#') {
        fprintf(fout, "     "); // 7 spaces for label
    }
    
    vfprintf(fout, code, args);
    va_end(args);
}


// using the global classesST, calculate the total number of fields,
// including inherited fields, in an object of the given type
int getNumObjectFields(int type) {
    int value = classesST[type].numVars;
    if (classesST[type].superclass <=0 ) return value;
    value += getNumObjectFields(classesST[type].superclass);
    return value; 
}

// generate code that increments the stack pointer
void incSP() {
    addCode("mov 1 1\n");
    addCode("add 6 6 1; #SP++\n");
}
// generate code that decrements the stack pointer
void decSP() {
    addCode("mov 1 1\n");
    addCode("sub 6 6 1; #SP--\n");
    addCode("blt 5 6 #labelNum%d ; branch if HP<SP\n", labelNumber);
    addCode("mov 1 77 ; error code 77 no stack memory\n");
    addCode("hlt 1; out of stack memory!!\n");
    addCode("#labelNum%d: mov 0 0\n", labelNumber);
    labelNumber++;
}

// output code to check for a null value at the top of the stack
// if the top stack value ast M(SP+1)) is null (0), the DISM code output will halt
void checkNullDereference() {
    addCode("add 1 6 1; 1 = SP + 1\n");
    addCode("lod 1 1 0; 1 = M(SP+1)\n");
    // check if loaded value is 0
    addCode("beq 1 0 #halt%d\n", labelNumber);
    addCode("jmp 0 #labelNum%d\n", labelNumber);
    addCode("#halt%d: mov 0 0\n", labelNumber);
    addCode("mov 1 77\n");
    addCode("hlt 1; Null pointer dereference\n");
    addCode("#labelNum%d: mov 0 0\n", labelNumber);
    labelNumber++;
}

/* generate DISM code for the given single expression, which appears in the given class and method (or maing block).
if classNumber <0 then methodNumber may be anything and we assume
we are generating code for the main block*/
void codeGenExpr(ASTree *t, int ClassNumber, int MethodNumber) {
    int trueLabel, falseLabel, endLabel, returnLabel, nextLabel, elseLabel, failLabel, passLabel, whileLabel;
    if (!t) return;
    switch (t->typ) {
    case NAT_TYPE:
         addCode("mov 1 %d\n", t->natVal);
         addCode("str 6 0 1\n");
         decSP();
         break;
    case AST_ID:
         if (ClassNumber <0 ) {
             for (int i = 0; i < numMainBlockLocals; i++) {
                 if (strcmp(mainBlockST[i].varName, t->idVal) == 0) {
                     addCode("mov 1 %d\n", i);
                     addCode("str 6 0 1\n");
                     //printf("hello!\n");
                     decSP();
                     break;
                 }
             }
         }
         else {
             for (int i = 0; i < classesST[ClassNumber].methodList[MethodNumber].numLocals; i++) {
                 if (strcmp(classesST[ClassNumber].methodList[MethodNumber].localST[i].varName, t->idVal) == 0) {
                     addCode("mov 1 %d\n", i);
                     addCode("str 6 0 1\n");
                     decSP();
                     break;
                 }
             }
         }
         break;

    //level 3
    case DOT_METHOD_CALL_EXPR:
    returnLabel = labelNumber++;

    // Push the return label onto the stack
    addCode("str 6 0 %d; push retLabel on stack\n", returnLabel);
    decSP();

    // pushes this on stack
    codeGenExpr(t->children->data, ClassNumber, MethodNumber);
    checkNullDereference();

    // Push the static class number onto the stack
    addCode("mov 1 %d\n", t->staticClassNum);
    addCode("str 6 0 1; push class number on stack\n");
    decSP();

    // Push the static method number onto the stack
    addCode("mov 1 %d\n", t->staticMemberNum);
    addCode("str 6 0 1; push method number on stack\n");
    decSP();

    // Evaluate the method arguments
    codeGenExprs(t->children->next->data, ClassNumber, MethodNumber);

    // Jump to the virtual table to resolve the method call
    addCode("jmp 0 #VTABLE\n");

    // Return label for after the method call
    addCode("#%d: mov 0 0\n", returnLabel);
    break;
    
    case METHOD_CALL_EXPR:
        returnLabel = labelNumber++;
        needVtable = 1;
        addCode("str 6 0 %d; push retLabel on stack\n", returnLabel);
        decSP();
        
        // pushes caller # on stack
        codeGenExpr(t->children->data, ClassNumber, MethodNumber);
        checkNullDereference();

        addCode("mov 1 %d\n",t->staticClassNum);
        addCode("str 6 0 1; push class number on stack\n");
        decSP();
        addCode("mov 1 %d\n",t->staticMemberNum);
        addCode("str 6 0 1; push method number on stack\n");
        decSP();
         // leave on stack
        codeGenExpr(t->children->next->data, ClassNumber, MethodNumber);
        addCode("jmp 0 #VTABLE\n");
        addCode("#%d: mov 0 0\n", returnLabel);
        break;
    
    case DOT_ID_EXPR:
    
        codeGenExpr(t->children->data, ClassNumber, MethodNumber);
        checkNullDereference();
        incSP();
        addCode("lod 1 6 0; load mem of r1\n");
        addCode("add 1 1 %d; r1 = offset + base address \n", t->staticMemberNum);
        addCode("lod 1 1 0; load mem at [A]\n");
        addCode("str 6 0 1; push r1 on stack\n");
        decSP();
        break;
    
    case ID_EXPR: 
        if(t->staticClassNum == 0){
            addCode("sub 1 7 %d; r1 = FP - %d\n", t->staticMemberNum);
            addCode("lod 1 1 0; r1 = M(r1)\n");

            addCode("str 6 0 1; push r1 on stack\n");
            decSP();
            //codeGenExpr(t->children->data, ClassNumber, MethodNumber);
            // may need to do this instead?
        }
        else{
            addCode("sub 1 7 1; r1 = (this)\n");
            addCode("lod 1 1 0; load 'this' address\n");
            // does this need a null dereference check?
            addCode("add 1 1 %d; r1 = offset + base address \n", t->staticMemberNum);
            addCode("lod 1 1 0; load mem at [A]\n");
            addCode("str 6 0 1; push r1 on stack\n");
            decSP();
        }
        break;
    
    case DOT_ASSIGN_EXPR:
        codeGenExpr(t->children->next->data, ClassNumber, MethodNumber);
        codeGenExpr(t->children->data, ClassNumber, MethodNumber);
        checkNullDereference();

        addCode("lod 1 6 1; load base address of E1\n");
        addCode("add 1 1 %d; r1 = offset + base address \n", t->staticMemberNum);
        addCode("str 6 1 1; store A on stack\n");

        addCode("lod 1 6 1; load address of A\n");
        addCode("lod 2 6 2; load value of r\n");
        addCode("str 2 1 0; store r at [A]\n");
        // make sure we leave r on the stack only
        incSP();
        break;


    case ASSIGN_EXPR:
        if(t->staticClassNum == 0){
            addCode("sub 1 7 %d; r1 = FP - offset\n", t->staticMemberNum);
            addCode("lod 1 1 0; r1 = M(r1)\n");
            addCode("str 6 0 1; push r1 on stack\n");
            decSP();
        }
        else{
            addCode("sub 1 7 1; r1 = (this)\n");
            addCode("lod 1 1 0; load 'this' address\n");
            addCode("add 1 1 %d; r1 = offset + base address \n", t->staticMemberNum);
            addCode("lod 1 1 0; load mem at [A]\n");
            addCode("str 6 0 1; push r1 on stack\n");
            decSP();
        }
        // we can leave this value on stack
        codeGenExpr(t->children->data, ClassNumber, MethodNumber);
        addCode("lod 1 6 1; load value r\n");
        addCode("lod 2 6 2; get address A\n");
        addCode("str 2 1 0; store r at [A]\n");
        break;

    case PLUS_EXPR:
         codeGenExpr(t->children->data, ClassNumber, MethodNumber);
         codeGenExpr(t->children->next->data, ClassNumber, MethodNumber);
         addCode("lod 1 6 2; load mem of r1\n");
         addCode("lod 2 6 1; load mem of r2\n");
         addCode("add 1 1 2; r1 = r1 + r2\n");
         addCode("str 6 2 1; store result at +2\n");
         incSP();
         break;
         
    case MINUS_EXPR:
         codeGenExpr(t->children->data, ClassNumber, MethodNumber);
         codeGenExpr(t->children->next->data, ClassNumber, MethodNumber);
         addCode("lod 1 6 2; load mem of r1\n");
         addCode("lod 2 6 1; load mem of r2\n");
         addCode("sub 1 1 2; r1 = r1 - r2\n");
         addCode("str 6 2 1; store result at +2\n");
         incSP();
         break;

    case TIMES_EXPR:
         codeGenExpr(t->children->data, ClassNumber, MethodNumber);
         codeGenExpr(t->children->next->data, ClassNumber, MethodNumber);
         addCode("lod 1 6 2; load mem of r1\n");
         addCode("lod 2 6 1; load mem of r2\n");
         addCode("mul 1 1 2; r1 = r1 * r2\n");
         addCode("str 6 2 1; store result at +2\n");
         incSP();
         break;

    case EQUALITY_EXPR:
         trueLabel = labelNumber++;
         endLabel = labelNumber++;
         codeGenExpr(t->children->data, ClassNumber, MethodNumber);
         codeGenExpr(t->children->next->data, ClassNumber, MethodNumber);
         addCode("lod 1 6 2; load mem of r1\n");
         addCode("lod 2 6 1; load mem of r2\n");
         addCode("beq 1 2 #true%d\n", trueLabel);
         addCode("mov 1 0\n");
         addCode("jmp 0 #end%d\n", endLabel);
         addCode("#true%d: mov 0 0\n", trueLabel);
         addCode("mov 1 1 ; condition true\n");
         addCode("#end%d: mov 0 0\n", endLabel);
         addCode("str 6 2 1 ; return final result on stack\n");
         incSP();
         break;

    case LESS_THAN_EXPR:
         trueLabel = labelNumber++;
         endLabel = labelNumber++;
         codeGenExpr(t->children->data, ClassNumber, MethodNumber);
         codeGenExpr(t->children->next->data, ClassNumber, MethodNumber);
         addCode("lod 1 6 2; load mem of r1\n");
         addCode("lod 2 6 1; load mem of r2\n");
         addCode("blt 1 2 #true%d\n", trueLabel);
         addCode("mov 1 0\n");
         addCode("jmp 0 #end%d\n", endLabel);
         addCode("#true%d: mov 0 0 ;\n", trueLabel);
         addCode("mov 1 1 ; condition true\n");
         addCode("#end%d: mov 0 0\n", endLabel);
         addCode("str 6 2 1 ; return final result on stack\n");
         incSP();
         break;

    case NOT_EXPR:
        trueLabel = labelNumber++;
        endLabel = labelNumber++;
        codeGenExpr(t->children->data, ClassNumber, MethodNumber);
        addCode("lod 1 6 1; load mem of r1\n");
        addCode("beq 0 1 #true%d ; not is true\n", trueLabel);
        addCode("mov 1 0\n");
        addCode("jmp 0 #end%d ; not is false\n", endLabel);
        addCode("#true%d: mov 0 0 ; not is false\n", trueLabel);
        addCode("mov 1 1 ; not is true\n");
        addCode("#end%d: mov 0 0\n", endLabel);
        addCode("str 6 1 1 ; return final result on stack\n");
        break;

    case OR_EXPR:
         nextLabel = labelNumber++;
         trueLabel = labelNumber++;
         falseLabel = labelNumber++;
         endLabel = labelNumber++;
         codeGenExpr(t->children->data, ClassNumber, MethodNumber);
         codeGenExpr(t->children->next->data, ClassNumber, MethodNumber);
         addCode("lod 1 6 2; load mem of r1\n");
         addCode("beq 0 1 #next%d\n", nextLabel);
         addCode("jmp 0 #true%d\n", trueLabel);
         addCode("#next%d: mov 0 0 ; condition true\n", nextLabel);
         addCode("lod 1 6 1; load mem of r2\n");
         addCode("beq 0 1 #false%d\n", falseLabel);
         addCode("#true%d: mov 0 0\n", trueLabel);
         addCode("mov 1 1 ; condition true\n");
         addCode("jmp 0 #end%d ; jump to end\n", endLabel);
         addCode("#false%d: mov 0 0\n", falseLabel);
         addCode("mov 1 0 ; condition false\n");
         addCode("#end%d: mov 0 0\n", endLabel);
         addCode("str 6 2 1 ; return final result on stack\n");
         incSP();
         break;
        

    case ASSERT_EXPR:
         failLabel = labelNumber++;
         passLabel = labelNumber++;
         codeGenExpr(t->children->data, ClassNumber, MethodNumber);
         // we can just leave this value on stack
         addCode("lod 1 6 1; load mem of r1\n");
         addCode("beq 0 1 #fail%d\n", failLabel);
         addCode("jmp 0 #pass%d\n", passLabel);
         addCode("#fail%d: mov 0 0\n", failLabel);
         addCode("halt 0 ; assertion failed\n");
         
         addCode("#pass%d: mov 0 0 ; assertion passed\n", passLabel);
         break;

    case IF_THEN_ELSE_EXPR:
         elseLabel = labelNumber++;
         endLabel = labelNumber++;
         codeGenExpr(t->children->data, ClassNumber, MethodNumber);
         addCode("lod 1 6 1; load mem of sp+1\n");
         addCode("beq 0 1 #else%d\n", elseLabel);
         incSP();
         codeGenExpr(t->children->next->data, ClassNumber, MethodNumber);
         addCode("jmp 0 #end%d\n", endLabel);
         incSP();
         addCode("#else%d: mov 0 0\n", elseLabel);
         codeGenExpr(t->children->next->next->data, ClassNumber, MethodNumber);
         addCode("#end%d: mov 0 0\n", endLabel);
         break;

    case WHILE_EXPR:
        whileLabel = labelNumber++;
        endLabel = labelNumber++;
        addCode("#while%d: mov 0 0\n", whileLabel);
        codeGenExpr(t->children->data, ClassNumber, MethodNumber);
        incSP();
        addCode("lod 1 6 0; load mem of r1\n");
        addCode("beq 0 1 #end%d; condition not true\n", endLabel);
        codeGenExpr(t->children->next->data, ClassNumber, MethodNumber);
        addCode("jmp 0 #while%d ; jump to start of while\n", whileLabel);
        addCode("#end%d: mov 0 0\n", endLabel);
        addCode("str 6 1 0 ; end loop and store 0 on top of stack\n");
        break;

    case PRINT_EXPR:
         codeGenExpr(t->children->data, ClassNumber, MethodNumber);
         // we can just leave this value on stack
         addCode("lod 1 6 1; load mem of r1 for printing\n");
         addCode("ptn 1\n");
         break;
    case READ_EXPR:
         addCode("rdn 1;  load input to r1\n");
         addCode("str 6 0 1; store input at sp\n");
         decSP();
         break;
    case THIS_EXPR:
         // load this from FP-1
         addCode("sub 1 7 1; r1 = FP - 1\n");
         addCode("lod 1 1 0; r1 = M(r1)\n");

         addCode("str 6 0 1; push this on stack\n");
         decSP();
         break;
    // level 3
    case NEW_EXPR:
         int numFields = 0;
         int currentClass = t->staticClassNum;
         while (currentClass > 0) {
             numFields += getNumObjectFields(currentClass);
             currentClass = classesST[currentClass].superclass;
         }
         //check HP < max_dism address
        addCode("add 1 5 1; r1 = HP + 1\n");
        addCode("add 1 1 %d; r1 = HP + %d\n +1", numFields, numFields);
        addCode("mov 2 65535; r2 = 65535\n");
        addCode("blt 1 2 #goodHP%d\n", labelNumber);
        addCode("mov 1 77 ;\n");
        addCode("hlt 1; out of heap memory!!\n");
        addCode("#goodHP%d: mov 0 0\n", labelNumber);
        labelNumber++;

         addCode("mov 1 1\n");
         for (int i = 0; i < numFields; i++) {
             addCode("str 5 0 0; allocate a field\n");
             addCode("add 5 5 1; HP++\n");
         }
         addCode("mov 2 %d; R2 = new object type\n", t->staticClassNum);
         addCode("str 5 0 2;");
         addCode("str 6 0 5; push new-obj address\n");
         addCode("add 5 5 1; HP++\n");
         decSP();
         break;
    case NULL_EXPR:
         addCode("str 6 0 0; push null on stack\n");
         decSP();
         break;

    case NAT_LITERAL_EXPR:
    if (t->typ == NAT_LITERAL_EXPR) {
        addCode("mov 1 %d\n", t->natVal);
        addCode("str 6 0 1; M[SP] <-R1 (a nat literal)\n");
        decSP();
        break;
    }
    
    }

}
/* GENERATE dism CODE FOR AN EXPRESSION LIST, WHICH APPEARS IN
THE GIVEN CLASS AND METHOD or main block
If classNumber <0 then methodNumber may be anything and we assume 
we are generating code for the programs main block*/
void codeGenExprs(ASTree *expList, int ClassNumber, int MethodNumber) {
    ASTList *childIterator = expList->children;
    while (childIterator != NULL) {
        codeGenExpr(childIterator->data, ClassNumber, MethodNumber);
        childIterator = childIterator->next;
        if (childIterator != NULL) {
            incSP();
        }
    }
}

/* generate DISM code as the prologue to the given method or main block. If classNumber < 0
then methodNumber may be anything and we assume we are generating code for the progtrams main block*/
void genPrologue(int ClassNumber, int MethodNumber) {
    // For the main block
    if (ClassNumber < 0) {
        addCode("mov 7 65535   ; initialize FP\n");
        addCode("mov 6 65535   ; initialize SP\n");
        addCode("mov 5 1       ; initialize HP\n");

        // Allocate stack space for main block locals
        for (int i = 0; i < numMainBlockLocals; i++) {
            addCode("str 6 0 0  ; Allocate stack space for main local %d\n", i);
            decSP();
        }

        addCode("mov 0 0       ; BEGIN METHOD/MAIN-BLOCK BODY\n");
    }
    // For a method in a class
    else {
        // Allocate stack space for method locals
        for (int i = 0; i < classesST[ClassNumber].methodList[MethodNumber].numLocals; i++) {
            addCode("str 6 0 0  ; Allocate stack space for method local %d\n", i);
            decSP();
        }

        // Save the old FP
        addCode("str 6 0 7   ; Save old FP\n");
        decSP();

        // Update FP to point to the current SP
        addCode("add 6 7 0   ; Have SP point to FP\n");

        // Adjust FP to account for the return address
        addCode("mov 1 2     ; Load offset for return address\n");
        addCode("sub 7 7 1   ; FP = FP - 2 to point to RA\n");
    }
}

/*Generate DISM code as the epiliogue to the given method or main block. If classNumber <0 then methodNumber
may be anything and we assume we are generating code for the program's main block*/
void genEpilogue(int ClassNumber, int MethodNumber) {
    if (ClassNumber < 0) {

        addCode("hlt 0     ; normal program termination\n");
    }
    else{
        addCode("lod 1 7 1  ; load return addr (M(FP-1))\n");
        addCode("lod 7 7 2  ; Restore caller's FP (M(FP-2))\n");
        addCode("lod 6 7 3  ; Restore caller's SP (M(FP-3))\n");
        addCode("jmp 0 1     ; return to caller\n");
    }
}
/* Generate DISM code for the given method or main block. 
If classNumber < 0 then methodNumber may be anything and we assume we are generating code for the program's main block*/
void genBody(int ClassNumber, int MethodNumber) {
    addCode("#CM%d%d: mov 0 0\n", ClassNumber, MethodNumber);

    genPrologue(ClassNumber, MethodNumber);
    codeGenExprs(classesST[ClassNumber].methodList[MethodNumber].bodyExprs, ClassNumber, MethodNumber);
    genEpilogue(ClassNumber, MethodNumber);
}
/* Map a given (1) static class number, 2 a method number defined in that class,
and (3) a dynamic object's type to: (a) the dynamic class number and (b)
the dynamic method number that actually gets called when an object of type (3) dynamically invokes method(2).
This method assumes that dynamicType is a subtype of staticClass*/
void getDynamicMethod(int staticClass, int staticMethod, int dynamicType,
    int *dynamicClassToCall, int *dynamicMethodToCall) {
        char *targetMethodName = classesST[staticClass].methodList[staticMethod].methodName;

        int currentClass = dynamicType;

        while (currentClass > 0) {
            for (int i = 0; i < classesST[currentClass].numMethods ; i++) {
                if (strcmp(classesST[currentClass].methodList[i].methodName, targetMethodName) == 0) {
                    *dynamicClassToCall = currentClass;
                    *dynamicMethodToCall = i;
                    return;
                }
            } 
    }
}

/*Emit code for the program's vtable, beginning at label #VTABLE.
The vtable jumps (i.e., dispatches) to code based on
(1) the dynamic calling object's address (at M(SP+4)),
(2) the calling object's static type (at M(SP+3)), and
(3) the static method number (at M(SP+2))*/
void genVtable() {
    addCode("#VTABLE:\n");
    addCode("lod 1 6 4; load object address\n");
    checkNullDereference();
    addCode("lod 1 6 4; load object address again after null check\n");
    addCode("lod 2 6 3; load static type\n");
    addCode("lod 3 6 2; load static method number\n");
    int dynClass = 0, dynMethod = 0;
    //getDynamicMethod(0, 0, 0, &dynClass, &dynMethod);
    int tableLabel = labelNumber++;
    for(int dynType = 0; dynType < numClasses; dynType++) {
        addCode("mov 4 %d; load class number\n", dynType);
        addCode("beq 1 4 #continueType%d; branch to class\n", dynType);
        addCode("jmp 0 #skipType%d; branch to next class\n", dynType);
        addCode("#continueType%d: mov 0 0\n", dynType);
        for(int staticClass = 0; staticClass < numClasses; staticClass++) {
            addCode("mov 4 %d; load class number\n", staticClass);
            addCode("beq 2 4 #continueClass%d%d; branch to class\n", dynType, staticClass);
            addCode("jmp 0 #skipClass%d%d\n", dynType, staticClass);
            addCode("#continueClass%d%d: mov 0 0\n", dynType, staticClass);
            for(int staticMethod = 0; staticMethod < classesST[staticClass].numMethods; staticMethod++) {
                addCode("mov 4 %d; load method number\n", staticMethod);
                addCode("beq 3 4 #continueMethod%d%d%d; branch to method\n", dynType, staticClass, staticMethod);
                addCode("jmp 0 #skipMethod%d%d%d\n", dynType, staticClass, staticMethod);
                addCode("#continueMethod%d%d%d: mov 0 0\n", dynType, staticClass, staticMethod);
                int dynTargetClass, dynTargetMethod;
                getDynamicMethod(staticClass, staticMethod, dynType, &dynTargetClass, &dynTargetMethod);
                addCode("jmp 0 #CM%d%d ; go to resolved method\n", dynTargetClass, dynTargetMethod);
                addCode("#skipMethod%d%d%d: mov 0 0\n", dynType, staticClass, staticMethod);
            }
            addCode("#skipClass%d%d: mov 0 0\n", dynType, staticClass);
        }
        addCode("#skipType%d: mov 0 0\n", dynType);
    }
    addCode("hlt 0; no matching method\n");
}

void generateDISM(FILE *outputFile){
    // add all null dereference checks good20-22.dj
    // make sure can handle disjunction operator good6.dj
    fout = outputFile;
    genPrologue(-1, -1);
    codeGenExprs(mainExprs, -1, -1); 
    genEpilogue(-1, -1);
    for (int i = 0; i < numClasses; i++) {
        for (int j = 0; j < classesST[i].numMethods; j++) {
            genBody(i, j);
        }
    }
    if (needVtable) {
        genVtable();
    }
}