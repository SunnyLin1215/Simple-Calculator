#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

///lec.h
#define MAXLEN 256
#define TBLSIZE 64
// Token types
typedef enum {
    UNKNOWN, END, ENDFILE,
    INT, ID,
    ADDSUB, MULDIV,
    ASSIGN,
    LPAREN, RPAREN,
    INCDEC,
    AND, OR, XOR
} TokenSet;

int match(TokenSet token);
void advance(void);
char *getLexeme(void);

// Call this macro to print error message and exit the program
// This will also print where you called it in your program

///parser.h
// Error types
typedef enum {
    UNDEFINED, MISPAREN, NOTNUMID, NOTFOUND, RUNOUT, NOTLVAL, DIVZERO, SYNTAXERR
} ErrorType;

// Structure of the symbol table
typedef struct {
    int val;
    char name[MAXLEN];
} Symbol;

// Structure of a tree node
typedef struct _Node {
    TokenSet data;
    int val;
    int haveID;
    int oper_count;
    char lexeme[MAXLEN];
    struct _Node *left;
    struct _Node *right;
} BTNode;


Symbol table[TBLSIZE];
void initTable(void);
int getval(char *str);
int setval(char *str, int val);

BTNode *makeNode(TokenSet tok, const char *lexe);
void freeTree(BTNode *root);

BTNode *factor(void);
BTNode* unary_expr(void);
BTNode* muldiv_expr(void);
BTNode* muldiv_expr_tail(BTNode* left);
BTNode* addsub_expr(void);
BTNode* addsub_expr_tail(BTNode* left);
BTNode* and_expr(void);
BTNode* and_expr_tail(BTNode* left);
BTNode* xor_expr(void);
BTNode* xor_expr_tail(BTNode* left);
BTNode* or_expr(void);
BTNode* or_expr_tail(BTNode* left);
BTNode* assign_expr(void);
void statement(void);

// Print error message and exit the program
void err(ErrorType errorNum);

int sbcount;
int left_id[100];
int leftIDcount;

///evaluate.h
int rFlag[100];
int head;
int evaluateTree(BTNode *root);

// Print the syntax tree in prefix
//extern void printPrefix(BTNode *root);
void print(char* s, int left, int right);
int cal_int(char* s );
int find_r_idx(void);
void change_val_rFlag(int i);


///main.c
int main() {
    //freopen("input.txt", "w", stdout);
    initTable();
    while (1) {
        statement();
    }
    return 0;
}

///lex.c

TokenSet getToken(void);
TokenSet curToken = UNKNOWN;
char lexeme[MAXLEN];

TokenSet getToken(void)
{
    int i = 0;
    char c = '\0';

    while ((c = fgetc(stdin)) == ' ' || c == '\t');

    if (isdigit(c)) {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    } else if (c == '+' || c == '-') {
        char cc = fgetc(stdin);
        if(cc == c) {
            lexeme[0] = lexeme[1] = c;
            lexeme[2] = '\0';
            return INCDEC;
        }
        else{
            ungetc(cc, stdin);
            lexeme[0] = c;
            lexeme[1] = '\0';
            return ADDSUB;
        }
    } else if (c == '*' || c == '/') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    } else if (c == '\n') {
        lexeme[0] = '\0';
        return END;
    } else if (c == '=') {
        strcpy(lexeme, "=");
        return ASSIGN;
    } else if (c == '(') {
        strcpy(lexeme, "(");
        return LPAREN;
    } else if (c == ')') {
        strcpy(lexeme, ")");
        return RPAREN;
    } else if (isalpha(c)) {
        lexeme[0] = c;
        i = 1;
        char cc = fgetc(stdin);
        while(cc == '_' || isdigit(cc) || isupper(cc) || islower(cc)) {
            lexeme[i] = cc;
            i++;
            cc = fgetc(stdin);
        }
        ungetc(cc, stdin);
        lexeme[i] = '\0';
        return ID;
    }else if( c == '&') {
        strcpy(lexeme, "&");
        return AND;
    }else if( c == '|') {
        strcpy(lexeme, "|");
        return OR;
    }else if( c == '^') {
        strcpy(lexeme, "^");
        return XOR;
    } else if (c == EOF) {
        return ENDFILE;
    } else {
        return UNKNOWN;
    }
}

