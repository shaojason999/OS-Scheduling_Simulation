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
#define REMOVED 4

void add_ready_queue(int pid);
void input_handler(int);
void create_a_new_task(int pid);

struct PID {
    int priority;	/*0 is low priority*/
    int time;	/*0 is small*/
    int task;	/*task1~task6*/
    int pid;
    int state;	/*see the #define*/
    int queueing_time;
    int remaining_time;	/*1 or 0*/
    int waiting_time;	/*remaining time in waiting queue*/
    ucontext_t *ctx;	/*context*/
    /*stack should be large enough!!! because it may call scheduler() and input_handler(), otherwise, it may occur segmentation fault*/
    char stack[STACK_SIZE];	/*assigned to ctx*/
    struct PID *prev,*next;
}*PID_inform[TOTAL_PID];
struct PID *high_queue_cur;	/*the running pid*/
struct PID *low_queue_cur;	/*the prev-running pid*/
struct itimerval old_val,new_val;
int create_in_main,pid,newest_PID,running_queue;
int tasks_is_waiting;
ucontext_t *new_ctx,*old_ctx,*terminate_ctx;
ucontext_t *back_from_terminate_ctx,*back_from_main_ctx,*create_task_in_main_ctx;
char terminate_stack[STACK_SIZE];

void hw_suspend(int msec_10)
{
    tasks_is_waiting++;
    ucontext_t *temp;
    if(running_queue==1) {
        temp=high_queue_cur->ctx;
        high_queue_cur->waiting_time=msec_10;
        high_queue_cur->state=WAITING;
        if(high_queue_cur->next==high_queue_cur) {
            high_queue_cur=NULL;
            swapcontext(temp,back_from_terminate_ctx);
        } else {
            high_queue_cur->prev->next=high_queue_cur->next;
            high_queue_cur->next->prev=high_queue_cur->prev;
            high_queue_cur=high_queue_cur->next;
            swapcontext(temp,high_queue_cur->ctx);
        }
    } else {
        temp=low_queue_cur->ctx;
        low_queue_cur->waiting_time=msec_10;
        low_queue_cur->state=WAITING;
        if(low_queue_cur->next==low_queue_cur) {
            low_queue_cur=NULL;
            swapcontext(temp,back_from_terminate_ctx);
        } else {
            low_queue_cur->prev->next=low_queue_cur->next;
            low_queue_cur->next->prev=low_queue_cur->prev;
            low_queue_cur=low_queue_cur->next;
            swapcontext(temp,low_queue_cur->ctx);
        }
    }
    return;
}

void hw_wakeup_pid(int pid)
{
    if(PID_inform[pid]->state==WAITING) {
        PID_inform[pid]->waiting_time=0;
        PID_inform[pid]->state=READY;
        add_ready_queue(pid);
        tasks_is_waiting--;
    }
    return;
}

int hw_wakeup_taskname(char *task_name)
{
    int i,j=0;
    for(i=1; i<newest_PID; ++i) {
        if(PID_inform[i]->task==3 && PID_inform[i]->state==WAITING) {
            PID_inform[i]->waiting_time=0;
            PID_inform[i]->state=READY;
            add_ready_queue(i);
            tasks_is_waiting--;
            ++j;
        }
    }
    return j;
}

