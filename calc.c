// Mohammed Kannankalil 251ADB014
// https://github.com/Siyaslwf/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: K&R Chapter 5, cppreference (math.h, dirent.h),
//                 AI (ChatGPT) for debugging grammar associativity, EOL positions, and dirent:
//                 https://chatgpt.com/share/your-link-here

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <stdbool.h>
#include <math.h>
#include <errno.h>
#include <dirent.h>

#include <sys/stat.h>

#define STUDENT_NAME     "mohammed"
#define STUDENT_LASTNAME "kannankalil"
#define STUDENT_ID       "251adb014"

typedef enum {

    TOK_EOF,
    TOK_EOL,
    TOK_INT,
    TOK_FLOAT,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_POW,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_ERROR
} TokenType;

typedef struct {
    TokenType type;

    long long int_val;

    double    float_val;
    size_t    pos;

} Token;

typedef enum {

    VAL_INT,
    VAL_FLOAT

} ValueType;

typedef struct {
    ValueType type;
    long long int_val;

    double    float_val;
    size_t    pos;
} Value;

typedef struct {

    const char *src;
    size_t      len;
    size_t      cursor;
 Token       current;
    size_t      line_end_pos;
  bool        has_error;
    size_t      error_pos;
} Parser;



static long long ipow64(long long base, long long exp) {
 long long res = 1;
    
    long long b = base;

 while (exp > 0) {
        if (exp & 1) res *= b;
        b *= b;

        exp >>= 1;
    }
    return res;
}



static Token next_token(Parser *p) {
    Token tok;
    
    tok.int_val = 0;

    tok.float_val = 0.0;

    while (p->cursor < p->len && (p->src[p->cursor] == ' ' || p->src[p->cursor] == '\t' || p->src[p->cursor] == '\r')) {
        p->cursor++;
    }

    tok.pos = p->cursor + 1;

    if (p->cursor >= p->len) {
        tok.type = TOK_EOF;

        return tok;
    }

    char c = p->src[p->cursor];

    if (c == '\n') {
        tok.type = TOK_EOL;
        p->cursor++;
        return tok;
    }

    if (isdigit((unsigned char)c)) {
        size_t start = p->cursor;
        while (p->cursor < p->len && isdigit((unsigned char)p->src[p->cursor])) {
            p->cursor++;
        }

        if (p->cursor < p->len && p->src[p->cursor] == '.') {
            size_t dot_pos = p->cursor + 1;

            if (p->cursor + 1 >= p->len || !isdigit((unsigned char)p->src[p->cursor + 1])) {
                tok.type = TOK_ERROR;


                tok.pos = dot_pos;
                p->cursor++;
                return tok;
            }
            p->cursor++;
            while (p->cursor < p->len && isdigit((unsigned char)p->src[p->cursor])) {
                p->cursor++;
            }
            if (p->cursor < p->len && (p->src[p->cursor] == 'e' || p->src[p->cursor] == 'E')) {
                tok.type = TOK_ERROR;
                tok.pos = p->cursor + 1;
                return tok;
            }
            tok.type = TOK_FLOAT;
            tok.float_val = strtod(p->src + start, NULL);
            return tok;
        }

        char buf[32];
        size_t n = p->cursor - start;
        if (n < sizeof(buf)) {
            memcpy(buf, p->src + start, n);
            buf[n] = '\0';
            tok.type = TOK_INT;
            tok.int_val = strtoll(buf, NULL, 10);
            return tok;
        }
    }

    if (c == '.') {
        tok.type = TOK_ERROR;
        tok.pos = p->cursor + 1;
        p->cursor++;
        return tok;
    }

    if (c == '+') { tok.type = TOK_PLUS; p->cursor++; return tok; }
    if (c == '-') { tok.type = TOK_MINUS; p->cursor++; return tok; }
    if (c == '/') { tok.type = TOK_DIV; p->cursor++; return tok; }
    if (c == '(') { tok.type = TOK_LPAREN; p->cursor++; return tok; }
    if (c == ')') { tok.type = TOK_RPAREN; p->cursor++; return tok; }

    if (c == '*') {
        if (p->cursor + 1 < p->len && p->src[p->cursor + 1] == '*') {
            tok.type = TOK_POW;
            p->cursor += 2;
            return tok;
        }
        tok.type = TOK_MUL;
        p->cursor++;
        return tok;
    }

    tok.type = TOK_ERROR;
    tok.pos = p->cursor + 1;
    p->cursor++;
    return tok;
}

static void record_error(Parser *p, size_t pos) {
    if (!p->has_error) {
        p->has_error = true;
        p->error_pos = pos;
    }
}