void advance(void) {
    curToken = getToken();
}

int match(TokenSet token) {
    if (curToken == UNKNOWN)
        advance();
    return token == curToken;
}

char *getLexeme(void) {
    return lexeme;
}

///parser.c


void initTable(void) {
    strcpy(table[0].name, "x");
    table[0].val = 0;
    strcpy(table[1].name, "y");
    table[1].val = 0;
    strcpy(table[2].name, "z");
    table[2].val = 0;
    sbcount = 3;
}

int getval(char *str) {
    int i = 0;

    for (i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0)
            return i;
    }
    err(SYNTAXERR);
    return -20;      //沒有意義
}
int check(int idx) {
    for( int i = 0; i < leftIDcount; i++) {
        if(left_id[i] == idx) return 1;
    }
    return 0;
}
BTNode *makeNode(TokenSet tok, const char *lexe) {
    BTNode *node = (BTNode*)malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->data = tok;
    node->val = 0;
    node->haveID = 0;
    node->oper_count = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void freeTree(BTNode *root) {
    if (root != NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

//factor :=  INT | ID | INCDEC ID | LPAREN assign_expr RPAREN
BTNode *factor(void) {
    BTNode *retp = NULL;

    if (match(INT)) {
        retp = makeNode(INT, getLexeme());
        advance();
    } else if (match(ID)) {
        retp = makeNode(ID, getLexeme());
        retp->haveID = 1;
        advance();
        if( !match(ASSIGN)) { //等號右側
            int flag = 0;
            for( int i = 0; i < sbcount && !flag; i++) {
                if(strcmp(retp->lexeme, table[i].name) == 0) {
                    flag = 1;
                    int left_exist = check(i);
                    if(left_exist) err(SYNTAXERR);
                    break;
                }
            }
            if(!flag) {    //在等號右側但沒出現過
                err(NOTFOUND);
            }
        }//else err(SYNTAXERR);

        else {   //等號左側
            int flag = 0;
            for( int i = 0; i < sbcount && !flag; i++) {
                if(strcmp(retp->lexeme, table[i].name) == 0) {
                    flag = 1;
                    int left_exist = check(i);
                    if(left_exist) err(SYNTAXERR);
                    break;
                }
            }
            if( !flag) {
                left_id[leftIDcount] = sbcount;
                leftIDcount++;

                strcpy( table[sbcount].name, retp->lexeme);
                table[sbcount].val = 0;
                sbcount++;
                if(sbcount >= TBLSIZE) err(RUNOUT);
            }
        }
    } else if (match(INCDEC)) {
        retp = makeNode(INCDEC, getLexeme());
        advance();
        if( !match(ID)) err(NOTNUMID);
        else {
            retp->left = NULL;
            retp->right = factor();
            retp->haveID = 1;
            retp->oper_count = 1;
        }
    } else if (match(LPAREN)) {
        advance();
        retp = assign_expr();
        if (match(RPAREN))
            advance();
        else
            err(MISPAREN);
    } else {
        err(NOTNUMID);
    }
    return retp;
}

BTNode* unary_expr(void) {
    BTNode* node = NULL;
    int l, r;
    if(match(ADDSUB)) {
        node = makeNode( ADDSUB, getLexeme());
        advance();
        node->left = NULL;
        node->right = unary_expr();
        (node->left) ? (l = (node->left)->haveID) : (l = 0);
        (node->right) ? (r = (node->right)->haveID) : (r = 0);
        if(l || r) node->haveID = 1;

        (node->left) ? (l = (node->left)->oper_count) : (l = 0);
        (node->right) ? (r = (node->right)->oper_count) : (r = 0);
        node->oper_count = l + r;

    }else{
        node = factor();
    }
    return node;
}
BTNode* muldiv_expr(void) {
    BTNode* node = unary_expr();
    return muldiv_expr_tail(node);
}
//muldiv_expr_tail := MULDIV unary_expr muldiv_expr_tail | NiL
BTNode* muldiv_expr_tail(BTNode* left) {
    BTNode* node = NULL;
    int l, r;
    if(match(MULDIV)) {
        node = makeNode(MULDIV, getLexeme());
        advance();
        node->left = left;
        node->right = unary_expr();
        (node->left) ? (l = (node->left)->haveID) : (l = 0);
        (node->right) ? (r = (node->right)->haveID) : (r = 0);
        if(l || r) node->haveID = 1;

        (node->left) ? (l = (node->left)->oper_count) : (l = 0);
        (node->right) ? (r = (node->right)->oper_count) : (r = 0);
        node->oper_count = l + r + 1;
        return muldiv_expr_tail(node);
    } else {
        return left;
    }
}
//addsub_expr := muldiv_expr addsub_expr_tail
BTNode* addsub_expr(void) {
    BTNode* node = muldiv_expr();
    return addsub_expr_tail(node);
}
//addsub_expr_tail := ADDSUB muldiv_expr addsub_expr_tail | NiL
BTNode* addsub_expr_tail(BTNode* left){
    BTNode* node = NULL;
    int l, r;
    if(match(ADDSUB)) {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = left;
        node->right = muldiv_expr();
        (node->left) ? (l = (node->left)->haveID) : (l = 0);
        (node->right) ? (r = (node->right)->haveID) : (r = 0);
        if(l || r) node->haveID = 1;

        (node->left) ? (l = (node->left)->oper_count) : (l = 0);
        (node->right) ? (r = (node->right)->oper_count) : (r = 0);
        node->oper_count = l + r + 1;
        return addsub_expr_tail(node);
    } else {
        return left;
    }
}
//and_expr         := addsub_expr and_expr_tail | NiL
BTNode* and_expr(void) {
    BTNode* node = addsub_expr();
    return and_expr_tail(node);
}
//and_expr_tail    := AND addsub_expr and_expr_tail | NiL
BTNode* and_expr_tail(BTNode* left){
    BTNode* node = NULL;
    int l, r;
    if(match(AND)) {
        node = makeNode(AND, getLexeme());
        advance();
        node->left = left;
        node->right = addsub_expr();
        (node->left) ? (l = (node->left)->haveID) : (l = 0);
        (node->right) ? (r = (node->right)->haveID) : (r = 0);
        if(l || r) node->haveID = 1;

        (node->left) ? (l = (node->left)->oper_count) : (l = 0);
        (node->right) ? (r = (node->right)->oper_count) : (r = 0);
        node->oper_count = l + r + 1;
        return and_expr_tail(node);
    } else {
        return left;
    }
}
//xor_expr         := and_expr xor_expr_tail
BTNode* xor_expr(void) {
    BTNode* node = and_expr();
    return xor_expr_tail(node);
}
//xor_expr_tail    := XOR and_expr xor_expr_tail | NiL
BTNode* xor_expr_tail(BTNode* left){
    BTNode* node = NULL;
    int l, r;
    if(match(XOR)) {
        node = makeNode(XOR, getLexeme());
        advance();
        node->left = left;
        node->right = and_expr();
        (node->left) ? (l = (node->left)->haveID) : (l = 0);
        (node->right) ? (r = (node->right)->haveID) : (r = 0);
        if(l || r) node->haveID = 1;

        (node->left) ? (l = (node->left)->oper_count) : (l = 0);
        (node->right) ? (r = (node->right)->oper_count) : (r = 0);
        node->oper_count = l + r + 1;
        return xor_expr_tail(node);
    } else {
        return left;
    }
}
//or_expr          := xor_expr or_expr_tail
BTNode* or_expr(void) {
    BTNode* node = xor_expr();
    return or_expr_tail(node);
}
//or_expr_tail     := OR xor_expr or_expr_tail | NiL
BTNode* or_expr_tail(BTNode* left){
    BTNode* node = NULL;
    int l, r;
    if(match(OR)) {
        node = makeNode(OR, getLexeme());
        advance();
        node->left = left;
        node->right = xor_expr();
        (node->left) ? (l = (node->left)->haveID) : (l = 0);
        (node->right) ? (r = (node->right)->haveID) : (r = 0);
        if(l || r) node->haveID = 1;

        (node->left) ? (l = (node->left)->oper_count) : (l = 0);
        (node->right) ? (r = (node->right)->oper_count) : (r = 0);
        node->oper_count = l + r + 1;
        return or_expr_tail(node);
    } else {
        return left;
    }
}
//assign_expr      := ID ASSIGN assign_expr | or_expr
BTNode* assign_expr(void) {       ///5+1=6 會是syntax error?
    BTNode* left = NULL, *node = NULL;
    int l, r;
    if(match(ID)) {
        left = makeNode(ID, getLexeme());
        left->haveID = 1;
        int flag = 0;
        for( int i = 0; i < sbcount && !flag; i++) {
            if(strcmp(left->lexeme, table[i].name) == 0) {
                if(check(i)) err(SYNTAXERR);
                flag = 1;
                break;
            }
        }
        if(!flag) {      //沒出現過
            left_id[leftIDcount] = sbcount;
            leftIDcount++;

            strcpy( table[sbcount].name, left->lexeme);
            table[sbcount].val = 0;
            sbcount++;
            if(sbcount >= TBLSIZE) err(RUNOUT);
        }
        char c = fgetc(stdin);
        while(c == ' ' || c == '\t') {
            c = fgetc(stdin);
        }
        if(c == '=') {
            ungetc(c, stdin);
            advance();
            node = makeNode(ASSIGN, getLexeme());
            advance();
            node->left = left;
            node->right = assign_expr();
            (node->left) ? (l = (node->left)->haveID) : (l = 0);
            (node->right) ? (r = (node->right)->haveID) : (r = 0);
            if(l || r) node->haveID = 1;
            return node;
        }else{
            ungetc(c, stdin);
            node = or_expr();
            return node;
        }
    }else {
        node = or_expr();
        return node;
    }
}
/*
void print_operCount(BTNode* node) {
    if(node == NULL) return;
    print_operCount(node->left);
    print_operCount(node->right);

    printf("%s %d\n", node->lexeme, node->oper_count);
    return;
}
*/

// statement := ENDFILE | END | expr END
void statement(void) {
    BTNode *retp = NULL;

    if (match(ENDFILE)) {
        printf("MOV r0 [0]\nMOV r1 [4]\nMOV r2 [8]\nEXIT 0\n");
        exit(0);
    } else if (match(END)) {
        advance();
    } else {
        for(int i = 0; i < 20; i++) {
            left_id[i] = 0;
        }
        leftIDcount = 0;
        head = 1;
        retp = assign_expr();
        //print_operCount(retp);
        if (match(END)) {
            evaluateTree(retp);
            freeTree(retp);
            advance();
        } else {
            err(SYNTAXERR);
        }
    }
}

void err(ErrorType errorNum) {
    printf("EXIT 1\n");
    exit(0);
}
///codeGen.c
int Flag = 0;
void not_var(BTNode* node, char* s) {
    if(Flag) return;
    if(node) {
        if(strcmp(node->lexeme, s) == 0) {
            Flag = 1;
            return;
        }
        not_var(node->right, s);
        if(Flag) return;
        not_var(node->left, s);
    }
}
void not_INCDEC_var(BTNode* node, char* s) {
    if(Flag) return;
    if(node) {
        if(node->data == INCDEC && strcmp((node->right)->lexeme, s) == 0) {
            Flag = 1;
            return;
        }
        not_INCDEC_var(node->right, s);
        if(Flag) return;
        not_INCDEC_var(node->left, s);
    }
}
void find_left_var(BTNode* node, BTNode* right) {
    if(Flag) return;
    if(node) {
        if(node->data == ID) {
            not_INCDEC_var(right, node->lexeme);
            if(Flag) return;
        }else if(node->data == INCDEC) {
            not_var(right, (node->right)->lexeme);
            if(Flag) return;
        }
        find_left_var(node->left, right);
        if(Flag) return;
        find_left_var(node->right, right);
    }
}
/*
r0 -> -10    r1 -> -1     r2 -> -2
[0] -> 0     [4] -> 1     [8] -> 2     */
int evaluateTree(BTNode *root) {
    int retval, tmp, lv = 0, rv = 0, r_idx;     //都代表r的編號
    if(root) {
        BTNode* R = root->right;
        BTNode* L = root->left;
        if(root->data == ASSIGN) {           //=
            if(L->data == ID) {
                lv = getval(L->lexeme);
                rv = evaluateTree(R);
                L->val = R->val;
                table[lv].val = L->val;
                print("MOV", lv, rv);
                if(head){
                    head = 0;
                    change_val_rFlag(rv);
                }
                return rv;
            }else err(NOTNUMID);
        }else if(root->data == INT) {             // int
            root->val = cal_int(root->lexeme);
            r_idx = find_r_idx();
            printf("MOV r%d %d\n", r_idx, root->val);
            (r_idx == 0) ? (retval = -10) : (retval = -r_idx);
            change_val_rFlag(retval);
            return retval;
        }else if(root->data == ID) {
            tmp = getval(root->lexeme);     //tmp = table index
            root->val = table[tmp].val;
            r_idx = find_r_idx();
            printf("MOV r%d [%d]\n", r_idx, tmp*4);
            (r_idx == 0) ? (retval = -10) : (retval = -r_idx);
            change_val_rFlag(retval);
            return retval;
        }else if(root->data == ADDSUB) {         // + -
            if(L == NULL) {             //-x
                if( strcmp(root->lexeme, "-") == 0) {
                    rv = evaluateTree(R);
                    root->val = -(R->val);
                    r_idx = find_r_idx();
                    printf("MOV r%d %d\n", r_idx, 0);
                    (r_idx == 0) ? (retval = -10) : (retval = -r_idx);
                    change_val_rFlag(retval);
                    print("SUB", retval, rv);
                    change_val_rFlag(rv);
                    return retval;
                }else {
                    rv = evaluateTree(R);
                    root->val = R->val;
                    return rv;
                }
            }else {         //a+b
                if(R->oper_count > L->oper_count){
                    Flag = 0;
                    find_left_var(L, R);
                    if(!Flag) {       //Flag = 1   -> invalid
                        rv = evaluateTree(R);
                        lv = evaluateTree(L);     //多的先
                    }else {
                        lv = evaluateTree(L);
                        rv = evaluateTree(R);
                    }
                } else {
                    lv = evaluateTree(L);
                    rv = evaluateTree(R);
                }
                if( strcmp(root->lexeme, "-") == 0) {
                    root->val = L->val - R->val;
                    print("SUB", lv, rv);
                }else {
                    print("ADD", lv, rv);
                    root->val = L->val + R->val;
                }
                change_val_rFlag(rv);
                return lv;
            }
        }else if(root->data == MULDIV) {         // + -
            if(R->oper_count > L->oper_count){
                Flag = 0;
                find_left_var(L, R);
                if(!Flag) {
                    rv = evaluateTree(R);
                    lv = evaluateTree(L);     //多的先
                }else {
                    lv = evaluateTree(L);
                    rv = evaluateTree(R);
                }
            } else {
                lv = evaluateTree(L);
                rv = evaluateTree(R);
            }
            if( strcmp(root->lexeme, "*") == 0) {
                root->val = L->val * R->val;
                print("MUL", lv, rv);
            }else {
                if(R->haveID == 0 && R->val == 0) err(DIVZERO);
                if(R->val != 0) root->val = L->val / R->val;
                else root->val = 0;
                print("DIV", lv, rv);
            }
            change_val_rFlag(rv);
            return lv;
        }else if(root->data == AND) {         // + -
            if(R->oper_count > L->oper_count){
                Flag = 0;
                find_left_var(L, R);
                if(!Flag) {
                    rv = evaluateTree(R);
                    lv = evaluateTree(L);     //多的先
                }else {
                    lv = evaluateTree(L);
                    rv = evaluateTree(R);
                }
            } else {
                lv = evaluateTree(L);
                rv = evaluateTree(R);
            }
            root->val = L->val & R->val;
            print("AND", lv, rv);
            change_val_rFlag(rv);
            return lv;
        }else if(root->data == OR) {         // + -
            if(R->oper_count > L->oper_count){
                Flag = 0;
                find_left_var(L, R);
                if(!Flag) {
                    rv = evaluateTree(R);
                    lv = evaluateTree(L);     //多的先
                }else {
                    lv = evaluateTree(L);
                    rv = evaluateTree(R);
                }
            } else {
                lv = evaluateTree(L);
                rv = evaluateTree(R);
            }
            root->val = L->val | R->val;
            print("OR", lv, rv);
            change_val_rFlag(rv);
            return lv;
        }else if(root->data == XOR) {         // + -
            if(R->oper_count > L->oper_count){
                Flag = 0;
                find_left_var(L, R);
                if(!Flag) {
                    rv = evaluateTree(R);
                    lv = evaluateTree(L);     //多的先
                }else {
                    lv = evaluateTree(L);
                    rv = evaluateTree(R);
                }
            } else {
                lv = evaluateTree(L);
                rv = evaluateTree(R);
            }
            root->val = L->val ^ R->val;
            print("XOR", lv, rv);
            change_val_rFlag(rv);
            return lv;
        }else if(root->data == INCDEC) {         // ++
            if(strcmp(root->lexeme, "++") == 0) {
                root->val = table[getval(R->lexeme)].val + 1;
                r_idx = find_r_idx();
                printf("MOV r%d %d\n", r_idx, 1);
                (r_idx == 0) ? (retval = -10) : (retval = -r_idx);
                change_val_rFlag(retval);

                r_idx = find_r_idx();
                (r_idx == 0) ? (tmp = -10) : (tmp = -r_idx);
                print("MOV",tmp, getval(R->lexeme));
                change_val_rFlag(tmp);

                print("ADD", tmp, retval);
                change_val_rFlag(retval);
                print("MOV", getval(R->lexeme), tmp);
                return tmp;
            }else if(strcmp(root->lexeme, "--") == 0) {
                root->val = table[getval(R->lexeme)].val - 1;
                r_idx = find_r_idx();
                printf("MOV r%d %d\n", r_idx, 1);
                (r_idx == 0) ? (retval = -10) : (retval = -r_idx);
                change_val_rFlag(retval);

                r_idx = find_r_idx();
                (r_idx == 0) ? (tmp = -10) : (tmp = -r_idx);
                print("MOV",tmp, getval(R->lexeme));
                change_val_rFlag(tmp);

                print("SUB", tmp, retval);
                change_val_rFlag(retval);
                print("MOV", getval(R->lexeme), tmp);
                return tmp;
            }
        }
    }else err(SYNTAXERR);
}
/*
r0 -> -10    r1 -> -1     r2 -> -2
[0] -> 0     [4] -> 1     [8] -> 2     */
void print(char* s, int left, int right) {
    if(left >= 0 && right >= 0) printf("%s [%d] [%d]\n", s, 4*left, 4*right);
    else if(left < 0 && right >= 0) {
        if (left == -10) printf("%s r%d [%d]\n", s, 0, 4*right);
        else printf("%s r%d [%d]\n", s, -left, 4*right);
    }
    else if(left >= 0 && right < 0) {
        if (right == -10) printf("%s [%d] r%d\n", s, 4*left, 0);
        else printf("%s [%d] r%d\n", s, 4*left, -right);
    }
    else if( left < 0 && right < 0) {
        if(left == -10) printf("%s r%d r%d\n", s, 0, -right);
        else if(right == -10) printf("%s r%d r%d\n", s, -left, 0);
        else printf("%s r%d r%d\n", s, -left, -right);
    }
}
int cal_int(char* s ) {
    int x = s[0] - '0';
    int i = 1;
    while(s[i] != '\0') {
        x = x* 10 + s[i] - '0';
        i++;
    }
    return x;
}
int find_r_idx(void) {
    for(int i = 0; i < 8; i++) {
        if( rFlag[i] == 0) return i;
    }
}
void change_val_rFlag(int i) {
    if(i >= 0) return;
    (i == -10) ? (i = 0) : (i = -i);

    if(rFlag[i]) rFlag[i] = 0;
    else rFlag[i] = 1;

    return;
}
