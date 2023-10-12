#include <stdio.h>
#include <stdlib.h>
#include "json_c.c"


int functionCnt;
int ifCnt, elseIfCnt, elseCnt;
char *temp, *json_string;

void addType(json_value root, char *function, const char *tailer);
void getFunction(json_value ext, char *function);
void ifCalc(json_value root);
void loopCalc(json_value root);
void switchCalc(json_value root);
void checkType(json_value item);

int main(void) {
    char fileName[100];
    printf("파일 이름 입력 : ");
    gets(fileName);
    FILE *fp = fopen(fileName, "r");

    if (!fp) {
        printf("존재하지 않는 파일입니다.");
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    temp = (char *)malloc(size+1);
    memset(temp, 0, size+1);
    json_string = (char *)malloc(size/2+1);
    memset(json_string, 0, size/2+1);

    fread(temp, size, 1, fp);
    for(int i=0;i<size/2;i++) {
        json_string[i] = temp[i*2];
    }

    json_value json = json_create(json_string);
    json_value exts = json_get(json, "ext");
    int extLen = json_len(exts);

    for(int i=0;i<extLen;i++) {
        ifCnt=0; elseIfCnt=0; elseCnt=0;
        json_value ext = json_get(exts, i);
        json_value extType = json_get(ext, "_nodetype");

        if (!strcmp(extType.value, "FuncDef")) {
            printf("\n-------------------------함수 #%02d-------------------------\n", ++functionCnt);
            char *function = malloc(200);
            memset(function, 0, 200);
            getFunction(ext, function);
            printf("%s\n", function);
            free(function);

            json_value body = json_get(ext, "body");
            json_value block_items = json_get(body, "block_items");

            if (!(block_items.value)) {
                printf("함수 본문 없음\n");
                continue;
            }

            for(int j=0;j<json_len(block_items);j++) {
                json_value item = json_get(block_items, j);
                checkType(item);
            }
            printf("\nif 개수 : %d개\n", ifCnt);
            printf("else if 개수 : %d개\n", elseIfCnt);
            printf("else 개수 : %d개\n", elseCnt);
        }
        
    }
    printf("\n----------------------------------------------------------\n");
    printf("총 함수 갯수 : %d", functionCnt);
    fclose(fp);
}

void addType(json_value root, char *function, const char *tailer) {
    if (!strcmp(json_get(root, "_nodetype").value, "IdentifierType")) {
        json_value retTypes = json_get(root, "names");
        for(int k=0;k<json_len(retTypes);k++) {
            json_value retType = json_get(retTypes, k);
            strcat(function, retType.value);
            if (k!=json_len(retTypes)-1) strcat(function," ");
        }
    }
    else if (!strcmp(json_get(root, "_nodetype").value, "Struct")) {
        json_value retType = json_get(root, "name");
        strcat(function, "struct ");
        strcat(function, retType.value);
    }
    else if (!strcmp(json_get(root, "_nodetype").value, "Union")) {
        json_value retType = json_get(root, "name");
        strcat(function, "union ");
        strcat(function, retType.value);
    }
    else if (!strcmp(json_get(root, "_nodetype").value, "Enum")) {
        json_value retType = json_get(root, "name");
        strcat(function, "enum ");
        strcat(function, retType.value);
    }
    strcat(function, tailer);
}

void getFunction(json_value ext, char *function) {
    json_value decl = json_get(ext, "decl");
    json_value name = json_get(decl, "name");
    json_value type1 = json_get(decl, "type");
    json_value type2 = json_get(type1, "type");
    if (!strcmp(json_get(type2, "_nodetype").value, "TypeDecl")) {
        json_value type3 = json_get(type2, "type");
        addType(type3, function, " ");
    }
    else if (!strcmp(json_get(type2, "_nodetype").value, "PtrDecl")) {
        int ptrCnt = 1;
        json_value type3 = json_get(type2, "type");
        while (1) {
            if (strcmp(json_get(type3, "_nodetype").value, "PtrDecl")) break;
            type3 = json_get(type3, "type");
            ptrCnt++;
        }
        char star[10] = " ";
        while (ptrCnt--) {
            strcat(star, "*");
        }
        type3 = json_get(type3, "type");
        addType(type3, function, star);
    }
    strcat(function, name.value);
    strcat(function, "(");
    json_value args = json_get(type1, "args");
    if (!(args.value)) strcat(function, "void");
    else {
        json_value params = json_get(args, "params");
        for(int j=0;j<json_len(params);j++) {
            json_value param = json_get(params, j);
            json_value paramName = json_get(param, "name");
            if (!(paramName.value)) {
                strcat(function, "void");
                break;
            }
            json_value paramType1 = json_get(param, "type");
            if (!strcmp(json_get(paramType1, "_nodetype").value, "TypeDecl")) {
                json_value paramType2 = json_get(paramType1, "type");
                addType(paramType2, function, " ");
            }
            else if (!strcmp(json_get(paramType1, "_nodetype").value, "PtrDecl")) {
                int ptrCnt = 1;
                json_value paramType2 = json_get(paramType1, "type");
                while (1) {
                    if (strcmp(json_get(paramType2, "_nodetype").value, "PtrDecl")) break;
                    paramType2 = json_get(paramType2, "type");
                    ptrCnt++;
                }
                char star[10] = " ";
                while (ptrCnt--) {
                    strcat(star, "*");
                }
                paramType2 = json_get(paramType2, "type");
                addType(paramType2, function, star);
            }
            else if (!strcmp(json_get(paramType1, "_nodetype").value, "ArrayDecl")){
                int arrayCnt = 1;
                json_value paramType2 = json_get(paramType1, "type");
                while (1) {
                    if (strcmp(json_get(paramType2, "_nodetype").value, "ArrayDecl")) break;
                    paramType2 = json_get(paramType2, "type");
                    arrayCnt++;
                }
                if (!strcmp(json_get(paramType2, "_nodetype").value, "TypeDecl")) {
                    json_value paramType3 = json_get(paramType2, "type");
                    addType(paramType3, function, " ");
                }
                else if (!strcmp(json_get(paramType2, "_nodetype").value, "PtrDecl")) {
                    int ptrCnt = 1;
                    json_value paramType3 = json_get(paramType2, "type");
                    while (1) {
                        if (strcmp(json_get(paramType3, "_nodetype").value, "PtrDecl")) break;
                        paramType3 = json_get(paramType3, "type");
                        ptrCnt++;
                    }
                    char star[10] = " ";
                    while (ptrCnt--) {
                        strcat(star, "*");
                    }
                    paramType3 = json_get(paramType3, "type");
                    addType(paramType3, function, star);
                }
                strcat(function, paramName.value);
                json_value type = json_get(param, "type");
                while (arrayCnt--) {
                    json_value dim = json_get(type, "dim");
                    strcat(function, "[");
                    if (dim.value) {
                        json_value elementCnt = json_get(dim, "value");
                        strcat(function, elementCnt.value);
                    } 
                    strcat(function, "]");
                    type = json_get(type, "type");
                 }
                if (j!=json_len(params)-1) strcat(function, ", ");
                continue;
            }
            strcat(function, paramName.value);
            if (j!=json_len(params)-1) strcat(function, ", ");
        }
    }
    strcat(function, ")");
}

void ifCalc(json_value root) {
    json_value iftrue = json_get(root, "iftrue");
    if (iftrue.value) {
        if (!strcmp(json_get(iftrue, "_nodetype").value, "Compound")) {
            json_value items = json_get(iftrue, "block_items");
            if (items.value) {
                for(int i=0;i<json_len(items);i++) {
                    json_value item = json_get(items, i);
                    checkType(item);
                }
            }
        }
        else {
            checkType(iftrue);
        }
    }
    json_value iffalse = json_get(root, "iffalse");
    if (iffalse.value) {
        json_value type = json_get(iffalse, "_nodetype");
        if (!strcmp(type.value, "Compound")) {
            elseCnt++;
        }
        else if (!strcmp(type.value, "If")) {
            elseIfCnt++;
            ifCalc(iffalse);
            return;
        }
        else {
            elseCnt++;
            checkType(iffalse);
            return;
        }
        json_value items = json_get(iffalse, "block_items");
        if (items.value) {
            
            for(int i=0;i<json_len(items);i++) {
                json_value item = json_get(items, i);
                json_value type = json_get(item, "_nodetype");
                checkType(item);
            }
        }
    }
}

void loopCalc(json_value root) {
    json_value stmt = json_get(root, "stmt");
    json_value loopType = json_get(stmt, "_nodetype");
    if (!strcmp(loopType.value, "EmptyStatement")) return;
    else if (!strcmp(loopType.value, "Compound")) {
        json_value items = json_get(stmt, "block_items");
        if (!(items.value)) return;
        for(int i=0;i<json_len(items);i++) {
            json_value item = json_get(items, i);
            checkType(item);
        }
    }
    else checkType(stmt);
}

void switchCalc(json_value root) {
    json_value stmt = json_get(root, "stmt");
    json_value items = json_get(stmt, "block_items");
    if (items.value) {
        for(int i=0;i<json_len(items);i++) {
            json_value item = json_get(items, i);
            json_value itemType = json_get(item, "_nodetype");
            if (!strcmp(itemType.value, "Case")) {
                json_value stmts = json_get(item, "stmts");
                if (stmts.value) {
                    for(int j=0;j<json_len(stmts);j++) {
                        json_value stmtItem = json_get(stmts, j);
                        checkType(stmtItem);
                    }
                }
            }
        }
    }
}

void checkType(json_value item) {
    json_value itemType = json_get(item, "_nodetype");
    if (!strcmp(itemType.value, "If")) {
        ifCnt++;
        ifCalc(item);
    }
    else if (!strcmp(itemType.value, "While") || !strcmp(itemType.value, "For") || !strcmp(itemType.value, "DoWhile")) {
        loopCalc(item);
    }
    else if (!strcmp(itemType.value, "Switch")) {
        switchCalc(item);
    }
}