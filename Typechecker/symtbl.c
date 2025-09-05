#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "symtbl.h"

ASTree *wholeProgram = NULL;
ASTree *mainExprs = NULL;
int numMainBlockLocals = 0;
VarDecl *mainBlockST = NULL;
int numClasses = 0;
ClassDecl *classesST = NULL;

/* Function to return the number of children for a given AST node */
int countChildren(ASTree *tree) {
    if (!tree || !tree->children) return 0;

    int count = 0;
    ASTList *curr = tree->children;
    while (curr) {
        count++;
        curr = curr->next;
        //printf("count: %d\n", count);
    }
    return count;
}

int classNameToNumber(char *className) {
    if (!className) return -3;

    if (strcmp(className, "Object") == 0) return 0;

    for (int i = 1; i < numClasses; i++) {
        if (strcmp(classesST[i].className, className) == 0) {
            return i;
        }
    }

    return -3;
}

void setupSymbolTables(ASTree *fullProgramAST) {
    //printf("Setting up symbol tables... ");
    if (!fullProgramAST || countChildren(fullProgramAST) < 3) {
        fprintf(stderr, "Malformed AST: not enough children.\n");
        exit(1);
    }
    //printf(" in the code\n");
    
    wholeProgram = fullProgramAST;
    ASTree *classList = fullProgramAST->children->data; // First child
    ASTree *mainVarDecls = fullProgramAST->children->next->data; // Second child
    mainExprs = fullProgramAST->children->next->next->data; // Third child
    

    // Set up main block locals
    numMainBlockLocals = countChildren(mainVarDecls);
    mainBlockST = malloc(sizeof(VarDecl) * numMainBlockLocals);
    if (!mainBlockST) {
        fprintf(stderr, "Memory allocation failed for mainBlockST.\n");
        exit(1);
    }
    memset(mainBlockST, 0, sizeof(VarDecl) * numMainBlockLocals);

    for (int i = 0; i < numMainBlockLocals; i++) {
        ASTree *vdecl = mainVarDecls->children->data; // First child of the mainVarDecls
        if (!vdecl || countChildren(vdecl) < 2) continue;

        ASTree *idNode = vdecl->children->data;
        ASTree *typeNode = vdecl->children->next->data;

        mainBlockST[i].varName = idNode ? idNode->idVal : NULL;
        mainBlockST[i].varNameLineNumber = idNode ? idNode->lineNumber : -1;
        mainBlockST[i].type = typeNode && typeNode->idVal ? classNameToNumber(typeNode->idVal) : -3;
        mainBlockST[i].typeLineNumber = typeNode ? typeNode->lineNumber : -1;
    }

    // Safely determine number of classes
    int userClassCount = classList ? countChildren(classList) : 0;
    numClasses = userClassCount;
    classesST = malloc(sizeof(ClassDecl) * (numClasses +1));
    if (!classesST) {
        fprintf(stderr, "Memory allocation failed for classesST.\n");
        exit(1);
    }
    memset(classesST, 0, sizeof(ClassDecl) * numClasses);

    // Add Object class
    classesST[0].className = "Object";
    classesST[0].classNameLineNumber = 0;
    classesST[0].superclass = -3;
    classesST[0].superclassLineNumber = 0;
    classesST[0].isFinal = 0;
    classesST[0].numVars = 0;
    classesST[0].varList = NULL;
    classesST[0].numMethods = 0;
    classesST[0].methodList = NULL;

    // Add user-defined classes if any
    if (userClassCount > 0 && classList && classList->children->data) {
        for (int i = 0; i < userClassCount; i++) {
            
            ASTree *classAST = classList->children->data;
            ClassDecl *classDecl = &classesST[i + 1];
            
            ASTree *idNode = classAST->children->data;
            
            ASTree *superclassNode = classAST->children->next->data;
            
            ASTree *fieldList = classAST->children->next->next->data;
            

            classDecl->className = idNode ? idNode->idVal : NULL;
            classDecl->classNameLineNumber = idNode ? idNode->lineNumber : -1;
            classDecl->superclass = (superclassNode && superclassNode->idVal) ? classNameToNumber(superclassNode->idVal) : -3;
            classDecl->superclassLineNumber = superclassNode ? superclassNode->lineNumber : -1;
            classDecl->isFinal = (classAST->typ == FINAL_CLASS_DECL);

            // Variable fields
            ASTree *varsNode = fieldList->children->data;
            classDecl->numVars = varsNode ? countChildren(varsNode) : 0;
            classDecl->varList = malloc(sizeof(VarDecl) * classDecl->numVars);
            if (!classDecl->varList && classDecl->numVars > 0) {
                fprintf(stderr, "Memory allocation failed for varList of class %s.\n", classDecl->className);
                exit(1);
            }
            memset(classDecl->varList, 0, sizeof(VarDecl) * classDecl->numVars);

            for (int j = 0; j < classDecl->numVars; j++) {
                ASTree *vdecl = varsNode->children->data;
                ASTree *idNode = vdecl->children->data;
                ASTree *typeNode = vdecl->children->next->data;

                classDecl->varList[j].varName = idNode ? idNode->idVal : NULL;
                classDecl->varList[j].varNameLineNumber = idNode ? idNode->lineNumber : -1;
                classDecl->varList[j].type = typeNode && typeNode->idVal ? classNameToNumber(typeNode->idVal) : -3;
                classDecl->varList[j].typeLineNumber = typeNode ? typeNode->lineNumber : -1;
            }

            // Methods
            //printf("methods started\n");
            ASTList *methodListNode = fieldList->children ? fieldList->children->next : NULL;
            ASTree *methodsNode = methodListNode ? methodListNode->data : NULL;

            classDecl->numMethods = methodsNode ? countChildren(methodsNode) : 0;
            classDecl->methodList = malloc(sizeof(MethodDecl) * classDecl->numMethods);
            
            if (classDecl->numMethods > 0) {
                if (!classDecl->methodList) {
                    fprintf(stderr, "Memory allocation failed for methodList of class %s.\n", classDecl->className);
                    exit(1);
                }
                memset(classDecl->methodList, 0, sizeof(MethodDecl) * classDecl->numMethods);

                for (int j = 0; j < classDecl->numMethods; j++) {
                    ASTree *methodDeclNode = methodsNode->children->data;
                    MethodDecl *method = &classDecl->methodList[j];

                    method->methodName = methodDeclNode && methodDeclNode->children ? methodDeclNode->children->data->idVal : NULL;
                    method->methodNameLineNumber = methodDeclNode && methodDeclNode->children ? methodDeclNode->children->data->lineNumber : -1;

                    ASTree *retTypeNode = methodDeclNode && methodDeclNode->children->next ? methodDeclNode->children->next->data : NULL;
                    method->returnType = retTypeNode && retTypeNode->idVal ? classNameToNumber(retTypeNode->idVal) : -3;
                    method->returnTypeLineNumber = retTypeNode ? retTypeNode->lineNumber : -1;

                    ASTree *paramDeclNode = methodDeclNode && methodDeclNode->children->next && methodDeclNode->children->next->next ?
                                            methodDeclNode->children->next->next->data : NULL;
                    ASTree *paramIdNode = paramDeclNode && paramDeclNode->children ? paramDeclNode->children->data : NULL;
                    ASTree *paramTypeNode = paramDeclNode && paramDeclNode->children && paramDeclNode->children->next ?
                                            paramDeclNode->children->next->data : NULL;

                    method->paramName = paramIdNode ? paramIdNode->idVal : NULL;
                    method->paramNameLineNumber = paramIdNode ? paramIdNode->lineNumber : -1;
                    method->paramType = paramTypeNode && paramTypeNode->idVal ? classNameToNumber(paramTypeNode->idVal) : -3;
                    method->paramTypeLineNumber = paramTypeNode ? paramTypeNode->lineNumber : -1;
                    method->isFinal = (methodDeclNode->typ == FINAL_METHOD_DECL);

                    ASTree *localsNode = methodDeclNode && methodDeclNode->children->next && methodDeclNode->children->next->next ?
                                         methodDeclNode->children->next->next->next->data : NULL;
                    method->numLocals = localsNode ? countChildren(localsNode) : 0;
                    method->localST = malloc(sizeof(VarDecl) * method->numLocals);
                    if (method->numLocals > 0 && !method->localST) {
                        fprintf(stderr, "Memory allocation failed for locals in method %s of class %s.\n", method->methodName, classDecl->className);
                        exit(1);
                    }
                    memset(method->localST, 0, sizeof(VarDecl) * method->numLocals);

                    for (int k = 0; k < method->numLocals; k++) {
                        ASTree *vdecl = localsNode && localsNode->children ? localsNode->children->data : NULL;
                        ASTree *idNode = vdecl && vdecl->children ? vdecl->children->data : NULL;
                        ASTree *typeNode = vdecl && vdecl->children && vdecl->children->next ? vdecl->children->next->data : NULL;

                        method->localST[k].varName = idNode ? idNode->idVal : NULL;
                        method->localST[k].varNameLineNumber = idNode ? idNode->lineNumber : -1;
                        method->localST[k].type = typeNode && typeNode->idVal ? classNameToNumber(typeNode->idVal) : -3;
                        method->localST[k].typeLineNumber = typeNode ? typeNode->lineNumber : -1;
                    }

                    method->bodyExprs = methodDeclNode && methodDeclNode->children->next && methodDeclNode->children->next->next ?
                                        methodDeclNode->children->next->next->next->data : NULL;
                }
            }
        }
    }

   // printf("Finished setting up symbol tables.\n");
}
