/*
Lexical Analyzer for the Cooke Programming Language

Name:               Christian Maldonado
R#:                 RX
Course:             CS 3361-001
Assignment:         Project I
Instructor:         Eric Rees, PH.D.
Due Date:           11/06/2024

Requirement Checks:
    - Reads input file specified as command line argument
    - Identifies all lexemes defined in the Cooke language specification
    - Maps lexemes to appropriate tokens
    - Handles whitespace and delimiters correctly
    - Reports unknown tokens without crashing
    - Follows proper formatting requirements
*/

// Declaring include libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Declaring definitions
#define MAX_LEXEME_LEN 100
#define MAX_TOKENS 1000

// Deckaring token types
typedef enum {
    ASSIGN_OP, LESSER_OP, GREATER_OP, EQUAL_OP, NEQUAL_OP,
    LEQUAL_OP, GEQUAL_OP, OPEN_PAREN, CLOSE_PAREN,
    ADD_OP, SUB_OP, MULT_OP, DIV_OP, MOD_OP,
    BOOL_AND, BOOL_OR, BOOL_NOT, SEMICOLON,
    KEY_IN, KEY_OUT, KEY_IF, KEY_ELSE,
    OPEN_CURL, CLOSE_CURL, IDENT, INT_LIT, UNKNOWN
} TokenType;

// Declaring structure for token's lexeme and type
typedef struct {
    char lexeme[MAX_LEXEME_LEN];
    TokenType token;
} Token;

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

// Declaring next token from input file
Token getNextToken(FILE* file) {
    Token token = {.lexeme = "", .token = UNKNOWN};
    char c;
    int i = 0;
    
    // Skipping whitespace and newlines
    while ((c = fgetc(file)) != EOF && (isspace(c) || c == '\n' || c == '\r'));
    if (c == EOF) {
        return token;
    }

    // Handling special characters
    if (c == '$' || c == '@' || c == ':') { 
        token.lexeme[0] = c;
        token.lexeme[1] = '\0';
        token.token = UNKNOWN;
        return token;
    }
    token.lexeme[i++] = c;
    token.lexeme[i] = '\0';
    
    // Handling identifiers/keywords
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
    
    // Handling numbers
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
    
    // Handling operators and other symbols
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

// Processing cmd agruments and I/O files
int main(int argc, char *argv[]) {
    // Checking arguments
    if (argc != 2) {
        printf("Usage: %s <source_file>\n", argv[0]);
        return 1;
    }
    
    // Opening input file
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        printf("Error: Could not open file %s\n", argv[1]);
        return 1;
    }
    
    // Printing R# header
    printf("Cooke Analyzer :: RX\n");
    
    // Process tokens directly
    Token token;
    while (1) {
        token = getNextToken(file);
        if (token.lexeme[0] == '\0') break;  // EOF reached
        printf("%s\t%s\n", token.lexeme, getTokenName(token.token));
    }
    
    fclose(file);
    return 0;
}