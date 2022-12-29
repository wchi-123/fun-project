#include<stdio.h>
#include<stdlib.h>
#include "list.h"

/* 进程状态 */
#define READY "Ready"   // 就绪状态
#define RUN "Run"       // 运行状态
#define WAIT "Wait"     // 阻塞状态
#define FINISH "Finish" // 完成状态
#define NONE "None"   // 就绪状态

#define JOB_NAME_SIZE_MAX (20) // 进程名称长度a
#define JOB_PROCESS_STA_LEN (10) // 进程长度状态
#define JOBNUMBER (5) // 设置进程测试数为5

static int currentTime = 0; // 当前时间，模拟时钟

/* 记录任务执行动作 */
struct task_action{
    int time;// 动作执行时间
    int aid; // 执行动作编号
    int data;// 跳转地址/IO等待时间
    struct list_head actions; // 所有 task_action 结构体组成的链表
};

struct task_struct {
	char jobName[JOB_NAME_SIZE_MAX]; // 作业名
    char jobComm[JOB_NAME_SIZE_MAX]; // 任务名
	int arriveTime; // 到达时间
    int priority; 	// 进程优先级
    float size; 	// 进程大小
    struct task_action *action; // 任务动作
    char processStatus[JOB_PROCESS_STA_LEN]; // 进程状态
    int runTime; // 当前运行时间
    int waitTime; // IO阻塞时间
    int startTime; // 开始时间
    int endTime;   // 完成时间
    int turnoverTime; // 周转时间
    float useWeightTurnoverTime; // 带权周转时间
    struct list_head tasks; // 所有 task_struct 结构体组成的链表
};

struct task_struct *get_task_from_name(char *name, struct task_struct *task){
    struct task_struct *task_tmp=NULL;
    list_for_each_entry(task_tmp, &task->tasks, tasks){
        if(!strncmp(name, task_tmp->jobComm)){
            return task_tmp;
        }
    }
    return NULL;
}

void readProcess(struct task_struct *task)
{
    int arriveTime;    // 到达时间
    int priority; 	   // 任务优先级
    char jobComm[JOB_NAME_SIZE_MAX]; // 任务备注
    char jobName[JOB_NAME_SIZE_MAX]; // 任务名
    struct task_struct *tmp;

    freopen("../Process.txt", "r", stdin);
    while(scanf("%s %d %d %s", jobName, &arriveTime, &priority, jobComm) != EOF){
        tmp= (struct task_struct *)malloc(sizeof(struct task_struct));
        strcpy(tmp->jobName, jobName);
        strcpy(tmp->jobComm, jobComm);
        tmp->arriveTime = arriveTime;
        tmp->priority = priority;
        tmp->startTime = -1;
        tmp->runTime = 0;
        tmp->endTime = 0;
        tmp->turnoverTime = 0;
        tmp->useWeightTurnoverTime = 0.0;
        strcpy(tmp->processStatus, NONE);
        list_add_tail(&tmp->tasks, &task->tasks); // 插入链表
    }
    fclose(stdin);//关闭文件
}

void readProgram(struct task_struct *task){
    struct task_struct *task_curt = NULL;
    struct task_struct *task_temp= NULL;

    char name_sec[JOB_NAME_SIZE_MAX] = {0};
    float size_sec = 0;
    float size = 0;
    int flag = 0;

    /* 遍历program.txt计算程序大小 */
    freopen("../program.txt", "r", stdin);
    while(scanf("%s %f", name_sec, &size_sec) != EOF){
        if(!strncmp(name_sec, "文件名")){ // 将上一次计算的任务大小存入相应结构体中，并重置数据
            if(task_curt != NULL){
                task_curt->size = size;
            }
            size_sec = 0;
            size = 0;
            flag = 0;
            task_curt = NULL;
            continue;
        }

        task_temp = get_task_from_name(name_sec, task); // 根据名称获取相应任务的结构体指针
        if(task_temp != NULL){ // 若该任务存在，则开始记录大小
            task_curt = task_temp;
            flag = 1; 
            continue;
        }
        
        if(flag == 1) {
            size += size_sec;
        }
    }

    if(task_curt != NULL){
        task_curt->size = size;
        task_curt = NULL;
    }
    fclose(stdin);  //关闭文件
}