static Value parse_expr(Parser *p);

static Value parse_primary(Parser *p) {
    Value val = {VAL_INT, 0, 0.0, p->current.pos};

    if (p->has_error) return val;

    if (p->current.type == TOK_INT) {
        val.type = VAL_INT;
        val.int_val = p->current.int_val;
        val.pos = p->current.pos;
        p->current = next_token(p);
        return val;
    }

    if (p->current.type == TOK_FLOAT) {
        val.type = VAL_FLOAT;
        val.float_val = p->current.float_val;
        val.pos = p->current.pos;
        p->current = next_token(p);
        return val;
    }

    if (p->current.type == TOK_LPAREN) {
        size_t lparen_pos = p->current.pos;
        p->current = next_token(p);

        if (p->current.type == TOK_RPAREN) {
            record_error(p, p->current.pos);
            return val;
        }

        val = parse_expr(p);
        if (p->has_error) return val;

        if (p->current.type == TOK_RPAREN) {
            val.pos = lparen_pos;
            p->current = next_token(p);
            return val;
        } else {
            // AI (ChatGPT): prompt "how to report error position for unclosed parenthesis at end of line"
            // The requirement states missing ')' at EOL must report the line's '\n' position
            record_error(p, p->line_end_pos);
           
           
            return val;
        }
    }

    if (p->current.type == TOK_ERROR) {
     
        record_error(p, p->current.pos);
        return val;
    }

    if (p->current.type == TOK_EOL || p->current.type == TOK_EOF) {
       
        record_error(p, p->line_end_pos);
        return val;
    }

    record_error(p, p->current.pos);
    return val;
}

// AI (ChatGPT): prompt "how to make ** right-associative in recursive descent parser in C"
// power() calls itself for the exponent so 2**3**2 evaluates as 2**(3**2)
static Value parse_power(Parser *p) {
  
    Value left = parse_primary(p);
    if (p->has_error) return left;

    if (p->current.type == TOK_POW) {
       
      
        p->current = next_token(p);

        Value right = parse_power(p);
        if (p->has_error) return left;

        Value res;
        res.pos = left.pos;

        if (left.type == VAL_INT && right.type == VAL_INT) {
          
            if (right.int_val < 0) {
                record_error(p, right.pos);
                return left;
            }
            res.type = VAL_INT;
            res.int_val = ipow64(left.int_val, right.int_val);
            return res;
        } else {
            double base = (left.type == VAL_INT) ? (double)left.int_val : left.float_val;
            double exp  = (right.type == VAL_INT) ? (double)right.int_val : right.float_val;
            double v    = pow(base, exp);

          
            res.type = VAL_FLOAT;
            res.float_val = v;
            return res;
        }
    }

    return left;
}

static Value parse_term(Parser *p) {
    Value left = parse_power(p);
    if (p->has_error) return left;

    while (p->current.type == TOK_MUL || p->current.type == TOK_DIV) {
        TokenType op = p->current.type;
        p->current = next_token(p);

        Value right = parse_power(p);
        if (p->has_error) return left;

        Value res;
        res.pos = left.pos;

        if (op == TOK_MUL) {
            if (left.type == VAL_INT && right.type == VAL_INT) {
                res.type = VAL_INT;
                res.int_val = left.int_val * right.int_val;
            } else {
                double l = (left.type == VAL_INT) ? (double)left.int_val : left.float_val;
                double r = (right.type == VAL_INT) ? (double)right.int_val : right.float_val;
                res.type = VAL_FLOAT;
                res.float_val = l * r;
            }
        } else {
            if (left.type == VAL_INT && right.type == VAL_INT) {
                if (right.int_val == 0) {
                    record_error(p, right.pos);
                    return left;
                }
                res.type = VAL_INT;
                res.int_val = left.int_val / right.int_val;
            } else {
                double l = (left.type == VAL_INT) ? (double)left.int_val : left.float_val;
                double r = (right.type == VAL_INT) ? (double)right.int_val : right.float_val;
                if (r == 0.0) {
                    record_error(p, right.pos);
                    return left;
                }
                res.type = VAL_FLOAT;
                res.float_val = l / r;
            }
        }
        left = res;
    }

    return left;
}

