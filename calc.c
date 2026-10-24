// Mohammed Kannankalil 251ADB014
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// AI (ChatGPT): prompt "how to report error position for unclosed parenthesis at end of line"
// Handled unclosed parentheses reporting at the line's '\n' position
typedef enum { TOK_EOF, TOK_EOL, TOK_INT, TOK_PLUS, TOK_MINUS, TOK_MUL, TOK_DIV, TOK_LPAREN, TOK_RPAREN, TOK_ERROR } TokenType;

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    return 0;
}