int hw_task_create(char *task_name)
{
    int task_num=task_name[4]-'0';
    if(task_num>6 || task_num<1)
        return -1;
    if((running_queue==1 && high_queue_cur!=NULL && (high_queue_cur->task==5 || high_queue_cur->task==6)) || (running_queue==0 && low_queue_cur!=NULL && (low_queue_cur->task==5 || low_queue_cur->task==6))) {
        pid=newest_PID;
        PID_inform[pid]->priority=0;
        PID_inform[pid]->time=0;
        PID_inform[pid]->remaining_time=0;
        PID_inform[pid]->task=task_num;
        PID_inform[pid]->pid=pid;
        PID_inform[pid]->state=READY;
        PID_inform[pid]->queueing_time=0;
        PID_inform[pid]->prev=NULL;
        PID_inform[pid]->next=NULL;
        add_ready_queue(pid);

        swapcontext(back_from_main_ctx,create_task_in_main_ctx);
    }
    return newest_PID++; // the pid of created task name
}
void terminate_state()
{
    int i;
    getcontext(terminate_ctx);
    /*because if will run next task immediately later, add queueing_time here*/
    for(i=1; i<newest_PID; ++i) {
        if(PID_inform[i]->state==READY)
            PID_inform[i]->queueing_time++;
    }
    if(high_queue_cur!=NULL) {
        high_queue_cur->state=TERMINATED;

        if(high_queue_cur->next!=high_queue_cur) {
            /*remove the task from queue*/
            high_queue_cur->prev->next=high_queue_cur->next;
            high_queue_cur->next->prev=high_queue_cur->prev;

            /*change the cur task*/
            high_queue_cur=high_queue_cur->next;
        } else {
            high_queue_cur=NULL;
        }
    } else if(low_queue_cur!=NULL) {
        low_queue_cur->state=TERMINATED;

        if(low_queue_cur->next!=low_queue_cur) {
            /*remove the task from queue*/
            low_queue_cur->prev->next=low_queue_cur->next;
            low_queue_cur->next->prev=low_queue_cur->prev;

            /*change the cur task*/
            low_queue_cur=low_queue_cur->next;
        } else {
            low_queue_cur=NULL;
        }
    }

    if(high_queue_cur!=NULL) {
        high_queue_cur->state=RUNNING;
        running_queue=1;
        setcontext(high_queue_cur->ctx);
    } else if(low_queue_cur!=NULL) {
        low_queue_cur->state=RUNNING;
        running_queue=0;
        setcontext(low_queue_cur->ctx);
    } else {
        setcontext(back_from_terminate_ctx);
    }
}
void scheduler(int sig_nunm)
{
    int i;
    for(i=1; i<newest_PID; ++i) {
        if(PID_inform[i]->state==READY)
            PID_inform[i]->queueing_time++;
    }
    if(tasks_is_waiting) {
        for(i=1; i<newest_PID; ++i) {
            if(PID_inform[i]->state==WAITING) {
                PID_inform[i]->waiting_time--;
                if(PID_inform[i]->waiting_time==0) {
                    PID_inform[i]->state=READY;
                    add_ready_queue(i);
                    tasks_is_waiting--;
                }
            }
        }
    }
    /*used when from low to high*/
    if(running_queue==0 && high_queue_cur!=NULL) {
        low_queue_cur->state=READY;
        getcontext(low_queue_cur->ctx);
        if(high_queue_cur==NULL) {
            low_queue_cur->state=RUNNING;
            running_queue=0;
            return;
        }
        high_queue_cur->state=RUNNING;
        running_queue=1;
        setcontext(high_queue_cur->ctx);
    }
    if(high_queue_cur!=NULL) {
        if(high_queue_cur->remaining_time==1) {
            high_queue_cur->remaining_time=0;
            return;
        }
        /*step into next task*/
        high_queue_cur->state=READY;
        running_queue=1;
        old_ctx=high_queue_cur->ctx;
        high_queue_cur=high_queue_cur->next;
        new_ctx=high_queue_cur->ctx;
        high_queue_cur->state=RUNNING;

        if(high_queue_cur->time==1)
            high_queue_cur->remaining_time=1;

        swapcontext(old_ctx,new_ctx);
    } else if(low_queue_cur!=NULL) {	/*every time high is finished, pre_is_empty=1*/
        if(low_queue_cur->remaining_time==1) {
            low_queue_cur->remaining_time=0;
            return;
        }

        low_queue_cur->state=READY;
        running_queue=0;
        old_ctx=low_queue_cur->ctx;
        low_queue_cur=low_queue_cur->next;	/*step into next task*/
        new_ctx=low_queue_cur->ctx;
        low_queue_cur->state=RUNNING;

        if(low_queue_cur->time==1)
            low_queue_cur->remaining_time=1;

        swapcontext(old_ctx,new_ctx);
    } else /*both are NULL(caused by removed)*/
        setcontext(back_from_terminate_ctx);
}
void add_ready_queue(int pid)
{
    if(PID_inform[pid]->priority==1) {
        if(high_queue_cur==NULL) {	/*cur=pid, cur->prev=cur, cur->next=cur */
            high_queue_cur=PID_inform[pid];
            high_queue_cur->next=high_queue_cur;
            high_queue_cur->prev=high_queue_cur;
            /*add the new task to the prev of the running pid, that is, the end of the ready queue*/
        } else {	/*insert between cur and cur->prev*/
            PID_inform[pid]->prev=high_queue_cur->prev;
            PID_inform[pid]->next=high_queue_cur;
            high_queue_cur->prev->next=PID_inform[pid];
            high_queue_cur->prev=PID_inform[pid];
        }
    } else if(PID_inform[pid]->priority==0) {
        if(low_queue_cur==NULL) {
            low_queue_cur=PID_inform[pid];
            low_queue_cur->next=low_queue_cur;
            low_queue_cur->prev=low_queue_cur;
        } else {
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
    ctx->uc_link=terminate_ctx;
    switch(PID_inform[pid]->task) {
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
    /*save the timer and then restor it before back to the simulation mode*/
    getitimer(ITIMER_REAL, &old_val);
    /*disable the timer in shell mode*/
    new_val.it_value.tv_sec=0;	/*remaining time(only used in the first time) in second*/
    new_val.it_value.tv_usec=0;	/*remaining time(only used in the first time) in microsecond*/
    setitimer(ITIMER_REAL, &new_val, NULL);
    char str[100],task_name[100];
    int task,time,priority,rmv_pid;
    while(1) {
        printf("$ ");
        memset(str,0,sizeof(str));
        scanf("%s",str);
        if(str[0]=='a') {
            time=0, priority=0;	/*default*/
            fgets(str,2,stdin);	/*discard one space(2=one space+one terminating null-char)*/
            fgets(str,sizeof(str),stdin);	/*the rest of add command*/
            task=str[4]-'0';
            if(strlen(str)>=6) {
                if(str[7]=='t') {
                    if(str[9]=='L')
                        time=1;	/*time=1 is large*/
                    else if(str[9]=='S')
                        time=0;	/*time=0 is small*/
                } else if(str[7]=='p') {
                    if(str[9]=='H')
                        priority=1;	/*priority=1 is high*/
                    else if(str[9]=='L')
                        priority=0;	/*priority=0 is low*/
                }
            }
            if(strlen(str)>=11) {
                if(str[12]=='t') {
                    if(str[14]=='L')
                        time=1;	/*time=1 is large*/
                    else if(str[14]=='S')
                        time=0;	/*time=0 is small*/
                } else if(str[12]=='p') {
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
            else {	/*pid set*/
                PID_inform[pid]->priority=priority;
                PID_inform[pid]->time=time;
                if(time==1 && task!=3)
                    PID_inform[pid]->remaining_time=1;
                else
                    PID_inform[pid]->remaining_time=0;
                PID_inform[pid]->task=task;
                PID_inform[pid]->pid=pid;
                PID_inform[pid]->state=READY;
                PID_inform[pid]->queueing_time=0;
                PID_inform[pid]->prev=NULL;
                PID_inform[pid]->next=NULL;
                add_ready_queue(pid);
                /*create here when there is no task in queue*/
                if(create_in_main==0)
                    create_a_new_task(pid);
                /*create in main when there is any task in queue*/
                else
                    swapcontext(back_from_main_ctx,create_task_in_main_ctx);
            }
        } else if(str[0]=='r') {
            scanf("%s",str);
            rmv_pid=atoi(str);
            if(rmv_pid<newest_PID && rmv_pid>0 && PID_inform[rmv_pid]->state!=REMOVED) {
                PID_inform[rmv_pid]->state=REMOVED;
                if(PID_inform[rmv_pid]->next == PID_inform[rmv_pid]) {
                    if(PID_inform[rmv_pid]->priority==1)
                        high_queue_cur=NULL;
                    else
                        low_queue_cur=NULL;
                } else {
                    PID_inform[rmv_pid]->prev->next=PID_inform[rmv_pid]->next;
                    PID_inform[rmv_pid]->next->prev=PID_inform[rmv_pid]->prev;

                    if(PID_inform[rmv_pid]->priority==1 && high_queue_cur->pid==rmv_pid)
                        high_queue_cur=high_queue_cur->next;
                    else if(PID_inform[rmv_pid]->priority==0 && low_queue_cur->pid==rmv_pid)
                        low_queue_cur=low_queue_cur->next;
                }
            } else
                printf("wrong pid or already removed\n");
        } else if(str[0]=='p') {
            int i;
            for(i=1; i<newest_PID; ++i) {
                if(PID_inform[i]->state==READY)
                    printf("%d\ttask%d\tTASK_READY\t%dms\t\t",i,PID_inform[i]->task,PID_inform[i]->queueing_time*10);
                else if(PID_inform[i]->state==RUNNING)
                    printf("%d\ttask%d\tTASK_RUNNING\t%dms\t\t",i,PID_inform[i]->task,PID_inform[i]->queueing_time*10);
                else if(PID_inform[i]->state==WAITING)
                    printf("%d\ttask%d\tTASK_WAITING\t%dms\t\t",i,PID_inform[i]->task,PID_inform[i]->queueing_time*10);
                else if(PID_inform[i]->state==TERMINATED)
                    printf("%d\ttask%d\tTASK_TERMINATED\t%dms\t\t",i,PID_inform[i]->task,PID_inform[i]->queueing_time*10);
                else/* if(PID_inform[i]->state==REMOVED)*/
                    printf("%d\ttask%d\tTASK_REMOVED\t%dms\t\t",i,PID_inform[i]->task,PID_inform[i]->queueing_time*10);
                if(PID_inform[i]->priority==1)
                    printf("H\t");
                else/* if(PID_inform[i]->priority==0)*/
                    printf("L\t");
                if(PID_inform[i]->time==1)
                    printf("L\n");
                else/* if(PID_inform[i]->time==0)*/
                    printf("S\n");
            }
            /*start the simulation!*/
        } else if(str[0]=='s') {
            /*restore the timer before back to simulation mode*/
            setitimer(ITIMER_REAL, &old_val, NULL);
            if(high_queue_cur==NULL && low_queue_cur==NULL)
                continue;
            create_in_main=1;
            printf("simulating...\n");
            return;
        } else
            printf("wrong input, try again\n");
        /*printf("task:%d time:%d priority:%d pid:%d\n",task,time,priority,pid);*/
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
    create_in_main=0;
    newest_PID=1;
    running_queue=1;	/*initial set to high*/
    tasks_is_waiting=0;
    for(i=0; i<TOTAL_PID; ++i) {
        PID_inform[i]=(struct PID*)malloc(sizeof(struct PID));
        PID_inform[i]->ctx=(ucontext_t*)malloc(sizeof(ucontext_t));
    }
    low_queue_cur=NULL;
    high_queue_cur=NULL;

    terminate_ctx=(ucontext_t*)malloc(sizeof(ucontext_t));
    back_from_terminate_ctx=(ucontext_t*)malloc(sizeof(ucontext_t));
    back_from_main_ctx=(ucontext_t*)malloc(sizeof(ucontext_t));
    create_task_in_main_ctx=(ucontext_t*)malloc(sizeof(ucontext_t));
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
    getcontext(create_task_in_main_ctx);
    /*create here when there is any task in queue*/
    if(high_queue_cur!=NULL || low_queue_cur!=NULL) {
        create_a_new_task(pid);
        setcontext(back_from_main_ctx);
    }
    /*only when there is at least one task, it would return*/
    input_handler(0);
    while(1) {
        if(high_queue_cur!=NULL) {
            high_queue_cur->state=RUNNING;
            running_queue=1;
            swapcontext(back_from_terminate_ctx,high_queue_cur->ctx);
        } else if(low_queue_cur!=NULL) {
            low_queue_cur->state=RUNNING;
            running_queue=0;
            swapcontext(back_from_terminate_ctx,low_queue_cur->ctx);
        } else if(tasks_is_waiting>0) {
            ;
        } else {
            create_in_main=0;
            input_handler(0);
        }
    }
    return 0;
}