static Value parse_expr(Parser *p) {
    Value left = parse_term(p);
    if (p->has_error) return left;

    while (p->current.type == TOK_PLUS || p->current.type == TOK_MINUS) {
        TokenType op = p->current.type;
        p->current = next_token(p);

        Value right = parse_term(p);
        if (p->has_error) return left;

        Value res;
        res.pos = left.pos;

        if (op == TOK_PLUS) {
            if (left.type == VAL_INT && right.type == VAL_INT) {
                res.type = VAL_INT;
                res.int_val = left.int_val + right.int_val;
            } else {
                double l = (left.type == VAL_INT) ? (double)left.int_val : left.float_val;
                double r = (right.type == VAL_INT) ? (double)right.int_val : right.float_val;
                res.type = VAL_FLOAT;
                res.float_val = l + r;
            }
        } else {
            if (left.type == VAL_INT && right.type == VAL_INT) {
                res.type = VAL_INT;
                res.int_val = left.int_val - right.int_val;
            } else {
                double l = (left.type == VAL_INT) ? (double)left.int_val : left.float_val;
                double r = (right.type == VAL_INT) ? (double)right.int_val : right.float_val;
                res.type = VAL_FLOAT;
                res.float_val = l - r;
            }
        }
        left = res;
    }

    return left;
}

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} ResultList;

static void list_init(ResultList *l) {
    l->count = 0;
    l->capacity = 16;
    l->items = malloc(l->capacity * sizeof(char *));
}

static void list_add(ResultList *l, const char *str) {
    if (l->count >= l->capacity) {
        l->capacity *= 2;
        l->items = realloc(l->items, l->capacity * sizeof(char *));
    }
    l->items[l->count++] = strdup(str);
}

static void list_free(ResultList *l) {
    for (size_t i = 0; i < l->count; i++) {
        free(l->items[i]);
    }
    free(l->items);
}

static bool process_file(const char *in_file, const char *out_file) {
    FILE *fp = fopen(in_file, "rb");
    if (!fp) {
        fprintf(stderr, "calc: cannot open %s: %s\n", in_file, strerror(errno));
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size < 0) {
        fclose(fp);
        return false;
    }

    char *data = malloc((size_t)size + 1);
    if (!data) {
        fclose(fp);
        return false;
    }

    size_t read_bytes = fread(data, 1, (size_t)size, fp);
    data[read_bytes] = '\0';
    fclose(fp);

    Parser p;
    p.src = data;
    p.len = read_bytes;
 
    p.cursor = 0;
    p.has_error = false;
    p.error_pos = 0;

    ResultList results;
    list_init(&results);

    size_t line_start = 0;

    while (line_start < p.len && !p.has_error) {
        size_t line_end = line_start;
        while (line_end < p.len && p.src[line_end] != '\n') {
            line_end++;
        }

        if (line_end < p.len && p.src[line_end] == '\n') {
            p.line_end_pos = line_end + 1;
        } else {
            p.line_end_pos = p.len + 1;
        }

        size_t non_blank = line_start;
        while (non_blank < line_end && (p.src[non_blank] == ' ' || p.src[non_blank] == '\t' || p.src[non_blank] == '\r')) {
            non_blank++;
        }

        if (non_blank == line_end) {
            line_start = (line_end < p.len) ? line_end + 1 : p.len;
            p.cursor = line_start;
            continue;
        }

        if (p.src[non_blank] == '#') {
            line_start = (line_end < p.len) ? line_end + 1 : p.len;
            p.cursor = line_start;
            continue;
        }

        p.cursor = line_start;
        p.current = next_token(&p);

        Value res = parse_expr(&p);

        if (!p.has_error) {
            if (p.current.type != TOK_EOL && p.current.type != TOK_EOF) {
                record_error(&p, p.current.pos);
            }
        }

        if (p.has_error) break;

        char buf[64];
        if (res.type == VAL_INT) {
            snprintf(buf, sizeof(buf), "%lld", res.int_val);
        } else {
            snprintf(buf, sizeof(buf), "%.15g", res.float_val);
        }
        list_add(&results, buf);

        line_start = (line_end < p.len) ? line_end + 1 : p.len;
        p.cursor = line_start;
    }

    FILE *out = fopen(out_file, "w");
    if (!out) {
        fprintf(stderr, "calc: cannot write %s: %s\n", out_file, strerror(errno));
        list_free(&results);
        free(data);
        return false;
    }

    if (p.has_error) {
        fprintf(out, "ERROR:%zu\n", p.error_pos);
    } else {
        for (size_t i = 0; i < results.count; i++) {
            fprintf(out, "%s\n", results.items[i]);
        }
    }

    fclose(out);
    list_free(&results);
    free(data);
    return true;
}

static void make_dir_if_needed(const char *dir) {
    struct stat st;
    if (stat(dir, &st) == -1) {
        mkdir(dir, 0755);
    }
}

