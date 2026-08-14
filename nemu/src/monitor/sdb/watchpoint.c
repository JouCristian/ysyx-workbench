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

#include "sdb.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>


typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char expression[WP_EXPR_LEN];
  word_t old_value;

  /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

WP* new_wp() {
	if(free_ == NULL) {
		printf("[Error]: Sorry,No space for new WatchPoints!,Try to free some?\n");
		return NULL;
	}	
	WP *wp;
	wp = free_;
	free_ = wp->next;
	wp->next = head;
	head = wp;
	return wp;
}

int free_wp(WP *wp) {
	if(wp == NULL) {
		printf("[Error]: Sorry!We can't free 'NULL'.\n");
		return 0;
	}
	if(head == wp) {
		head = head->next;
	}
	else {
		WP *prev = head;
		while(prev != NULL && prev->next != wp) {
			prev = prev->next;	
		}
		if(prev == NULL) {
			printf("[Error]: Sorry,we can't find the suitable WatchPoints to free.\n");
			return 0;
		}
		prev->next = wp->next;
	}
	wp->next = free_;
	free_ = wp;
	return 1;
}

int create_wp(const char *expression) {
	if(expression == NULL || expression[0] == '\0') {
		printf("[Error]: None Expression!\n");
		return 0;
	}	
	int expr_len = strlen(expression);
	if(expr_len >= WP_EXPR_LEN) {
		printf("[Error]: Expression too long!\n");
		return 0;
	}
	bool success;
	word_t ov = expr(expression,&success);
	if(!success) {
		printf("[Error]: Invalid Expression!\n");
		return 0;
	}
	WP* wp = new_wp();
	if(wp == NULL) return 0;
	memcpy(wp->expression,expression,expr_len + 1);
	wp->old_value = ov;	

	printf("WatchPoint %d: %s\n",wp->NO,wp->expression);
	printf("Initial Valid:" FMT_WORD "\n",wp->old_value);
	return 1;
}

void display_watchpoints(void) {
	if(head == NULL) {
		printf("%s: Empty watchpoints pool!\n", ANSI_FMT("[ERROR]",ANSI_FG_RED));
		return;
	}
	printf("NO\tValue\tExpression\n");
	WP *wp = head;
	while(wp != NULL) {
		printf("%d\t" FMT_WORD "\t%s\n",wp->NO,wp->old_value,wp->expression);
		wp = wp->next;
	}
}

int delete_wp(int no) {
	WP *wp = head;
	while(wp != NULL) {
		if(wp->NO == no) {
			if(!free_wp(wp)){
				return 0;
			}
			printf("WatchPoint %d has been deleted!\n",no);
			return 1;
		}
		wp = wp->next;
	}
	printf("%s: WatchPoint %d doesn't exist!\n",ANSI_FMT("[ERROR]",ANSI_FG_RED),no);
	return 0;
}

bool check_watchpoints(void) {
	bool change = false;
	WP *wp = head;
	while(wp != NULL) {
		bool success;
		word_t new_value = expr(wp->expression,&success);
		if(!success) {
			printf("%s: Can't evaluate watchpoints' expression\n",ANSI_FMT("[ERROR]",ANSI_FG_RED));
			printf("NO: %d\n",wp->NO);
			printf("Expression: %s\n",wp->expression);
			printf("Old_value: " FMT_WORD "\n",wp->old_value);
		}
		if(new_value != wp->old_value) {
			printf("%s\n", ANSI_FMT("CHANGED!", ANSI_FG_RED));
			printf("NO: %d\n",wp->NO);
			printf("Expression: %s\n",wp->expression);
			printf("Old_value: " FMT_WORD "\n",wp->old_value);
			printf("New_value: " FMT_WORD "\n",new_value);
			wp->old_value = new_value;
			change = true;
		}
		wp = wp->next;
	}
	return change;
}

bool empty_wp(void) {
	return head == NULL;
}

/* TODO: Implement the functionality of watchpoint */

