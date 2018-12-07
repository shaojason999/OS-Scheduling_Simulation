#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "scheduling_simulator.h"

struct itimerval old_val, cur_val, new_val;
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
//	printf("received %d\n",sig_num);
//	printf("%ld %ld %ld %ld\n",new_val.it_interval.tv_sec,new_val.it_interval.tv_usec,new_val.it_value.tv_sec,new_val.it_value.tv_usec);
	signal(SIGALRM, SIGALRM_handler);	/*set again to avoid error*/
}
void input_handler()
{
	/*save the timer and then restor it before back to the simulation mode*/
	getitimer(ITIMER_REAL, &old_val);
	/*disable the timer in shell mode*/
	new_val.it_value.tv_sec=0;	/*remaining time(only used in the first time) in second*/
	new_val.it_value.tv_usec=0;	/*remaining time(only used in the first time) in microsecond*/
	setitimer(ITIMER_REAL, &new_val, NULL);

	signal(SIGTSTP, SIG_IGN);	/*ignore the ctrl+z in input_handler*/
	char str[100];
	int task,time,priority,pid;
	while(1){
		memset(str,0,sizeof(str));
		scanf("%s",str);
		if(str[0]=='a'){
			time=0, priority=0;	/*default*/
			fgets(str,2,stdin);	/*discard one space(2=one space+one terminating null-char)*/
			fgets(str,sizeof(str),stdin);	/*the rest of add command*/
			task=str[4]-'0';
			if(strlen(str)>=6){
				if(str[7]=='t'){
					if(str[9]=='L')
						time=1;	/*time=1 is large*/
					else if(str[9]=='S')
						time=0;	/*time=0 is small*/
				}else if(str[7]=='p'){
					if(str[9]=='H')
						priority=1;	/*priority=1 is high*/
					else if(str[9]=='L')
						priority=0;	/*priority=0 is low*/
				}
			}
			if(strlen(str)>=11){
				if(str[12]=='t'){
					if(str[14]=='L')
						time=1;	/*time=1 is large*/
					else if(str[14]=='S')
						time=0;	/*time=0 is small*/
				}else if(str[12]=='p'){
					if(str[14]=='H')
						priority=1;	/*priority=1 is high*/
					else if(str[14]=='L')
						priority=0;	/*priority=0 is low*/
				}
			}
		}else if(str[0]=='r'){
			scanf("%s",str);
			pid=atoi(str);
		}else if(str[0]=='p'){
			printf("ps\n");
		}else if(str[0]=='s'){
			printf("start\n");
			signal(SIGTSTP, input_handler);	/*reset the ctrl+z before return*/
			setitimer(ITIMER_REAL, &old_val, NULL);	/*restore the timer before back to simulation mode*/
			return;
		}else
			printf("wrong input, try again\n");
printf("task:%d time:%d priority:%d pid:%d\n",task,time,priority,pid);
	}
}
void signal_set()
{
	signal(SIGTSTP, input_handler);
	signal(SIGALRM, SIGALRM_handler);	/*deal the SIGALRM signals with the programmer-defined function*/
	new_val.it_interval.tv_sec=0;	/*period time in second*/
	new_val.it_interval.tv_usec=10000;	/*period time in microsecond 10^-6*/
	new_val.it_value.tv_sec=0;	/*remaining time(only used in the first time) in second*/
	new_val.it_value.tv_usec=10000;	/*remaining time(only used in the first time) in microsecond*/
	setitimer(ITIMER_REAL, &new_val, &old_val);
}
int main()
{
	signal_set();
	input_handler();
	while(1){

		;
	}

	return 0;
}
