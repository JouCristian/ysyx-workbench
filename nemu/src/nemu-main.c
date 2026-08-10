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

#include <common.h>
#include "../include/utils.h"

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

#ifdef CONFIG_WATCHPOINT
  printf("%s\n", ANSI_FMT("WatchPoints ON!", ANSI_FG_GREEN));
#else
  printf("%s\n", ANSI_FMT("WatchPoints OFF!", ANSI_FG_RED));
  printf("%s\n", ANSI_FMT("You can run 'make nemuconfig' under '$NPC_HOME' to turn it ON!\n", ANSI_FG_RED));
#endif

  /* Start engine. */
  engine_start();

  return is_exit_status_bad();
}
