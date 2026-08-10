/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <memory/vaddr.h>

enum {
  TK_NOTYPE = 256, 
  TK_EQ,
  TK_NEQ,
  TK_HEX,
  TK_NUM,
  TK_REG,
  TK_AND,
  TK_OR,
  TK_NEG,
  TK_GE,
  TK_LE,
  TK_DEREF,
  /* TODO: Add more token types */
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    	// spaces
  {"\\+", '+'},         	// plus
  {"-", '-'},			// sub
  {"\\*", '*'},			// multi
  {"/", '/'},			// div
  {"\\(", '('},			// lpa
  {"\\)", ')'},			// rpa
  {"&&", TK_AND},		// and
  {"\\|\\|", TK_OR},		// or
  {"==", TK_EQ},        	// equal
  {"!=", TK_NEQ},		// not_equal
  {">=", TK_GE},		// greater or equal
  {"<=", TK_LE},		// less or equal
  {">", '>'},			// greater than
  {"<", '<'},			// less than
  {"0x[0-9a-fA-F]+", TK_HEX},	// hex
  {"[0-9]+", TK_NUM},		// num
  {"\\$[a-zA-Z0-9]+", TK_REG},	// reg
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
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

static Token tokens[65536] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(const char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        const char *substr_start = e + position;
        int substr_len = pmatch.rm_eo - pmatch.rm_so;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
		case (TK_NOTYPE) : 
			break;
          	default: 
			if(nr_token >= ARRLEN(tokens)){
				printf("[Error]:Too many tokens (max 32)\n");
				return false;
			}
			tokens[nr_token].type = rules[i].token_type;
			if(substr_len >= sizeof(tokens[nr_token].str)){
				printf("[Error]:Token string too long at position %d\n",position - substr_len);
				return false;
			}
			memcpy(tokens[nr_token].str,substr_start,substr_len);
			tokens[nr_token].str[substr_len] = '\0';
			nr_token++;
			break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  /*加入负数判断*/
  for(int j = 0; j < nr_token; j ++) {
	  if(j == 0) {
	  	if(tokens[j].type == '-') {
			tokens[j].type = TK_NEG;
		}
	  }
	  else {
	  	int lt = tokens[j-1].type;
		if(tokens[j].type == '-' &&
			lt != TK_NUM && lt != TK_HEX && lt != TK_REG && lt != ')'){
			tokens[j].type = TK_NEG;
		}
	  }
  }
  /*加入解析地址符判断*/
  for(int j = 0; j < nr_token; j ++) {
  	if(j == 0) {
		if(tokens[j].type == '*') {
			tokens[j].type = TK_DEREF;
		}
	}
	else {
		int lt = tokens[j-1].type;
		if(tokens[j].type == '*' &&
			lt != TK_NUM && lt != TK_HEX && lt != TK_REG && lt != ')'){
			tokens[j].type = TK_DEREF;
		}
	}
  }

  //TEST
  printf("nr_token = %d\n", nr_token);
    for(int j = 0; j < nr_token; j ++) {
  	printf("tokens[%d]: type = %d, str = %s\n",j,tokens[j].type,tokens[j].str);
    }
  return true;
}

static bool check_parentheses(int p, int q) {
	if(tokens[p].type != '(' || tokens[q].type != ')') {
		return false;
	}
	int depth = 0;
	for(int i = p; i <= q; i ++) {
		if(tokens[i].type == '('){
			depth++;
		}
		else if (tokens[i].type == ')'){
			depth--;
		}
		if(depth < 0) {
			return false;
		}
		if(depth == 0 && i < q) {
			return false;
		}
	}
	if(depth != 0) {
		return false;
	}
	return true;
}

static int precedence(int type) {
	switch (type) {
		case TK_AND :
			return 1;
		case TK_EQ :
		case TK_NEQ :
		case TK_GE :
		case TK_LE :
		case '>' :
		case '<' :
			return 2;
		case '+' :
		case '-' :
			return 3;
		case '*' :
		case '/' :
			return 4;
		case TK_NEG : 
		case TK_DEREF :
			return 5;
		default : 
			return 100;
	}
}

static int find_main_op(int p, int q) {
	int op = -1;
	int min_prec = 100;
	int depth = 0;
	for(int i = p; i <= q; i ++) {
		int type = tokens[i].type;
		if(type == '(') {
			depth++;
			continue;
		}
		if(type == ')') {
			depth--;
			continue;
		}
		if(depth != 0) {
			continue;
		}
		/*目前只支持四则运算*/
		/*加入负号运算*/
		/*加入 && || == != >= <= > <运算*/
		/*加入解析地址符号运算*/
		if(type != '+' && type != '-' && type != '*' && type != '/' && type != TK_NEG
				&& type != TK_AND && type != TK_OR && type != TK_EQ && type != TK_NEQ
				&& type != TK_GE && type != TK_LE && type != '>' && type != '<'
				&& type != TK_DEREF) {
			continue;
		}
		int prec = precedence(type);
		if(prec < min_prec) {
			op = i;
			min_prec = prec;
		}
		else if(prec == min_prec) {
			if(tokens[op].type != TK_NEG && tokens[op].type != TK_DEREF) {
				op = i;
			}
		}
	}
	return op;
}

static word_t eval(int p, int q, bool *success) {
	//DEBUG
	//printf("p = %d q = %d\n",p,q);
	//fflush(stdout);
	if(p > q) {
		*success = false;
		return 0;
	}
	if(p == q){
		if(tokens[p].type == TK_NUM) {
			return (word_t)strtoul(tokens[p].str,NULL,10);
		}
		if(tokens[p].type == TK_HEX) {
			return (word_t)strtoul(tokens[p].str,NULL,16);
		}
		/*isa_reg_str2val可以传递success参数*/
		if(tokens[p].type == TK_REG) {
			word_t value = isa_reg_str2val(tokens[p].str,success);
			if(*success == false) {
				return 0;	
			}
			return value;
		}
		assert(0);
	}
	else if(check_parentheses(p,q)){
		word_t ans = eval(p+1,q-1,success);
		if(!*success) return 0;
		return ans;
	}
	else {
		int op = find_main_op(p,q);
		if(op == -1) {
			*success = false;
			return 0;
		}
		//&& || 的短路规则
		if(tokens[op].type == TK_OR) {
			word_t val1 = eval(p,op-1,success);
			if(!*success) return 0;
			if(val1 == 1) {
				return 1;
			}
			word_t val2 = eval(op+1,q,success);
			if(!*success) return 0;
			return val1 || val2;
		}
		if(tokens[op].type == TK_AND) {
			word_t val1 = eval(p,op-1,success);
			if(!*success) return 0;
			if(val1 == 0) {
				return 0;
			}
			word_t val2 = eval(op+1,q,success);
			if(!*success) return 0;
			return val1 && val2;
		}
		word_t val2 = eval(op+1,q,success);
		if(!*success) return 0;
		if(tokens[op].type == TK_NEG) {
			return 0 - val2;	
		}
		
		if(tokens[op].type == TK_DEREF) {
			word_t addr = eval(op + 1,q,success);
			if(!*success) {
				return 0;
			}
			return vaddr_read(addr,4);
		}
		word_t val1 = eval(p,op-1,success);
		if(!*success) return 0;
		switch (tokens[op].type) {
			case '+' : return val1 + val2;
		 	case '-' : return val1 - val2;
			case '*' : return val1 * val2;
			case '/' : if(val2 == 0) {
					printf("%s\n",ANSI_FMT("The divisor cannot be ZERO(0)!\n",ANSI_FG_RED));
				   	*success = false;
					return 0;
				   }
				   return val1 / val2;
			case TK_EQ :
				   return val1 == val2;
			case TK_NEQ :
				   return val1 != val2;
			case TK_GE :
				   return val1 >= val2;
			case TK_LE :
				   return val1 <= val2;
			case '>' : return val1 > val2;
			case '<' : return val1 < val2;

			default: assert(0);
		}
	}
}


word_t expr(const char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  //TEST
  *success = true;

  word_t result = eval(0,nr_token - 1,success);

  return result;
}
