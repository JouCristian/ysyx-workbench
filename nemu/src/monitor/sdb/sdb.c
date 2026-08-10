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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <errno.h>
#include "sdb.h"
#include <memory/vaddr.h> 


static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_p(char *args);

static int cmd_test(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Execute N instructions, defaulting to 1.", cmd_si},
  { "info", "Print the state of sth: info 'cmd'.", cmd_info},
  { "x", "Print data from N consecutive memory address: x N addr", cmd_x},
  { "p", "Evaluate expression.",cmd_p},
  { "test", "Test correctness of any cmd: test cmd",cmd_test},
  { "w", "Set a watchpoints for expression: w EXPR",cmd_w},
  { "d", "Delete a watchpoints: d NUM",cmd_d},

  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_d(char *args) {
	if(args == NULL) {
		printf("[Error]: 'd' should be followed with a NUM!\n");
		return 0;
	}
	char *end = NULL;
	long value = strtol(args,&end,10);
	if(end == args || *end != '\0' || value < 0 || value >= NR_WP) {
		printf("[Error]: Invalid watchpoint num.\n");
		if(!empty_wp()){
			printf("We have these watchpoints below!\n");
			display_watchpoints();
		}
		else {
			printf("Empty WatchPoints Pool!\n");
		}
		return 0;
	}	
	return delete_wp((int)value);
}

static int cmd_w(char *args) {
	char *expr = args;
	return create_wp(expr);	
}

static int test_p(char *args);

static struct {
	const char *name;
	const char *description;
	int (*handler)(char *);
}test_table[] = {
	{ "p", "Random test for expression evaluator", test_p},
};

#define NR_TEST ARRLEN(test_table)

static bool test_p_from_file(const char *filename) {
	FILE *fp = fopen(filename,"r");
	if(fp == NULL) {
		printf("[TEST-p] Failed to open %s\n",filename);
		return false;
	}
	char line[65536];
	int total = 0;
	int passed = 0;
	while(fgets(line,sizeof(line),fp) != NULL) {
		total++;
		char *num = strtok(line," ");
		char *expr_str = strtok(NULL,"");
		unsigned long expected = strtoul(num,NULL,10);
		
		expr_str[strcspn(expr_str, "\r\n")] = '\0';

		bool success = false;

		//DEBUG
		printf("[TEST-p] case #%d: %s\n",total,expr_str);
		fflush(stdout);

		word_t actual = expr(expr_str,&success);
		if(success){
			if(actual == (word_t)expected){
				passed++;
			}
			else {
				printf("[TEST-p] Failed at case #%d: Expression:%s Expected:%u Actual:%u\n",
						total,expr_str,(word_t)expected,actual);
				return false;
			}
		}
		else{
			continue;
		}
	}
	fclose(fp);
	printf("[TEST-p] PASS: %d/%d expression passed.\n",passed,total);
	return passed == total;
}

static int test_p(char *args) {
	printf("test_p received: %s\n", args == NULL ? "(null)" : args);
	if(args == NULL) {
		printf("[Error]: test_p should be followed with a number as the LoopTime!\n");
		return 0;
	}
	char *end = NULL;
	errno = 0;
	long value = strtol(args,&end,10);
	if(errno == ERANGE) {
		perror("[Error]: LoopTime value out of range!");
		return 0;
	}
	if(end == args) {
		printf("[Error]: %s isn't a number.Please try again!\n",args);
		return 0;
	}
	if(value <= 0) {
		printf("[Error]: LoopTime must be a positive number!\n");
		return 0;
	}
	if(*end != '\0') {
		printf("[Error]: Invalid LoopTime: %s\n", args);
	}
	char command[256];
	snprintf(command,sizeof(command),"./tools/gen-expr/build/gen-expr %ld > /tmp/nemu-expr-input",value);
	printf("[TEST-p] Generating %ld random expression......\n",value);
	int ret = system(command);
	if(ret != 0) {
		printf("[Error]: Failed to run expression generator.\n");
		return 0;
	}
	printf("[TEST-p] Expression cases generated successfully.\n");
	if(test_p_from_file("/tmp/nemu-expr-input")){
		printf("[TEST-p] TEST PASSED!\n");	
	}

	return 0;	
}

static int cmd_test(char *args) {
	char *testCmd = strtok(NULL," ");
	if(testCmd != NULL) {
		char *testArgs = strtok(NULL,"");
		int i;
		for(i = 0; i < NR_TEST; i++) {
			if(strcmp(test_table[i].name,testCmd) == 0) {
				return test_table[i].handler(testArgs);
			}
		}	
		if(i == NR_TEST) {
			printf("[Error]: No Such cmd like %s. Try it again!\n", testCmd);
			printf("Commands that can be tested are as follow.\n");
			for(int j = 0; j < NR_TEST; j++) {
				printf("#%d: %s\n",j,test_table[j].name);
			}
		}
	}	
	else {
		printf("[Error Usage]: \"test\" should be followed with a \"cmd\"\n");
	}	
	return 0;
}

static int cmd_p(char *args) {
	if(args == NULL) {
		printf("Usage: p EXPR\n");
		return 0;
	}
	bool success = true;
	word_t result = expr(args,&success);
	if(success) {
		printf(FMT_WORD "\n",result);
	}
	else {
		printf("Bad Expression!\n");
	}
	return 0;
}

static void printFomatError_cmd_x(int current_argNum) {
	printf("[Fomat Error]: 'x' should followed with 2 args.\n");
	printf("Now only has %d args", current_argNum);
}

static int cmd_x(char *args) {
	int current_argNum = 0;
	char *arg1 = strtok(NULL, " ");
	if(arg1 == NULL) {
		printFomatError_cmd_x(current_argNum);
		return 0;
	}
	current_argNum++;
	char *arg2 = strtok(NULL, " ");
	if(arg2 == NULL) {
		printFomatError_cmd_x(current_argNum);
		return 0;
	}
	current_argNum++;
	while(strtok(NULL, " ") != NULL) {
		current_argNum++;
	}
	if(current_argNum > 2) {
		printFomatError_cmd_x(current_argNum);
		return 0;
	}
	char *end1 = NULL;
	errno = 0;
	long num = strtol(arg1,&end1,10);
	if(errno == ERANGE || end1 == arg1 || *end1 != '\0' ||  num <= 0) {
		printf("[Error Usage]: The first arg should be a posetive integer.\n");
		return 0;
	}
	char *end2 = NULL;
	errno = 0;
	vaddr_t addr = (vaddr_t)strtoul(arg2,&end2,16);
	if(errno == ERANGE || end2 == arg2 || *end2 != '\0') {
		printf("[Error Usage]: The second arg should be a posetive address: 0x????????.\n");
		return 0;	
	}
	for(size_t i = 0; i < (size_t)num; i ++) {
		word_t data = vaddr_read(addr,4);
		printf("Addr: " FMT_WORD "\t Data: " FMT_WORD "\n",addr,data);
		addr += 4;	
	}
	return 0;
}

static struct {
	const char *name;
	const char *description;
	void (*handler)(void);
}info_table[] = {
	{ "r", "Print the status of all current REGS!", isa_reg_display},
	{ "w", "Print all watchpoints infomation!", display_watchpoints},
};

#define NR_INFO ARRLEN(info_table)

static int cmd_info(char *args) {
	if(args == NULL) {
		printf("[Error Usage]: info should be followed with a 'cmd'.\n");
		printf("We have:\n");
		for(int i = 0; i < NR_INFO; i++) {
			printf("%s: %s\n",info_table[i].name,info_table[i].description);
		}
		return 0;
	}
	char *cmd = strtok(args, " ");
	char *extraArgs = strtok(NULL,"");
	if(extraArgs != NULL) {
		printf("[Error Usage]: Don't accept extra Args!\n");
		return 0;
	}
	for(int i = 0; i < NR_INFO; i++) {
		if(strcmp(cmd,info_table[i].name) == 0) {
			info_table[i].handler();	
			return 0;
		}
	}
	printf("[Error Cmd]: No kind of such cmd like %s\n",cmd);
	return 0;
}

static int cmd_si(char *args) {
	char *arg = strtok(NULL, " ");
	uint64_t exec_time = 1;

	if(arg != NULL) {
		char *end = NULL;
		errno = 0;
		long value = strtol(arg, &end, 10);
		if(errno == ERANGE || end == arg || *end != '\0' || value <= 0) {
			printf("[Error Usage!]: si N, N should be a single positive integer!\n");
			return 0;
		}
		exec_time = (uint64_t)value;
	}

	cpu_exec(exec_time);

	return 0;
}

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
