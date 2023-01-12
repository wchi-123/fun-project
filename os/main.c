
}#include "read_data.c"

void show_tasks_info(struct task_struct *task){
    struct task_struct *task_tmp = NULL;
    struct task_action *action_temp = NULL;
    printf("name\tstart_time\tpriority\tsize\tpage repalce\n");
    list_for_each_entry(task_tmp, &task->tasks, tasks){
        printf("%s\t%d\t\t%d\t\t%.2fk\t", task_tmp->jobName, task_tmp->arriveTime, task_tmp->priority, task_tmp->size);
        list_for_each_entry(action_temp, &task_tmp->action->actions, actions){
            if(action_temp->aid == 1)
                printf("[%d,%d] ", (action_temp->time), (action_temp->data)-1);
        }
        printf("\n");
    }
    return NULL;
}

void createTasks(struct task_struct *task)
{
    readProcess(task);
    readProgram(task);
    readAction(task);
}

/* 遍历任务队列，符合到达时间的任务加入就绪队列 */
void get_begin_task(struct task_struct *task, struct task_struct *task_ready)
{
    struct task_struct *task_tmp, *next;
    list_for_each_entry_safe(task_tmp, next, &task->tasks, tasks){
        if(task_tmp->arriveTime == currentTime){
            strcpy(task_tmp->processStatus, READY); // 变更任务状态
            list_move_tail(&task_tmp->tasks, &task_ready->tasks); // 加入就绪队列中
        }
    }
}

// 遍历等待队列:唤醒等待时间为0的任务，将其加入就绪队列
void try_to_wake_up(struct task_struct *task_wait, struct task_struct *task_ready)
{
    int ret = 0;
    struct task_struct *task_tmp, *next;
    list_for_each_entry_safe(task_tmp, next, &task_wait->tasks, tasks){
        if(task_tmp->waitTime == 0){
            strcpy(task_tmp->processStatus, READY); // 变更任务状态
            list_move_tail(&task_tmp->tasks, &task_ready->tasks); // 加入就绪队列中
        }
    }
}

/* 优先级调度算法 */
struct task_struct *prioritySchedule(struct task_struct *task_ready, struct task_struct **task_run)
{
    int priority = -1;
    /* 若运行队列进程状态不是运行态，则一定会发生调度 */
    if(*task_run != NULL)
        if(!strcmp((*task_run)->processStatus, RUN))
            priority = (*task_run)->priority;
    
    struct task_struct *task_tmp, *task_pick = NULL;
    /* 遍历绪队列，判断是否有高优先级任务抢占 */
    list_for_each_entry(task_tmp, &task_ready->tasks, tasks){
        if(task_tmp->priority > priority){  // 优先级相同则选择先进入就绪队列的任务
            priority = task_tmp->priority;
            task_pick = task_tmp;
        }
    }
    return task_pick;
}

/* 先来先服务调度算法 */
struct task_struct *firstComeFirstServed(struct task_struct *task_ready, struct task_struct **task_run)
{
    struct task_struct *task_tmp, *task_pick = NULL;
    /* 若运行队列进程状态是运行态，则不进行调度，返回空 */
    if(*task_run != NULL)
        if(!strcmp((*task_run)->processStatus, RUN))
            goto out;
    
    /* 遍历绪队列，判断是否有高优先级任务抢占 */
    list_for_each_entry(task_tmp, &task_ready->tasks, tasks){
        task_pick = task_tmp;
        goto out;
    }
out:
    return task_pick;
}

/* 时间片轮转调度 */
struct task_struct *roundRobinTimeout(struct task_struct *task_ready, struct task_struct **task_run)
{
    struct task_struct *task_tmp, *task_pick = NULL;
    /* 当前运行进程时间片未消耗完 */
    if(*task_run != NULL)
        if((*task_run)->time_slice != 0)
            return task_pick;
    
    /* 从就绪队列中返回下一个进程 */
    list_for_each_entry(task_tmp, &task_ready->tasks, tasks){
        task_pick = task_tmp;
        break;
    }
    return task_pick;
}

