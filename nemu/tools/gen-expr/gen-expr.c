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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char oracle_buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static uint32_t choose(uint32_t n) {
	return rand() % n;
}

static int buf_pos = 0;
static int oracle_pos = 0;

static void gen(char c) {
	buf[buf_pos] = c;
	buf_pos++;
	buf[buf_pos] = '\0';

	oracle_buf[oracle_pos] = c;
	oracle_pos++;
	oracle_buf[oracle_pos] = '\0';
}

static void gen_num(void) {
	unsigned num = (unsigned)rand() % 100u;
	int len = snprintf(buf + buf_pos,sizeof(buf) - buf_pos,"%uu",num);
	int oracle_len = snprintf(oracle_buf + oracle_pos,sizeof(oracle_buf) - oracle_pos,"%u",num);
	buf_pos += len;
	oracle_pos += oracle_len;
}

static void gen_rand_op(void) {
	switch (choose(4)) {
		case 0 :
			gen('+');
			break;
		case 1 :
			gen('-');
			break;
		case 2 :
			gen('*');
			break;
		case 3 : 
			gen('/');
			break;
	}
}

static int depth = 0; 	//避免缓冲区溢出问题

static void gen_nonZero_num(void) {
	unsigned num = (unsigned)rand() % 10000u + 1;
	int len = snprintf(buf + buf_pos,sizeof(buf) - buf_pos,"%uu",num);
	int oracle_len = snprintf(oracle_buf + oracle_pos,sizeof(oracle_buf) - oracle_pos,"%u",num);
	buf_pos += len;
	oracle_pos += oracle_len;
}

static void gen_rand_expr() {
	if(depth == 10) {
		gen_num();
		return;
	}
	depth++;
	switch (choose(3)) {
		case 0 : gen_num(); break;
		case 1 : gen('('); gen_rand_expr(); gen(')'); break;
		default : gen_rand_expr(); gen_rand_op(); gen_nonZero_num()/*gen_rand_expr()*/; break;
	}
	depth--;
}

static void init_buf(void) {
	buf[0] = '\0';
	buf_pos = 0;
	depth = 0;
	oracle_buf[0] = '\0';
	oracle_pos = 0;
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    //在递归循环开始之前进行初始化
    init_buf();
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr 2>/dev/null");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr 2>/dev/null", "r");
    assert(fp != NULL);

    unsigned result;
    ret = fscanf(fp, "%u", &result);
    pclose(fp);
    if(ret != 1) continue;
    printf("%u %s\n",result,oracle_buf);
  }
  return 0;
}