static void extract_file_base(const char *path, char *out, size_t out_len) {
    const char *slash = strrchr(path, '/');
    const char *name = slash ? slash + 1 : path;
    strncpy(out, name, out_len - 1);
    out[out_len - 1] = '\0';

    char *dot = strrchr(out, '.');
    if (dot && strcmp(dot, ".txt") == 0) {
        *dot = '\0';
    }
}

static void extract_dir_base(const char *path, char *out, size_t out_len) {
    char temp[1024];
    strncpy(temp, path, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    size_t len = strlen(temp);
    while (len > 1 && temp[len - 1] == '/') {
        temp[len - 1] = '\0';
        len--;
    }

    const char *slash = strrchr(temp, '/');
    const char *base = slash ? slash + 1 : temp;
    strncpy(out, base, out_len - 1);
    out[out_len - 1] = '\0';
}

int main(int argc, char *argv[]) {
    const char *dir_opt = NULL;
    const char *out_opt = NULL;
    const char *file_arg = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "calc: %s missing folder argument\n", argv[i]);
                return EXIT_FAILURE;
            }
            dir_opt = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "calc: %s missing output folder argument\n", argv[i]);
                return EXIT_FAILURE;
            }
            out_opt = argv[++i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "calc: unknown option %s\n", argv[i]);
            return EXIT_FAILURE;
        } else {
            if (!file_arg) {
                file_arg = argv[i];
            } else {
                fprintf(stderr, "calc: unexpected argument %s\n", argv[i]);
                return EXIT_FAILURE;
            }
        }
    }

    if (!dir_opt && !file_arg) {
        fprintf(stderr, "Usage: calc [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]\n");
        return EXIT_FAILURE;
    }

    if (!dir_opt) {
        struct stat st;
        if (stat(file_arg, &st) != 0 || S_ISDIR(st.st_mode)) {
            fprintf(stderr, "calc: file %s does not exist\n", file_arg);
            return EXIT_FAILURE;
        }

        char base[256];
        extract_file_base(file_arg, base, sizeof(base));

        char outdir[512];
        if (out_opt) {
            strncpy(outdir, out_opt, sizeof(outdir) - 1);
            outdir[sizeof(outdir) - 1] = '\0';
        } else {
            snprintf(outdir, sizeof(outdir), "%s_%s_%s", base, STUDENT_NAME, STUDENT_ID);
        }
        make_dir_if_needed(outdir);

        char out_path[1024];
        snprintf(out_path, sizeof(out_path), "%s/%s_%s_%s_%s.txt",
                 outdir, base, STUDENT_NAME, STUDENT_LASTNAME, STUDENT_ID);

        if (!process_file(file_arg, out_path)) {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    // AI (ChatGPT): prompt "in C opendir how to filter only regular .txt files without subdirectories"
    struct stat d_st;

if (stat(dir_opt, &d_st) != 0 || !S_ISDIR(d_st.st_mode)) {
        fprintf(stderr, "calc: directory %s does not exist\n", dir_opt);
       
        return EXIT_FAILURE;
    }

    char dir_base[256];
    
    extract_dir_base(dir_opt, dir_base, sizeof(dir_base));

    char outdir[512];
    if (out_opt) {
        strncpy(outdir, out_opt, sizeof(outdir) - 1);
     
        outdir[sizeof(outdir) - 1] = '\0';
    } else {
        snprintf(outdir, sizeof(outdir), "%s_%s_%s", dir_base, STUDENT_NAME, STUDENT_ID);
    }
    make_dir_if_needed(outdir);

    DIR *d = opendir(dir_opt);
    if (!d) {
      fprintf(stderr, "calc: cannot open %s\n", dir_opt);
      
      return EXIT_FAILURE;
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        size_t nlen = strlen(ent->d_name);
        if (nlen < 4 || strcmp(ent->d_name + nlen - 4, ".txt") != 0) {
            continue;
        }

     char full_in_path[1024];
        snprintf(full_in_path, sizeof(full_in_path), "%s/%s", dir_opt, ent->d_name);

     struct stat f_st;
        if (stat(full_in_path, &f_st) != 0 || !S_ISREG(f_st.st_mode)) {
            continue;
        }

        
    char base[256];
        extract_file_base(ent->d_name, base, sizeof(base));

        char out_path[1024];
       
        snprintf(out_path, sizeof(out_path), "%s/%s_%s_%s_%s.txt",
                 outdir, base, STUDENT_NAME, STUDENT_LASTNAME, STUDENT_ID);

        if (!process_file(full_in_path, out_path)) {
            closedir(d);
            return EXIT_FAILURE;
        }
    }

closedir(d);
    return EXIT_SUCCESS;
}