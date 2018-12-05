#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "scheduling_simulator.h"

struct itimerval curr_val, new_val;
void hw_suspend(int msec_10)
{
	return;
}

void hw_wakeup_pid(int pid)
{
	return;
}

int hw_wakeup_taskname(char *task_name)
{
    return 0;
}

int hw_task_create(char *task_name)
{
    return 0; // the pid of created task name
}

void SIGALRM_handler(int sig_num)
{
	printf("received %d\n",sig_num);
//	printf("%ld %ld %ld %ld\n",new_val.it_interval.tv_sec,new_val.it_interval.tv_usec,new_val.it_value.tv_sec,new_val.it_value.tv_usec);
	signal(SIGALRM, SIGALRM_handler);	/*set again to avoid error*/
//
}
void signal_set()
{
	signal(SIGALRM, SIGALRM_handler);	/*deal the SIGALRM signals with the programmer-defined function*/
	new_val.it_interval.tv_sec=1;	/*period time in second*/
	new_val.it_interval.tv_usec=200;	/*period time in micro second 10^-6*/
	new_val.it_value.tv_sec=2;	/*remaining time(only used in the first time) in second*/
	new_val.it_value.tv_usec=500;	/*remaining time(only used in the first time) in second*/
	setitimer(ITIMER_REAL, &new_val, &curr_val);
}
int main()
{
	signal_set();
	while(1){	
		;
	}

	return 0;
}
