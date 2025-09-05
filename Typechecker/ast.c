/* File ast.c
   Implementation of DJ ASTs
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ast.h"

void printError(char *reason) {
  printf("AST Error: %s\n", reason);
  exit(-1);
}

/* Create a new AST node of type t */
ASTree *newAST(ASTNodeType t, ASTree *child, unsigned int natAttribute, 
  char *idAttribute, unsigned int lineNum) {
  ASTree *toReturn = malloc(sizeof(ASTree));
  if(toReturn == NULL) printError("malloc in newAST()");
  toReturn->typ = t;
  // create a linked list of children
  ASTList *childList = malloc(sizeof(ASTList));
  if (childList == NULL) {
      printf("AST Error: malloc failed in newAST()\n");
      exit(-1);
  }
  childList->data = child;
  childList->next = NULL;

  toReturn->children = childList;
  toReturn->childrenTail = childList;

  // Set node attributes
  if (t == NAT_LITERAL_EXPR) {
      toReturn->natVal = natAttribute;
      toReturn->idVal = NULL;
  } else if (t == AST_ID) {
      if (idAttribute != NULL) {
          toReturn->idVal = malloc(strlen(idAttribute) + 1);
          if (toReturn->idVal == NULL) {
              printf("AST Error: malloc failed in newAST()\n");
              exit(-1);
          }
          strcpy(toReturn->idVal, idAttribute);
      } else {
          toReturn->idVal = NULL;
      }
      toReturn->natVal = 0;
  } else {
      toReturn->natVal = 0;
      toReturn->idVal = NULL;
  }

  // Initialize static type-checking attributes
  toReturn->staticClassNum = 0;
  toReturn->staticMemberNum = 0;

  // Store line number
  toReturn->lineNumber = lineNum;
  return toReturn;
}

/* Append a new child AST node onto a parent's list of children */
void appendToChildrenList(ASTree *parent, ASTree *newChild) {
    if (parent == NULL) printError("append called with null parent");
    if (parent->children == NULL || parent->childrenTail == NULL)
      printError("append called with bad parent");
    if (newChild == NULL) printError("append called with null newChild");

    if (parent->childrenTail->data == NULL) {  // Replace empty tail with new child
      parent->childrenTail->data = newChild;
    } else {  // Append new child to list
      ASTList *newList = malloc(sizeof(ASTList));
      if (newList == NULL) printError("malloc in appendToChildrenList()");
      newList->data = newChild;
      newList->next = NULL;
      parent->childrenTail->next = newList;
      parent->childrenTail = newList;
    }
}

/* Print the type of this node and any node attributes */
void printNodeTypeAndAttribute(ASTree *t) {
  if (t == NULL) return;
  
  switch (t->typ) {
    case PROGRAM: printf("PROGRAM"); break;
    case CLASS_DECL_LIST: printf("CLASS_DECL_LIST"); break;
    case FINAL_CLASS_DECL: printf("FINAL_CLASS_DECL"); break;
    case NONFINAL_CLASS_DECL: printf("NONFINAL_CLASS_DECL"); break;
    case VAR_DECL_LIST: printf("VAR_DECL_LIST"); break;
    case VAR_DECL: printf("VAR_DECL"); break;
    case METHOD_DECL_LIST: printf("METHOD_DECL_LIST"); break;
    case FINAL_METHOD_DECL: printf("FINAL_METHOD_DECL"); break;
    case NONFINAL_METHOD_DECL: printf("NONFINAL_METHOD_DECL"); break;

    /* types, including generic IDs */
    case NAT_TYPE: printf("NAT_TYPE"); break;
    case AST_ID: printf("AST_ID(%s)", t->idAttribute); break;

    /* expression-lists */
    case EXPR_LIST: printf("EXPR_LIST"); break;

    /* expressions */
    case DOT_METHOD_CALL_EXPR: printf("DOT_METHOD_CALL_EXPR"); break;
    case METHOD_CALL_EXPR: printf("METHOD_CALL_EXPR"); break;
    case DOT_ID_EXPR: printf("DOT_ID_EXPR"); break;
    case ID_EXPR: printf("ID_EXPR"); break;
    case DOT_ASSIGN_EXPR: printf("DOT_ASSIGN_EXPR"); break;
    case ASSIGN_EXPR: printf("ASSIGN_EXPR"); break;
    case PLUS_EXPR: printf("PLUS_EXPR"); break;
    case MINUS_EXPR: printf("MINUS_EXPR"); break;
    case TIMES_EXPR: printf("TIMES_EXPR"); break;
    case EQUALITY_EXPR: printf("EQUALITY_EXPR"); break;
    case LESS_THAN_EXPR: printf("LESS_THAN_EXPR"); break;
    case NOT_EXPR: printf("NOT_EXPR"); break;
    case OR_EXPR: printf("OR_EXPR"); break;
    case ASSERT_EXPR: printf("ASSERT_EXPR"); break;
    case IF_THEN_ELSE_EXPR: printf("IF_THEN_ELSE_EXPR"); break;
    case WHILE_EXPR: printf("WHILE_EXPR"); break;
    case PRINT_EXPR: printf("PRINT_EXPR"); break;
    case READ_EXPR: printf("READ_EXPR"); break;
    case THIS_EXPR: printf("THIS_EXPR"); break;
    case NEW_EXPR: printf("NEW_EXPR"); break;
    case NULL_EXPR: printf("NULL_EXPR"); break;
    case NAT_LITERAL_EXPR: printf("NAT_LITERAL_EXPR(%u)", t->natAttribute); break;

    default:
      printError("unknown node type in printNodeTypeAndAttribute()");
  }
}

/* Print tree in preorder */
void printASTree(ASTree *t, int depth) {
  if (t == NULL) return;
  printf("%d:", depth);
  for (int i = 0; i < depth; i++) printf("  ");
  printNodeTypeAndAttribute(t);
  printf("\n");
  
  ASTList *childListIterator = t->children;
  while (childListIterator != NULL) {
    printASTree(childListIterator->data, depth + 1);
    childListIterator = childListIterator->next;
  }
}

/* Print the AST to stdout with indentations marking tree depth. */
void printAST(ASTree *t) { printASTree(t, 0); }
