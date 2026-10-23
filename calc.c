// Mohammed Kannankalil 251ADB014
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

typedef enum { TOK_EOF, TOK_EOL, TOK_INT, TOK_PLUS, TOK_MINUS, TOK_ERROR } TokenType;
typedef struct { TokenType type; long long val; size_t pos; } Token;
typedef struct { const char *src; size_t len; size_t cursor; Token current; bool has_error; size_t error_pos; } Parser;

static Token next_token(Parser *p) {
    Token tok = {TOK_EOF, 0, p->cursor + 1};
    while (p->cursor < p->len && (p->src[p->cursor] == ' ' || p->src[p->cursor] == '\t')) p->cursor++;
    tok.pos = p->cursor + 1;
    if (p->cursor >= p->len) return tok;
    char c = p->src[p->cursor];
    if (c == '\n') { tok.type = TOK_EOL; p->cursor++; return tok; }
    if (isdigit((unsigned char)c)) {
        size_t s = p->cursor;
        while (p->cursor < p->len && isdigit((unsigned char)p->src[p->cursor])) p->cursor++;
        char b[32]; size_t n = p->cursor - s;
        if (n < sizeof(b)) { memcpy(b, p->src + s, n); b[n] = '\0'; tok.type = TOK_INT; tok.val = strtoll(b, NULL, 10); }
        return tok;
    }
    if (c == '+') { tok.type = TOK_PLUS; p->cursor++; return tok; }
    if (c == '-') { tok.type = TOK_MINUS; p->cursor++; return tok; }
    tok.type = TOK_ERROR; tok.pos = p->cursor + 1; p->cursor++;
    return tok;
}

int main(int argc, char *argv[]) {
    if (argc < 2) { fprintf(stderr, "usage: calc file.txt\n"); return 1; }
    printf("Evaluating %s\n", argv[1]);
    return 0;
}
