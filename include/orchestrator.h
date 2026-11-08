#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#define MAX_ARGS_INITIAL 7
#define MAX_CMDS_INITIAL 5

char*** parse_command(char* cmd, int* n_cmds);

void execute_pipeline(char ***commands, int n_cmds, int fd_output);

void execute_command(char** command, int fd_output);

void status(GList *running_list, GList *pending_list, int client_fifo);

gint compare_tasks(gconstpointer a, gconstpointer b, gpointer user_data);

int run_queue_tasks(GQueue *monitor_pending, GHashTable *monitor_running, const char *outputFolder, int maxParallelTasks, int *n_active_tasks);

int fcfs(int server_fifo_read, int server_fifo_write, int maxParallelTasks, const char* outputFolder);

void process_command(char* commands_original, const char* output_folder, int task_id, int finish_id, struct timeval initial_time);

#endif