/* 调度器，从就绪队列中选择一个任务加入运行队列 */
void schedule(struct task_struct *task_ready, struct task_struct **task_run)
{
    struct task_struct *task_pick = NULL;  
    /* 找到需要抢占的进程 */
    if(n_sche == 1)
        task_pick = firstComeFirstServed(task_ready, task_run);
    else   
        task_pick = roundRobinTimeout(task_ready, task_run);
    
    /* 无需调度，继续执行之前的进程 */
    if(task_pick == NULL)
        return;
    
    /* 将当前运行的进程更改为就绪态 */
    if(*task_run != NULL)
        if(!strcmp((*task_run)->processStatus, RUN)){
            strcpy((*task_run)->processStatus, READY);
            list_add_tail(&(*task_run)->tasks, &task_ready->tasks);
        }
        
    /* 修改抢占进程的状态，并加入运行队列 */
    list_del(&task_pick->tasks);
    if(task_pick->startTime == -1)
        task_pick->startTime = currentTime;
    strcpy(task_pick->processStatus, RUN);
    if(n_sche == 2)
        task_pick->time_slice = time_slice;
    (*task_run) = task_pick;
}

/* 任务执行，消耗时间,返回是否请求调度 */
void run(struct task_struct **Mtask, struct task_struct *task_run, struct task_struct *task_wait)
{
    int i;
    struct task_action *action_temp = NULL;
    
    for (i = 0; i < JOBNUMBER; i++)
    {
        if(!strcmp(Mtask[i]->processStatus, RUN)){  //正在运行
            Mtask[i]->runTime += 1;//运行时间+1
            Mtask[i]->serverTime -= 1;//剩余服务时间-1
            if(n_sche == 2) //时间片轮转
                Mtask[i]->time_slice -= 1; //时间片-1
        }
        if(!strcmp(Mtask[i]->processStatus, WAIT)) //阻塞状态
            Mtask[i]->waitTime -= 1; //阻塞时间-1
    }
    
    if(task_run == NULL)
        return;
    if(strcmp(task_run->processStatus, RUN))
        return;
    
    // 遍历运行态任务的动作
    list_for_each_entry(action_temp, &task_run->action->actions, actions){  
        /* 判断当前时间是否有动作执行 */
        if(action_temp->time == task_run->runTime){ 
            if( action_temp->aid == 1){ // 页面切换
                pageReplace(task_run, action_temp->data);
            } else if( action_temp->aid == 2){ // 触发IO请求，进入阻塞态
                strcpy(task_run->processStatus, WAIT);
                task_run->waitTime = action_temp->data; // 记录阻塞时间
                if(n_sche == 2)
                    task_run->time_slice = 0;
                list_add_tail(&task_run->tasks, &task_wait->tasks); // 加入到阻塞队列中
            } else if(action_temp->aid == 3){ // 任务完成
                strcpy(task_run->processStatus, FINISH);
                task_run->endTime = currentTime;
                task_run->turnoverTime = task_run->endTime - task_run->arriveTime;
                task_run->useWeightTurnoverTime = task_run->turnoverTime * 1.0 / task_run->runTime;
                if(n_sche == 2)
                    task_run->time_slice = 0;
            }
        }
    }
}

void FIFO(struct task_struct *task_run, int page) //先进先出置换算法
{
    int j;
    int count = task_run->page_count;
    
    /* 在物理块中查看该页是否已经存在 */
    for (j = 0; j < frame_count; j++) {
        if (page == task_run->frame[j].num) // 已经存在则无需置换，直接返回
            return;
    }

    /* 进程页面还没有存满，直接存入 */
    if (count < frame_count) 
        task_run->frame[count].num =  page;
    else
        task_run->frame[count % frame_count].num = page; //换掉最旧的页
    
    task_run->page_count += 1;
}
//0 1
//count=2    2%2
void LRU(struct task_struct *task_run, int page) //最久未使用置换算法
{
    int i, j, m, n;
    int count = task_run->page_count;

    /* 在物理块中查看该页是否已经存在 */
    for (j = 0; j < frame_count; j++) {
        if (page == task_run->frame[j].num){
            task_run->frame[j].mark = currentTime; // 更新页面时间戳
            return;
        }
    }

    /* 进程页面还没有存满，直接存入 */
    if (count < frame_count){
        task_run->frame[count].num =  page;
        task_run->frame[j].mark = currentTime; // 初始化页面时间戳
        task_run->page_count += 1;
    } else {
        m = task_run->frame[0].mark,n = 0;
        for (i = 1; i < frame_count; i++) {
            if(task_run->frame[i].mark < m){
                m = task_run->frame[i].mark; // 找到时间戳最小的页面
                n = i;
            }
        }
        task_run->frame[n].num = page;
        task_run->frame[n].mark = currentTime;
    }
}

