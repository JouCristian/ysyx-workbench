#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "Vtop.h"
#include "verilated.h"
#include "verilated_fst_c.h"

int main(int argc, char **argv){
	VerilatedContext *contextp = new VerilatedContext;
	contextp->commandArgs(argc,argv);
	
	//告诉 Verilator 本次仿真需要记录波形
	Verilated::traceEverOn(true);

	Vtop *top = new Vtop{contextp};
	
	//创建 FST 波形记录对象
	VerilatedFstC *tfp = new VerilatedFstC;
	top->trace(tfp,99);
	tfp->open("wave.fst");
	int i = 0;
	while(i != 20){
		i ++;
		int a = std::rand() & 1;
		int b = std::rand() & 1;
		top->a = a;
		top->b = b;
		top->eval();

		//contextp->time() 返回当前的仿真时间 记录回tfp波形文件
		tfp->dump(contextp->time());
		std::printf("time = %llu, a = %d, b = %d, f = %d\n",static_cast<unsigned long long>(contextp->time()), 
				a,b,static_cast<int>(top->f));
		assert(top->f == (a ^ b));
		contextp->timeInc(1);
	}
	top->final();

	tfp->close();

	delete tfp;
	delete top;
	delete contextp;
	return 0;
}

