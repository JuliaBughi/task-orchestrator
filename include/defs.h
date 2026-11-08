#ifndef DEFS_H
#define DEFS_H

#define CLIENT "./tmp/fifo_client"
#define ORCHESTRATOR "./tmp/fifo_orchestrator"
#define MESSAGE_TO_USER_MAX_SIZE 350

typedef enum Options{
    EXECUTE,
    STATUS,
    CLOSE,
    FINISHED_TASK,
    FINISHED_STATUS
} Opts;

typedef enum Schedule_Policy{
    SJF,
    FCFS
} SchedulePolicy;

typedef struct Message{
    int pid;
    int expectedTime;
    Opts option;
    char command[300];
} Message;

typedef struct Task{
    int pid;
    int expectedTime;
    struct timeval initial_time;
    char command[300];
} Task;

typedef struct Task_Logs{
    int pid;
    char command[300];
    double time;
} Logs;

#endif