void readAction(struct task_struct *task){
    int time = 0;
    int id_act = 0;
    int data = 0;
    struct task_action *action = NULL;
    struct task_action *action_temp = NULL;
    struct task_struct *task_curt = NULL;
    struct task_struct *task_temp= NULL;
    char name_act[JOB_NAME_SIZE_MAX] = {0};
    char name_tmp[JOB_NAME_SIZE_MAX] = {0};

    freopen("../run.txt", "r", stdin);
    while(scanf("%s", name_tmp) != EOF){
        task_temp = get_task_from_name(name_tmp, task); // 根据名称获取相应任务的结构体指针
        
        if(task_temp == NULL)
            continue;

        /* 初始化任务动作链表 */
        action = (struct task_action *)malloc(sizeof(struct task_action));
        INIT_LIST_HEAD(&action->actions);

        /* 将任务动作存入链表 */
        while(scanf("%d %s %d",&time, name_act, &data) !=EOF){
            action_temp = (struct task_action *)malloc(sizeof(struct task_action));
            action_temp->time = time;
            action_temp->data = data;
            if(!strcmp(name_act, "跳转"))
                action_temp->aid = 1;
            else if(!strcmp(name_act, "读写磁盘"))
                action_temp->aid = 2;
            else if(!strcmp(name_act, "结束")){
                action_temp->aid = 3;
                list_add_tail(&action_temp->actions, &action->actions);
                break;
            }
            list_add_tail(&action_temp->actions, &action->actions);

            data = 0;
        }
        task_temp->action = action; // 将动作链表存入相应任务结构体中
    }
}

// 计算平均带权周转时间
float weightTurnoverTimeCount(struct task_struct **Mtask)
{
    int i;
    float sum = 0.0;
    for (i = 0; i < JOBNUMBER; i++)
        sum += Mtask[i]->useWeightTurnoverTime;
    return sum / JOBNUMBER;
}
// 计算平均周转时间
float turnOverTimeCount(struct task_struct **Mtask)
{
    int i;
    float sum = 0.0;
    for (i = 0; i < JOBNUMBER; i++)
        sum += Mtask[i]->turnoverTime;
    return sum / JOBNUMBER;
}

void printJob(struct task_struct **Mtask)
{
    int i;
    printf("当前时间为%d\n", currentTime);
    printf("作业号\t到达时间\t已运行时间\t开始时间\t完成时间\t周转时间\t带权周转时间\t阻塞时间\t进程状态\n");
    for (i = 0; i < JOBNUMBER; i++)
    {
        // 如果进程为finish状态，这样输出
        if (strcmp(Mtask[i]->processStatus, FINISH) == 0)
        {
            printf("%s\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%.2f\t\tnone\t\t%s\n",
                   Mtask[i]->jobName, Mtask[i]->arriveTime, Mtask[i]->runTime,
                   Mtask[i]->startTime, Mtask[i]->endTime, Mtask[i]->turnoverTime,
                   Mtask[i]->useWeightTurnoverTime,Mtask[i]->processStatus);
        }
        else if (strcmp(Mtask[i]->processStatus, READY) == 0)
        {
            if(Mtask[i]->startTime == -1)
                printf("%s\t%d\t\t%d\t\t未运行\t\tnone\t\tnone\t\tnone\t\tnone\t\t%s\n",
                    Mtask[i]->jobName, Mtask[i]->arriveTime,
                    Mtask[i]->runTime, Mtask[i]->processStatus);
            // 已经启动但是被抢占
            else
                printf("%s\t%d\t\t%d\t\t%d\t\tnone\t\tnone\t\tnone\t\tnone\t\t%s\n",
                    Mtask[i]->jobName, Mtask[i]->arriveTime,
                    Mtask[i]->runTime, Mtask[i]->startTime,
                    Mtask[i]->processStatus);
        }
        else if (strcmp(Mtask[i]->processStatus, WAIT) == 0)
        {
            printf("%s\t%d\t\t%d\t\t%d\t\tnone\t\tnone\t\tnone\t\t%d\t\t%s\n",
                   Mtask[i]->jobName, Mtask[i]->arriveTime,
                   Mtask[i]->runTime, Mtask[i]->startTime,
                   Mtask[i]->waitTime,Mtask[i]->processStatus);
        }
        else if (strcmp(Mtask[i]->processStatus, RUN) == 0)
        {
            printf("%s\t%d\t\t%d\t\t%d\t\tnone\t\tnone\t\tnone\t\tnone\t\t%s\n",
                Mtask[i]->jobName, Mtask[i]->arriveTime,
                Mtask[i]->runTime, Mtask[i]->startTime,
                Mtask[i]->processStatus);
        }
    }
    
}