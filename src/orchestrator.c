#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <glib.h>
#include "defs.h"
#include "orchestrator.h"

char*** parse_command(char* cmd, int* n_cmds) {
    int max_args_temp = MAX_ARGS_INITIAL, maxCmds = MAX_CMDS_INITIAL;
    int args_count = 0, cmd_count = 0;
    char*** exec_args = malloc(sizeof(char*) * maxCmds);
    char* string;

    for(int i = 0; i < maxCmds; i++){
        exec_args[i] = malloc(sizeof(char*) * max_args_temp);
    }

    while((string = strsep(&cmd, " ")) != NULL){
        if(strcmp(string, "|") == 0){
            exec_args[cmd_count][args_count] = NULL;
            cmd_count++;
            args_count = 0;
            max_args_temp = MAX_ARGS_INITIAL;

            if(cmd_count >= maxCmds){
                maxCmds *= 2;
                exec_args = realloc(exec_args, sizeof(char*) * maxCmds);
            }
        }
        else{
            exec_args[cmd_count][args_count] = string;
            args_count++;

            if(args_count >= max_args_temp){
                max_args_temp *= 2;
                exec_args[cmd_count] = realloc(exec_args[cmd_count], sizeof(char*) * max_args_temp);
            }
        }
    }

    exec_args[cmd_count][args_count] = NULL;
    *n_cmds = cmd_count + 1;

    return exec_args;
}

void execute_pipeline(char ***commands, int n_cmds, int fd_output){
    int pipes[n_cmds - 1][2];
    ssize_t ret;

    pid_t pid;
    for (int i = 0; i < n_cmds; i++) {
        ret = pipe(pipes[i]);
        if(ret == -1){
            perror("Erro ao criar o pipe");
            _exit(1);
        }

        pid = fork();
        if (pid == 0) {
            dup2(fd_output, 2);
            if (i == 0) {
                dup2(pipes[i][1], 1);
            }
            else if (i == n_cmds - 1) {
                dup2(pipes[i - 1][0], 0);
                dup2(fd_output, 1);
            }
            else {
                dup2(pipes[i - 1][0], 0);
                dup2(pipes[i][1], 1);
            }

            int exec_ret = execvp(commands[i][0], commands[i]);
            if(exec_ret  == -1) {
                perror("Erro ao executar um comando");
                _exit(1);
            }
            _exit(0);
        }
        else if (pid < 0) {
            perror("Erro ao criar o processo filho");
            _exit(1);
        }
        else{
            if(i == 0){
                close(pipes[i][1]);
            }
            else if(i == n_cmds - 1){
                close(pipes[i-1][0]);
            }
            else{
                close(pipes[i-1][0]);
                close(pipes[i][1]);
            }
        }
    }

    for(int i = 0; i < n_cmds; i++){
        wait(NULL);
    }
}

void execute_command(char** command, int fd_output){
    int fork_ret, ret;

    fork_ret = fork();
    switch (fork_ret) {
        case -1:
            perror("Erro no fork");
            break;
        case 0:
            dup2(fd_output, 1);
            dup2(fd_output, 2);
            ret = execvp(command[0], command);
            close(fd_output);
            if (ret == -1) {
                perror("Erro ao executar um comando");
            }
            _exit(ret);
        default:
            wait(NULL);
            break;
    }
}

