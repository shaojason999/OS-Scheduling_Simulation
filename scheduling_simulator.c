#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/time.h>
#include "scheduling_simulator.h"
#ifdef _LP64
 #define STACK_SIZE 2097152+16384
#else
 #define STACK_SIZE 16384
#endif


#define TOTAL_PID 100
#define READY 0
#define RUNNING 1
#define WAITING 2
#define TERMINATED 3

void input_handler(int);

struct PID{
	int priority;	/*0 is low priority*/
	int time;	/*0 is small*/
	int task;	/*task1~task6*/
	int state;	/*see the #define*/
	ucontext_t *ctx;	/*context*/
	/*stack should be large enough!!! because it may call scheduler() and input_handler(), otherwise, it may occur segmentation fault*/
	int stack[STACK_SIZE];	/*assigned to ctx*/
	struct PID *prev,*next;
}*PID_inform[TOTAL_PID];
struct PID *high_queue_cur;	/*the running pid*/
struct PID *low_queue_cur;	/*the prev-running pid*/
struct itimerval old_val,new_val;
int newest_PID,pre_is_empty,running_queue;
ucontext_t *new_ctx,*old_ctx,*terminate_ctx,*back_from_terminate_ctx;
int terminate_stack[STACK_SIZE];

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
	int task_num=task_name[4]-'0';
	if(task_num>6 || task_num<1)
		return -1;
	return newest_PID++; // the pid of created task name
}
void terminate_state()
{
	getcontext(terminate_ctx);

	struct PID *temp;
	pre_is_empty=1;
	if(high_queue_cur!=NULL){
		high_queue_cur->state=TERMINATED;

		if(high_queue_cur->next!=high_queue_cur){
			/*remove the task from queue*/
			temp=high_queue_cur->prev;
			high_queue_cur->prev->next=high_queue_cur->next;
			high_queue_cur->next->prev=temp;
			
			/*change the cur task*/
			high_queue_cur=high_queue_cur->next;
		}else{
			high_queue_cur=NULL;
		}
	}
//	signal(SIGTSTP,input_handler);

	setcontext(back_from_terminate_ctx);
}
void scheduler(int sig_nunm)
{
printf("123\n");
//	signal(SIGALRM, scheduler);	/*set again to avoid error*/
//	signal(SIGTSTP,input_handler);
	if(running_queue==0 && high_queue_cur!=NULL){	/*used when low to high*/
		low_queue_cur=low_queue_cur->next;
		getcontext(low_queue_cur->ctx);
		if(high_queue_cur==NULL)	/*after come back from high, later, when run again this task, keep from here*/
			return;
	}
	if(high_queue_cur!=NULL){
		running_queue=1;
		if(pre_is_empty==0){
			old_ctx=high_queue_cur->ctx;
			high_queue_cur=high_queue_cur->next;	/*step into next task*/
			new_ctx=high_queue_cur->ctx;
			swapcontext(old_ctx,new_ctx);
		}
		else{	/*=1 has two conditions: (1)pre-task is terminated (2)a new start of high_queue or low_queue*/
			pre_is_empty=0;	/*set to 0 before go to taskX()*/
			setcontext(high_queue_cur->ctx);
		}
	}else if(low_queue_cur!=NULL){	/*every time high is finished, pre_is_empty=1*/
		running_queue=0;
		if(pre_is_empty==0){
			old_ctx=low_queue_cur->ctx;
			low_queue_cur=low_queue_cur->next;	/*step into next task*/
			new_ctx=low_queue_cur->ctx;
			swapcontext(old_ctx,new_ctx);
		}
		else{	/*=1 has two conditions: (1)pre-task is terminated (2)a new start of high_queue or low_queue*/
			pre_is_empty=0;	/*set to 0 before go to taskX()*/
			setcontext(low_queue_cur->ctx);
		}
	}
//printf("123\n");

}
/*add the new task to the prev of the running pid, that is, the end of the ready queue*/
void add_ready_queue(int pid)
{
	if(PID_inform[pid]->priority==1){
		if(high_queue_cur==NULL){	/*cur=pid, cur->prev=cur, cur->next=cur */
			high_queue_cur=PID_inform[pid];
			high_queue_cur->next=high_queue_cur;
			high_queue_cur->prev=high_queue_cur;
			pre_is_empty=1;
		}else{	/*insert between cur and cur->prev*/
			PID_inform[pid]->prev=high_queue_cur->prev;
			PID_inform[pid]->next=high_queue_cur;
			high_queue_cur->prev->next=PID_inform[pid];
			high_queue_cur->prev=PID_inform[pid];
		}
	}else if(PID_inform[pid]->priority==0){
		if(low_queue_cur==NULL){
			low_queue_cur=PID_inform[pid];
			low_queue_cur->next=low_queue_cur;
			low_queue_cur->prev=low_queue_cur;
			pre_is_empty=1;
		}else{
			PID_inform[pid]->prev=low_queue_cur->prev;
			PID_inform[pid]->next=low_queue_cur;
			low_queue_cur->prev->next=PID_inform[pid];
			low_queue_cur->prev=PID_inform[pid];
		}
	}
}
void create_a_new_task(int pid)
{
	ucontext_t *ctx;
	ctx=PID_inform[pid]->ctx;
	getcontext(ctx);
	ctx->uc_stack.ss_sp=PID_inform[pid]->stack;
	ctx->uc_stack.ss_size=sizeof(PID_inform[pid]->stack);
//	ctx->uc_stack.ss_flags=0;
	ctx->uc_link=terminate_ctx;
	switch(PID_inform[pid]->task){
		case 1:	
			makecontext(ctx,task1,0);
			break;
		case 2:
			makecontext(ctx,task2,0);
			break;
		case 3:
			makecontext(ctx,task3,0);
			break;
		case 4:
			makecontext(ctx,task4,0);
			break;
		case 5:
			makecontext(ctx,task5,0);
			break;
		case 6:
			makecontext(ctx,task6,0);
			break;
	}
}
void input_handler(int sig_num)
{
int i;
printf("%d\n",&i);
	/*save the timer and then restor it before back to the simulation mode*/
	getitimer(ITIMER_REAL, &old_val);
	/*disable the timer in shell mode*/
	new_val.it_value.tv_sec=0;	/*remaining time(only used in the first time) in second*/
	new_val.it_value.tv_usec=0;	/*remaining time(only used in the first time) in microsecond*/
	setitimer(ITIMER_REAL, &new_val, NULL);
//	signal(SIGTSTP, SIG_IGN);	/*ignore the ctrl+z in input_handler*/
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
			if(pid==-1)
				printf("wrong input, try again\n");
			else{	/*pid set*/
				PID_inform[pid]->priority=priority;
				PID_inform[pid]->time=time;
				PID_inform[pid]->task=task;
				PID_inform[pid]->state=READY;
				PID_inform[pid]->prev=NULL;
				PID_inform[pid]->next=NULL;
				add_ready_queue(pid);
				create_a_new_task(pid);
			}
		}else if(str[0]=='r'){
			scanf("%s",str);
			rmv_pid=atoi(str);
		}else if(str[0]=='p'){
			printf("ps\n");
		/*start the simulation!*/
		}else if(str[0]=='s'){
			printf("start\n");
//			signal(SIGTSTP, input_handler);	/*reset the ctrl+z before return*/
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
	new_val.it_interval.tv_usec=100000;	/*period time in microsecond 10^-6*/
	new_val.it_value.tv_sec=0;	/*remaining time(only used in the first time) in second*/
	new_val.it_value.tv_usec=10000;	/*remaining time(only used in the first time) in microsecond*/
	setitimer(ITIMER_REAL, &new_val, &old_val);
}
void init_variable_set()
{
	int i;
	newest_PID=1;
	running_queue=1;	/*initial set to high*/
	for(i=0;i<TOTAL_PID;++i){
		PID_inform[i]=(struct PID*)malloc(sizeof(struct PID));
		PID_inform[i]->ctx=(ucontext_t*)malloc(sizeof(ucontext_t));
	}
	low_queue_cur=NULL;
	high_queue_cur=NULL;

	terminate_ctx=(ucontext_t*)malloc(sizeof(ucontext_t));
	getcontext(terminate_ctx);
	terminate_ctx->uc_stack.ss_sp=terminate_stack;
	terminate_ctx->uc_stack.ss_size=sizeof(terminate_stack);
	terminate_ctx->uc_link=back_from_terminate_ctx;
	makecontext(terminate_ctx,terminate_state,0);

}
int main()
{
	signal_set();
	init_variable_set();
	input_handler(0);

	pre_is_empty=0;	/*set to 0 before go to taskX()*/
//	getcontext(terminate_ctx);
///*high*/	swapcontext(terminate_ctx,high_queue_cur->ctx);
	swapcontext(back_from_terminate_ctx,high_queue_cur->ctx);
//	terminate_state();
	while(1){
//		pause();
		;
	}
//	pause();

	return 0;
}
