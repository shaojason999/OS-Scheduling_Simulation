#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "scheduling_simulator.h"

#define TOTAL_PID 100
#define READY 0
#define RUNNING 1
#define WAITING 2
#define TERMINATED 3

struct itimerval old_val, cur_val, new_val;
int newest_PID;
struct PID{
	int priority;	/*0 is low priority*/
	int time;	/*0 is small*/
	int task;	/*task1~task6*/
	int status;	/*see the #define*/
	struct PID *prev,*next;
}*PID_inform[TOTAL_PID];
struct PID *high_queue_cur,*low_queue_cur;	/*the running pid*/

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
    return newest_PID++; // the pid of created task name
}
/*add the new task to the prev of the running pid, that is, the end of the ready queue*/
void add_ready_queue(int pid)
{
	if(PID_inform[pid]->priority==1){
		if(high_queue_cur==NULL){	/*cur=pid, cur->prev=cur, cur->next=cur */
			high_queue_cur=PID_inform[pid];
			high_queue_cur->next=high_queue_cur;
			high_queue_cur->prev=high_queue_cur;
		}else{	/*insert between cur and cur->prev*/
			high_queue_cur->prev->next=PID_inform[pid];
			PID_inform[pid]->prev=high_queue_cur->prev;
			PID_inform[pid]->next=high_queue_cur;
			high_queue_cur->prev=PID_inform[pid];
		}
	}else if(PID_inform[pid]->priority==0){
		if(low_queue_cur==NULL){
			low_queue_cur=PID_inform[pid];
			low_queue_cur->next=low_queue_cur;
			low_queue_cur->prev=low_queue_cur;
		}else{
			low_queue_cur->prev->next=PID_inform[pid];
			PID_inform[pid]->prev=low_queue_cur->prev;
			PID_inform[pid]->next=low_queue_cur;
			low_queue_cur->prev=PID_inform[pid];
		}
	}
}
void scheduler(int sig_num)
{
	


//	printf("received %d\n",sig_num);
//	printf("%ld %ld %ld %ld\n",new_val.it_interval.tv_sec,new_val.it_interval.tv_usec,new_val.it_value.tv_sec,new_val.it_value.tv_usec);
	signal(SIGALRM, scheduler);	/*set again to avoid error*/
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
	char str[100],task_name[10];
	int task,time,priority,pid,rmv_pid;
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
			memset(task_name,0,sizeof(task_name));
			sprintf(task_name,"task%d",task);
			pid=hw_task_create(task_name);
			/*pid set*/
			PID_inform[pid]->priority=priority;
			PID_inform[pid]->time=time;
			PID_inform[pid]->task=task;
			PID_inform[pid]->status=READY;
			PID_inform[pid]->prev=NULL;
			PID_inform[pid]->next=NULL;
			add_ready_queue(pid);
		}else if(str[0]=='r'){
			scanf("%s",str);
			rmv_pid=atoi(str);
		}else if(str[0]=='p'){
			printf("ps\n");
		}else if(str[0]=='s'){
			printf("start\n");
			signal(SIGTSTP, input_handler);	/*reset the ctrl+z before return*/
			setitimer(ITIMER_REAL, &old_val, NULL);	/*restore the timer before back to simulation mode*/
			return;
		}else
			printf("wrong input, try again\n");
printf("task:%d time:%d priority:%d rmv_pid:%d\n",task,time,priority,rmv_pid);
	}
}
void signal_set()
{
	signal(SIGTSTP, input_handler);	/*ctrl+z to input_handler*/
	signal(SIGALRM, scheduler);	/*deal the SIGALRM signals with the programmer-defined function*/
	new_val.it_interval.tv_sec=0;	/*period time in second*/
	new_val.it_interval.tv_usec=10000;	/*period time in microsecond 10^-6*/
	new_val.it_value.tv_sec=0;	/*remaining time(only used in the first time) in second*/
	new_val.it_value.tv_usec=10000;	/*remaining time(only used in the first time) in microsecond*/
	setitimer(ITIMER_REAL, &new_val, &old_val);
}
void init_variable_set()
{
	int i;
	newest_PID=1;
	for(i=0;i<TOTAL_PID;++i)
		PID_inform[i]=malloc(sizeof(struct PID));
	low_queue_cur=NULL;
	high_queue_cur=NULL;
}
int main()
{
	signal_set();
	init_variable_set();
	input_handler();
	while(1){

		;
	}

	return 0;
}
