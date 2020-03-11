#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

enum {
    TK_NOTYPE = 256,
    TK_NUMBER
        /* TODO: Add more token types */
};

static struct rule {
    char *regex;
    int token_type;
} rules[] = {
    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */
    {" +", TK_NOTYPE},    // one or more spaces, + is not plus
    {"0|[1-9][0-9]*", TK_NUMBER},
    {"\\+", '+'},         // plus
    {"-", '-'},           // minus
    {"\\*", '*'},         // mul
    {"\\/", '/'},         // divide
    {"\\(", '('},         // left parenthesis
    {"\\)", ')'},         // right parenthesis

    // {"==", TK_EQ}         // equal
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];  // store compiled regex

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
    int i;
    char error_msg[128];
    int ret;

    for (i = 0; i < NR_REGEX; i ++) {
        ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);  // compile regex
        if (ret != 0) {
            regerror(ret, &re[i], error_msg, 128);
            panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
        }
    }
}

typedef struct token {
    int type;
    char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
    int position = 0;
    int i;
    regmatch_t pmatch;

    nr_token = 0;

    while (e[position] != '\0') {
        /* Try all rules one by one. */
        for (i = 0; i < NR_REGEX; i ++) {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
                char *substr_start = e + position;
                int substr_len = pmatch.rm_eo;

                Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
                    i, rules[i].regex, position, substr_len, substr_len, substr_start);
                position += substr_len;

                /* TODO: Now a new token is recognized with rules[i]. Add codes
                 * to record the token in the array `tokens'. For certain types
                 * of tokens, some extra actions should be performed.
                 */
                if (substr_len > 32)
                    assert(0);  // TODO: handle this condition
                if (rules[i].token_type == TK_NOTYPE)
                    break;  // ignore space
                else {
                    tokens[nr_token].type = rules[i].token_type;
                    switch (rules[i].token_type) {
                    case TK_NUMBER:
                        strncpy(tokens[nr_token].str, substr_start, substr_len);
                        *(tokens[nr_token].str + substr_len) = '\0';
                        break;
                    }
                    printf("Success record: nr_token=%d, type=%d, str=%s\n",
                           nr_token, tokens[nr_token].type, tokens[nr_token].str);
                    nr_token += 1;  // update nr_token
                    break;  // break for loop, for we've found a match
                }
            }
        }

        if (i == NR_REGEX) {
            printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
            return false;
        }
    }
    return true;
}

bool check_parentheses(int p, int q) {
    if (p >= q) {
        printf("error: p>=q in check_parentheses\n");
        return false;
    }
    if (tokens[p].type != '(' || tokens[q].type != ')')
        return false;

    int count = 0;  // number of unmatched left parentheses
    for (int curr = p + 1; curr < q; curr++) {
        if (tokens[curr].type == '(')
            count++;
        if (tokens[curr].type == ')') {
            if (count != 0)
                count--;
            else
                return false;  // exists unmatched right
        }
    }
    if (count == 0)
        return true;
    else
        return false;  // exists unmatched left
}

int findDominantOp(int p, int q) {
    int pos[2] = {-1, -1};  // rightmost +-, rightmost */
    int level = 0;

    for (int i = 0; i < nr_token; i++) {
        if ((tokens[i].type != '+') && (tokens[i].type != '-')
            && (tokens[i].type != '*') && (tokens[i].type != '/'))
            continue;
        if (tokens[i].type == '(')
            level++;
        if (tokens[i].type == ')')
            level--;
        if (level == 0) {
            if ((tokens[i].type == '+') && (tokens[i].type == '-'))
                pos[0] = i;
            if ((tokens[i].type == '*') && (tokens[i].type == '/'))
                pos[1] = i;
        }
    }
    if (pos[0] != -1)
        return pos[0];
    else 
        return pos[1];
}

int eval(int p, int q) {
    if (p > q) {
        printf("error! p=%d > q=%d in eval\n", p, q);
        assert(0);
    } else if (p == q) {
        return atoi(tokens[p].str);
    } else if (check_parentheses(p, q) == true) {
        return eval(p + 1, q - 1);
    } else {
        int op = findDominantOp(p, q);
        int val1 = eval(p, op - 1);
        int val2 = eval(op + 1, q);
        switch(op) {
        case '+':
            return val1 + val2;
        case '-':
            return val1 - val2;
        case '*':
            return val1 * val2;
        case '/':
            return val1 / val2;
        default:
            assert(0);
        }
    }
}

uint32_t expr(char *e, bool *success) {
    if (!make_token(e)) {
        *success = false;
        return 0;
    }

    *success = true;
    printf("DEBUG: p=%d, q=%d", 0, nr_token - 1);
    return eval(0, nr_token - 1);
}