void process_command(char* commands_original, const char* output_folder, int task_id, int finish_id, struct timeval initial_time){
    int n_cmds;
    char *commands_temp = strdup(commands_original);
    char ***commands = parse_command(commands_original, &n_cmds);

    char output_filename[40];
    sprintf(output_filename, "%soutput_tarefa%d.txt", output_folder, task_id);
    int fd_output = open(output_filename, O_WRONLY | O_CREAT, 0666);
    if (fd_output == -1) {
        perror("Erro ao abrir o ficheiro de output");
    }

    char output_geral_filename[40];
    sprintf(output_geral_filename, "%starefas_terminadas.txt", output_folder);
    int fd_output_geral = open(output_geral_filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if(fd_output_geral == -1){
        perror("Erro ao abrir o ficheiro de output");
    }

    char finished_tasks_filename[20];
    sprintf(finished_tasks_filename, "./tmp/logs_tarefas");
    int fd_finished = open(finished_tasks_filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if(fd_finished == -1){
        perror("Erro ao abrir o ficheiro das tarefas acabadas");
    }

    if(n_cmds == 1) {
        execute_command(commands[0], fd_output);
    }
    else if(n_cmds > 1){
        execute_pipeline(commands, n_cmds, fd_output);
    }

    struct timeval final_time;
    gettimeofday(&final_time, NULL);

    double elapsed_time = (double) (final_time.tv_sec - initial_time.tv_sec) * 1000.0 +
                          (double) (final_time.tv_usec - initial_time.tv_usec) / 1000.0;

    char task_message[60];
    ssize_t wr_bytes = sprintf(task_message, "Tarefa ID %d -> Duração: %.3lf ms\n", task_id, elapsed_time);

    if (write(fd_output_geral, task_message, wr_bytes) == -1) {
        perror("Erro ao escrever no ficheiro de output (mensagem geral)");
    }

    Logs finished_task;
    finished_task.pid = task_id;
    strcpy(finished_task.command, commands_temp);
    finished_task.time = elapsed_time;
    if(write(fd_finished, &finished_task, sizeof(Logs)) == -1){
        perror("Erro ao escrever no ficheiro das tarefas acabadas");
    }

    if(n_cmds < MAX_CMDS_INITIAL){
        n_cmds = MAX_CMDS_INITIAL;
    }

    for(int i = 0; i < n_cmds; i++){
        free(commands[i]);
    }
    free(commands);
    free(commands_temp);

    close(fd_output);
    close(fd_output_geral);

    int server_fifo_write = open(ORCHESTRATOR, O_WRONLY);
    if(server_fifo_write == -1){
        perror("Erro na abertura do FIFO do servidor");
    }

    Message finish_msg;
    finish_msg.pid = finish_id;
    finish_msg.option = FINISHED_TASK;
    finish_msg.expectedTime = 0;
    finish_msg.command[0] = '\0';

    if(write(server_fifo_write, &finish_msg, sizeof(Message)) == -1){
        perror("Erro na escrita para o FIFO do servidor");
    }
    close(server_fifo_write);
}

void status(GList *running_list, GList *pending_list, int client_fifo){
    int fd_logs = open("./tmp/logs_tarefas", O_RDONLY | O_CREAT, 0666);
    if(fd_logs == -1) {
        perror("Erro ao abrir o ficheiro das tarefas terminadas");
    }

    GList *running = running_list;
    ssize_t string_bytes;
    if(write(client_fifo, "Executing\n", 10) == -1){
        perror("Erro na escrita para o FIFO do cliente (executing)");
    }
    while(running != NULL){
        Task *task = running->data;
        char task_log[320];
        string_bytes = sprintf(task_log, "%d %s\n", task->pid, task->command);
        if(write(client_fifo, task_log, string_bytes) == -1){
            perror("Erro na escrita para o FIFO do cliente (executing)");
        }
        running = g_list_next(running);
    }
    g_list_free(running_list);

    GList *pending = pending_list;
    if(write(client_fifo, "\nScheduled\n", 11) == -1){
        perror("Erro na escrita para o FIFO do cliente (scheduled)");
    }
    while(pending != NULL){
        Task *task = pending->data;
        char task_log[320];
        string_bytes = sprintf(task_log, "%d %s\n", task->pid, task->command);
        if(write(client_fifo, task_log, string_bytes) == -1){
            perror("Erro na escrita para o FIFO do cliente (scheduled)");
        }
        pending = g_list_next(pending);
    }
    g_list_free(pending_list);

    Logs task;
    if(write(client_fifo, "\nCompleted\n", 11) == -1){
        perror("Erro na escrita para o FIFO do cliente (completed)");
    }
    while(read(fd_logs, &task, sizeof(Logs)) > 0){
        char task_log[MESSAGE_TO_USER_MAX_SIZE];
        string_bytes = sprintf(task_log, "%d %s %.3lf ms\n", task.pid, task.command, task.time);
        if(write(client_fifo, task_log, string_bytes) == -1){
            perror("Erro na escrita para o FIFO do cliente (completed)");
        }
    }

    close(client_fifo);

    int server_fifo_write = open(ORCHESTRATOR, O_WRONLY);
    if(server_fifo_write == -1){
        perror("Erro na abertura do FIFO do servidor");
    }

    Message finish_msg;
    finish_msg.pid = getpid();
    finish_msg.option = FINISHED_STATUS;
    finish_msg.expectedTime = 0;
    finish_msg.command[0] = '\0';

    if(write(server_fifo_write, &finish_msg, sizeof(Message)) == -1){
        perror("Erro na escrita para o FIFO do servidor");
    }
    close(server_fifo_write);
}

gint compare_tasks(gconstpointer a, gconstpointer b, gpointer user_data){
    Task *task_a = (Task *) a;
    Task *task_b = (Task *) b;

    if(task_a->expectedTime < task_b->expectedTime){
        return -1;
    }
    else if(task_a->expectedTime > task_b->expectedTime){
        return 1;
    }
    else{
        return 0;
    }
}

int run_queue_tasks(GQueue *monitor_pending, GHashTable *monitor_running, const char *outputFolder, int maxParallelTasks, int *n_active_tasks){
    while (*n_active_tasks < maxParallelTasks && !g_queue_is_empty(monitor_pending)) {
        Task *next_task = g_queue_pop_head(monitor_pending);

        pid_t child_queue = fork();
        if (child_queue < 0) {
            perror("Erro no fork");
            return -1;
        } else if (child_queue == 0) {
            char *command = strdup(next_task->command);
            process_command(command, outputFolder, next_task->pid, getpid(), next_task->initial_time);
            free(command);

            _exit(0);
        }

        int *key = malloc(sizeof(int));
        *key = child_queue;
        g_hash_table_insert(monitor_running, key, next_task);
        (*n_active_tasks)++;
    }

    return 0;
}

int run_server(int server_fifo_read, int server_fifo_write, int maxParallelTasks, const char* outputFolder, SchedulePolicy schedule_policy){
    GQueue *monitor_pending = g_queue_new();
    GHashTable *monitor_running = g_hash_table_new_full(g_int_hash, g_int_equal, free, free);
    int n_active_tasks = 0;
    Message msg;

    int will_close = 0;
    while (read(server_fifo_read, &msg, sizeof(Message)) > 0 && !will_close) {
        if(msg.option == FINISHED_TASK){
            waitpid(msg.pid, NULL, 0);
            int key = msg.pid;
            g_hash_table_remove(monitor_running, &key);
            n_active_tasks--;

            if (n_active_tasks < maxParallelTasks) {
                int val = run_queue_tasks(monitor_pending, monitor_running, outputFolder, maxParallelTasks, &n_active_tasks);
                if (val == -1) {
                    return -1;
                }
            }
        }
        else if(msg.option == FINISHED_STATUS){
            waitpid(msg.pid, NULL, 0);
        }
        else {
            if (msg.option == EXECUTE) {
                Task *new_task = malloc(sizeof(Task));
                new_task->pid = msg.pid;
                new_task->expectedTime = msg.expectedTime;
                strcpy(new_task->command, msg.command);
                struct timeval initial_time;
                gettimeofday(&initial_time, NULL);
                new_task->initial_time = initial_time;

                if (n_active_tasks < maxParallelTasks) {
                    pid_t child = fork();
                    if (child < 0) {
                        perror("Erro no fork");
                        return -1;
                    } else if (child == 0) {
                        process_command(msg.command, outputFolder, msg.pid, getpid(), new_task->initial_time);
                        _exit(0);
                    }

                    int *key = malloc(sizeof(int));
                    *key = child;
                    g_hash_table_insert(monitor_running, key, new_task);
                    n_active_tasks++;
                } else {
                    g_queue_push_tail(monitor_pending, new_task);
                    if(schedule_policy == SJF){
                        g_queue_sort(monitor_pending, compare_tasks, NULL);
                    }
                }
            }

            int task_id;
            char fifo_name[35];
            sprintf(fifo_name, "%s%d", CLIENT, msg.pid);
            int client_fifo = open(fifo_name, O_WRONLY);
            if (client_fifo == -1) {
                perror("Erro na abertura do FIFO do cliente");
            }

            if (msg.option == CLOSE) {
                task_id = -1;
            } else {
                task_id = msg.pid;
            }

            if (msg.option == STATUS) {
                pid_t child_status = fork();
                if (child_status == 0) {
                    status(g_hash_table_get_values(monitor_running), g_queue_peek_head_link(monitor_pending),
                           client_fifo);
                    _exit(0);
                }
            } else {
                if (write(client_fifo, &task_id, sizeof(int)) == -1) {
                    perror("Erro na escrita para o FIFO do cliente (task_id)");
                    return -1;
                }
            }

            close(client_fifo);

            if (msg.option == CLOSE) {
                will_close = 1;

                while(n_active_tasks > 0) {
                    wait(NULL);
                    n_active_tasks--;
                }

                g_queue_clear_full(monitor_pending, free);
                g_hash_table_destroy(monitor_running);
                close(server_fifo_write);
            }
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        if(write(2, "Servidor: Formato errado.\n", 27) == -1){
            perror("Erro na escrita para o standard error");
        }
        return 1;
    }

    int len = strlen(argv[1]);
    char *outputFolder;
    if (argv[1][len - 1] != '/') {
        outputFolder = malloc(len + 2);
        strcpy(outputFolder, argv[1]);
        strcat(outputFolder, "/");
    } else {
        outputFolder = malloc(len + 1);
        strcpy(outputFolder, argv[1]);
    }

    int maxParallelTasks = atoi(argv[2]);

    SchedulePolicy schedule_policy;
    if (strcmp(argv[3], "sjf") == 0){
        schedule_policy = SJF;
    }
    else if(strcmp(argv[3], "fcfs") == 0){
        schedule_policy = FCFS;
    }
    else{
        if(write(2, "Servidor: Política de escalonamento inválida. Válidas: 'sjf' e 'fcfs'.\n", 75) == -1){
            perror("Erro na escrita para o standard error");
        }
        return 1;
    }

    if(mkfifo(ORCHESTRATOR, 0666) == -1){
        perror("Erro ao criar o FIFO do servidor");
        return -1;
    }

    int server_fifo_read = open(ORCHESTRATOR, O_RDONLY);
    if(server_fifo_read == -1){
        perror("Erro na abertura do FIFO do servidor (leitura)");
        return -1;
    }

    int server_fifo_write = open(ORCHESTRATOR, O_WRONLY);
    if(server_fifo_write == -1){
        perror("Erro na abertura do FIFO do servidor (escrita)");
        return -1;
    }

    run_server(server_fifo_read, server_fifo_write, maxParallelTasks, outputFolder, schedule_policy);

    close(server_fifo_read);
    unlink(ORCHESTRATOR);
    free(outputFolder);

    return 0;
}
