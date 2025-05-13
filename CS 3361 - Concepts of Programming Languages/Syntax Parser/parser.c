/*
Combined Lexical Analyzer and Syntax Parser for the Cooke Programming Language

Name:               Christian Maldonado
R#:                 RX
Course:             CS 3361-001
Assignment:         Project II
Instructor:         Eric Rees, PH.D.
Due Date:           11/20/2024
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEXEME_LEN 100
#define MAX_TOKENS 1000

// Token types
typedef enum {
    ASSIGN_OP, LESSER_OP, GREATER_OP, EQUAL_OP, NEQUAL_OP,
    LEQUAL_OP, GEQUAL_OP, OPEN_PAREN, CLOSE_PAREN,
    ADD_OP, SUB_OP, MULT_OP, DIV_OP, MOD_OP,
    BOOL_AND, BOOL_OR, BOOL_NOT, SEMICOLON,
    KEY_IN, KEY_OUT, KEY_IF, KEY_ELSE,
    OPEN_CURL, CLOSE_CURL, IDENT, INT_LIT, UNKNOWN
} TokenType;

// Lexeme array
typedef struct {
    char lexeme[MAX_LEXEME_LEN];
    TokenType token;
} Token;

// Global variables
FILE* sourceFile;
Token currentToken;
int lineNumber = 1;
int hasError = 0;

// Declaring usage functions
Token getNextToken(FILE* file);
const char* getTokenName(TokenType token);
int isValidIdentChar(char c);
TokenType getKeywordToken(const char* lexeme);
void reportError();
void match(TokenType expectedToken);
void P();
void S();
void C();
void E();
void T();
void F();
void V();
void N();

// Converting TokenType to string representation
const char* getTokenName(TokenType token) {
    switch(token) {
        case ASSIGN_OP: return "ASSIGN_OP";
        case LESSER_OP: return "LESSER_OP";
        case GREATER_OP: return "GREATER_OP";
        case EQUAL_OP: return "EQUAL_OP";
        case NEQUAL_OP: return "NEQUAL_OP";
        case LEQUAL_OP: return "LEQUAL_OP";
        case GEQUAL_OP: return "GEQUAL_OP";
        case OPEN_PAREN: return "OPEN_PAREN";
        case CLOSE_PAREN: return "CLOSE_PAREN";
        case ADD_OP: return "ADD_OP";
        case SUB_OP: return "SUB_OP";
        case MULT_OP: return "MULT_OP";
        case DIV_OP: return "DIV_OP";
        case MOD_OP: return "MOD_OP";
        case BOOL_AND: return "BOOL_AND";
        case BOOL_OR: return "BOOL_OR";
        case BOOL_NOT: return "BOOL_NOT";
        case SEMICOLON: return "SEMICOLON";
        case KEY_IN: return "KEY_IN";
        case KEY_OUT: return "KEY_OUT";
        case KEY_IF: return "KEY_IF";
        case KEY_ELSE: return "KEY_ELSE";
        case OPEN_CURL: return "OPEN_CURL";
        case CLOSE_CURL: return "CLOSE_CURL";
        case IDENT: return "IDENT";
        case INT_LIT: return "INT_LIT";
        case UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

// Checking char as valid identifier
int isValidIdentChar(char c) {
    return (c >= 'a' && c <= 'z');
}

// Determining lexeme keyword/identifier
TokenType getKeywordToken(const char* lexeme) {
    if (strcmp(lexeme, "input") == 0) return KEY_IN;
    if (strcmp(lexeme, "output") == 0) return KEY_OUT;
    if (strcmp(lexeme, "if") == 0) return KEY_IF;
    if (strcmp(lexeme, "else") == 0) return KEY_ELSE;
    return IDENT;
}

// Get next token (lexical analyzer)
Token getNextToken(FILE* file) {
    Token token = {.lexeme = "", .token = UNKNOWN};
    char c;
    int i = 0;
    int foundNewline = 0;
    
    // Count lines and skip whitespace
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            lineNumber++;
            foundNewline = 1;
        }
        if (!isspace(c)) {
            break;
        }
    }
    
    if (c == EOF) {
        return token;
    }

    // Handle special characters
    if (c == '$' || c == '@' || c == ':') {
        token.lexeme[0] = c;
        token.lexeme[1] = '\0';
        token.token = UNKNOWN;
        return token;
    }
    
    token.lexeme[i++] = c;
    token.lexeme[i] = '\0';
    
    // Handle identifiers and keywords
    if (isValidIdentChar(c)) {
        while ((c = fgetc(file)) != EOF && (isValidIdentChar(c) || isdigit(c))) {
            if (i < MAX_LEXEME_LEN - 1) {
                token.lexeme[i++] = c;
                token.lexeme[i] = '\0';
            }
        }
        ungetc(c, file);
        token.token = getKeywordToken(token.lexeme);
        return token;
    }
    
    // Handle numbers
    if (isdigit(c)) {
        while ((c = fgetc(file)) != EOF && isdigit(c)) {
            if (i < MAX_LEXEME_LEN - 1) {
                token.lexeme[i++] = c;
                token.lexeme[i] = '\0';
            }
        }
        ungetc(c, file);
        token.token = INT_LIT;
        return token;
    }
    
    // Handle operators and other symbols
    switch (c) {
        case '=':
            c = fgetc(file);
            if (c == '=') {
                token.lexeme[i++] = c;
                token.lexeme[i] = '\0';
                token.token = EQUAL_OP;
            } else {
                ungetc(c, file);
                token.token = ASSIGN_OP;
            }
            return token;
            
        case '<':
            c = fgetc(file);
            if (c == '=') {
                token.lexeme[i++] = c;
                token.lexeme[i] = '\0';
                token.token = LEQUAL_OP;
            } else {
                ungetc(c, file);
                token.token = LESSER_OP;
            }
            return token;
            
        case '>':
            c = fgetc(file);
            if (c == '=') {
                token.lexeme[i++] = c;
                token.lexeme[i] = '\0';
                token.token = GEQUAL_OP;
            } else {
                ungetc(c, file);
                token.token = GREATER_OP;
            }
            return token;
            
        case '!':
            c = fgetc(file);
            if (c == '=') {
                token.lexeme[i++] = c;
                token.lexeme[i] = '\0';
                token.token = NEQUAL_OP;
            } else {
                ungetc(c, file);
                token.token = BOOL_NOT;
            }
            return token;
            
        case '&':
            c = fgetc(file);
            if (c == '&') {
                token.lexeme[i++] = c;
                token.lexeme[i] = '\0';
                token.token = BOOL_AND;
            } else {
                ungetc(c, file);
                token.token = UNKNOWN;
            }
            return token;

        case '|':
            c = fgetc(file);
            if (c == '|') {
                token.lexeme[i++] = c;
                token.lexeme[i] = '\0';
                token.token = BOOL_OR;
            } else {
                ungetc(c, file);
                token.token = UNKNOWN;
            }
            return token;
        
        case '+': token.token = ADD_OP; return token;
        case '-': token.token = SUB_OP; return token;
        case '*': token.token = MULT_OP; return token;
        case '/': token.token = DIV_OP; return token;
        case '%': token.token = MOD_OP; return token;
        case '(': token.token = OPEN_PAREN; return token;
        case ')': token.token = CLOSE_PAREN; return token;
        case '{': token.token = OPEN_CURL; return token;
        case '}': token.token = CLOSE_CURL; return token;
        case ';': token.token = SEMICOLON; return token;
        default: token.token = UNKNOWN; return token;
    }
}

// Error reporting function
void reportError() {
    if (!hasError) {
        printf("Error encounter on line %d: The next lexeme was %s and the next token is %s\n",
               lineNumber, currentToken.lexeme, getTokenName(currentToken.token));
        hasError = 1;
    }
}

// Match and consume expected token
void match(TokenType expectedToken) {
    if (currentToken.token == expectedToken) {
        currentToken = getNextToken(sourceFile);
    } else {
        reportError();
    }
}

// Recursive descent parsing functions
// P() function
void P() { // P ::= S
    S();
}

// S() function
void S() {
    if (hasError) return;
    
    // Handle empty input or end of file
    if (currentToken.lexeme[0] == '\0') {
        return;
    }
    
    switch(currentToken.token) {
        case IDENT: // V = E;
            V();
            if (hasError) return;
            match(ASSIGN_OP);
            if (hasError) return;
            E();
            if (hasError) return;
            match(SEMICOLON);
            if (currentToken.lexeme[0] != '\0' && !hasError) {
                S();
            }
            break;
            
        case KEY_IN: // input(V);
            match(KEY_IN);
            if (hasError) return;
            match(OPEN_PAREN);
            if (hasError) return;
            V();
            if (hasError) return;
            match(CLOSE_PAREN);
            if (hasError) return;
            match(SEMICOLON);
            if (currentToken.lexeme[0] != '\0' && !hasError) {
                S();
            }
            break;
            
        case KEY_OUT: // output(E);
            match(KEY_OUT);
            if (hasError) return;
            match(OPEN_PAREN);
            if (hasError) return;
            E();
            if (hasError) return;
            match(CLOSE_PAREN);
            if (hasError) return;
            match(SEMICOLON);
            if (currentToken.lexeme[0] != '\0' && !hasError) {
                S();
            }
            break;
            
        case KEY_IF: // if ( C ) { S } else { S }
            match(KEY_IF);
            if (hasError) return;
            match(OPEN_PAREN);
            if (hasError) return;
            C();
            if (hasError) return;
            match(CLOSE_PAREN);
            if (hasError) return;
            match(OPEN_CURL);
            if (hasError) return;
            S();
            if (hasError) return;
            match(CLOSE_CURL);
            
            if (currentToken.token == KEY_ELSE) {
                match(KEY_ELSE);
                if (hasError) return;
                match(OPEN_CURL);
                if (hasError) return;
                S();
                if (hasError) return;
                match(CLOSE_CURL);
            }
            
            if (currentToken.lexeme[0] != '\0' && !hasError) {
                S();
            }
            break;
            
        case '\0': // Empty input
            return;
    }
}

// C() function
void C() {
    if (hasError) return;

    if (currentToken.token == BOOL_NOT) {
        match(BOOL_NOT);
        C();
    } else {
        E();
        if (currentToken.token == LESSER_OP || currentToken.token == GREATER_OP ||
            currentToken.token == EQUAL_OP || currentToken.token == NEQUAL_OP ||
            currentToken.token == LEQUAL_OP || currentToken.token == GEQUAL_OP) {
            TokenType op = currentToken.token;
            match(op);
            E();
        }
        
        if (currentToken.token == BOOL_AND || currentToken.token == BOOL_OR) {
            TokenType logicOp = currentToken.token;
            match(logicOp);
            C();
        }
    }
}

void E() { // E ::= T | E + T | E - T
    if (hasError) return;
    
    T();
    while (currentToken.token == ADD_OP || currentToken.token == SUB_OP) {
        TokenType op = currentToken.token;
        match(op);
        T();
    }
}

void T() { // T ::= F | T * F | T / F | T % F
    if (hasError) return;
    
    F();
    while (currentToken.token == MULT_OP || currentToken.token == DIV_OP || currentToken.token == MOD_OP) {
        TokenType op = currentToken.token;
        match(op);
        F();
    }
}

void F() { // F ::= (E) | N | V
    if (hasError) return;
    
    if (currentToken.token == OPEN_PAREN) {
        match(OPEN_PAREN);
        E();
        match(CLOSE_PAREN);
    }
    else if (currentToken.token == INT_LIT) {
        N();
    }
    else if (currentToken.token == IDENT) {
        V();
    }
    else {
        reportError();
    }
}

void V() { // V ::= a | b | ... | y | z | aV | bV | ... | yV | zV ......................
    if (hasError) return;
    
    if (currentToken.token == IDENT) {
        match(IDENT);
    }
    else {
        reportError();
    }
}

void N() { // N ::= 0 | 1 | ... | 8 | 9 | 0N | 1N | ... | 8N | 9N ..........................
    if (hasError) return;
    
    if (currentToken.token == INT_LIT) {
        match(INT_LIT);
    }
    else {
        reportError();
    }
}

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != 2) {
        printf("Usage: %s <source_file>\n", argv[0]);
        return 2;
    }
    
    // Try to open the source file
    sourceFile = fopen(argv[1], "r");
    if (!sourceFile) {
        printf("Error: Could not open file %s\n", argv[1]);
        return 3;
    }
    
    // Print R# header
    printf("Cooke Parser :: RX\n");
    
    // Initialize parsing
    lineNumber = 1;
    hasError = 0;
    currentToken = getNextToken(sourceFile);
    
    // Start parsing from the root!
    P();
    
    // Only check for trailing content if no errors yet
    if (!hasError) {
        Token nextToken = getNextToken(sourceFile);
        // Only report error if there's actual content, not just whitespace
        while (nextToken.lexeme[0] != '\0') {
            if (!isspace(nextToken.lexeme[0])) {
                currentToken = nextToken;
                reportError();
                break;
            }
            nextToken = getNextToken(sourceFile);
        }
    }
    
    // Close file
    fclose(sourceFile);
    
    // Print result and return exit code
    if (!hasError) {
        printf("Syntax Validated\n");
        return 0;
    }
    return 1;
}