void pageReplace(struct task_struct *task_run, int page)
{
    if(n_page == 1)
        FIFO(task_run, page);
    else if (n_page == 2)
        LRU(task_run, page);
}

void indexTask(struct task_struct *task, struct task_struct **Mtask)
{
    int i = 0;
    struct task_struct *task_tmp;
    list_for_each_entry(task_tmp, &task->tasks, tasks){
        Mtask[i] = task_tmp;
        i++;
    }
}

void showReady(struct task_struct *task)
{
    printf("就绪队列：");
    struct task_struct *task_tmp=NULL;
    list_for_each_entry(task_tmp, &task->tasks, tasks){
        printf("%s ",task_tmp->jobName);
    }
    printf("\n");
}

void showWait(struct task_struct *task)
{
    printf("阻塞队列：");
    struct task_struct *task_tmp=NULL;
    list_for_each_entry(task_tmp, &task->tasks, tasks){
        printf("%s ",task_tmp->jobName);
    }
    printf("\n");
}

int isFinish(struct task_struct **Mtask)
{
    int i;
    for (i = 0; i < JOBNUMBER; i++){
        if(strcmp(Mtask[i]->processStatus, FINISH))
            return 0;
    }
    return 1;
}

int main(){
    int ret = 0, i = 0;
    printf("请选择进程调度算法:\n1、先来先服务\t2、时间片轮转法\n");
    scanf("%d",&n_sche);
    if(n_sche == 2){
        printf("请输入时间片大小：");
        scanf("%d",&time_slice);
    }
    printf("请选择页面置换算法:\n1、先进先出(FIFO)\t2、最近最久未使用(LRU)\n");
    scanf("%d",&n_page);
    printf("请输入页面大小(B)，进程分配页数:\n");
    scanf("%d %d",&frame_size, &frame_count);
    
    /* 任务队列、运行队列、就绪队列、等待队列 */
    struct task_struct *task = (struct task_struct *)malloc(sizeof(struct task_struct));
    struct task_struct *task_ready = (struct task_struct *)malloc(sizeof(struct task_struct));
    struct task_struct *task_wait = (struct task_struct *)malloc(sizeof(struct task_struct));
    struct task_struct *task_run = NULL;
    struct task_struct *Mtask[JOBNUMBER]; // 管理所有进程的指针数组

    /* 初始化链表 */
    INIT_LIST_HEAD(&task->tasks);
    INIT_LIST_HEAD(&task_ready->tasks);
    INIT_LIST_HEAD(&task_wait->tasks);
    
    createTasks(task);
    show_tasks_info(task);
    indexTask(task, Mtask);

    printf("时间\t运行的进程\t开始时间\t运行时间\t剩余服务时间\t进程A状态\t进程B状态\t进程C状态\t进程D状态\t进程E状态\t");
    for (i = 0; i < JOBNUMBER; i++)
        printf("%s页面\t",Mtask[i]->jobName);
    printf("\n");
    /* 模拟时钟 */
    while(1) {
        get_begin_task(task, task_ready); // 将新到达的任务加入就绪队列
        run(Mtask, task_run, task_wait);  // 任务执行，消耗时间, 可能发生任务阻塞以及内存调度
        try_to_wake_up(task_wait, task_ready); // 唤醒等待时间为0的任务
        schedule(task_ready, &task_run); // 轮询调度    
//        if(currentTime != 0)
            printJob(Mtask,task_run);
        if(isFinish(Mtask)){
            break;
        }
        currentTime++;
    }
    printJob2(Mtask);
    return 0;