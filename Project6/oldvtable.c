void genVtable() {
    addCode("#VTABLE:\n");
    addCode("lod 1 6 4; load object address\n");
    

    for (int i = 0; i < numClasses; i++) {
        int tableLabel = labelNumber++;
        // if match is found then branch to correct class
        addCode("mov 2 %d; load class number\n", i);
        addCode("beq 1 2 #VTABLE%d; branch to class\n", tableLabel);
    }
    addCode("hlt 0; no matching class\n");

    for (int i = 0; i < numClasses; i++) {
        int tableLabel = labelNumber++;
        addCode("#VTABLE%d: mov 0 0\n", tableLabel);
        
        addCode("lod 3 6 3; load static type\n");
        addCode("lod 4 6 2; load method number\n");

        for (int j = 0; j < numClasses; j++) {
            for (int k = 0; k < classesST[j].numMethods; k++) {
                int dynTargetClass, dynTargetMethod;
                getDynamicMethod(i, j, classesST[j].methodList[k].returnType, &dynTargetClass, &dynTargetMethod);

                // addCode("mov 1 %d; load class number\n", j);
                // addCode("bne 1 3 #skip%d%d; branch to class\n", j, k);
                // addCode("mov 2 %d; load method number\n", k);
                // addCode("bne 2 4 #skip%d%d; branch to method\n", j, k);

                // addCode("jmp 0 #C%dM%d\n", dynTargetClass, dynTargetMethod);
                // addCode("#skip%d%d: mov 0 0\n", j, k);
                addCode("mov 1 %d; load class number\n", j);
                addCode("beq 1 3 #continue%d%d; branch to class\n", j, k);
                addCode("jmp 0 #skip%d%d; branch to next class\n", j, k);
                addCode("#continue%d%d: mov 0 0\n", j, k);
                addCode("mov 2 %d; load method number\n", j, k, k);
                addCode("beq 2 4 #continueMethod%d%d; branch to method\n", j, k);
                addCode("jmp 0 #skip%d%d; branch to next method\n", j, k);

                addCode("#continueMethod%d%d: mov 0 0\n", j, k);
                addCode("jmp 0 #C%dM%d\n", dynTargetClass, dynTargetMethod);
                addCode("#skip%d%d: mov 0 0\n", j, k);
            }
    }
    addCode("hlt 0; no matching method\n");
    }
}