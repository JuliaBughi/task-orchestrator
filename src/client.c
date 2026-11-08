#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "defs.h"
#include "client.h"

int check_if_multiple_commands(char* cmd) {
    char* string;
    int multiple_commands = 0;
    char *command = strdup(cmd);
    char *original_command = command;

    while((string = strsep(&command, " ")) != NULL){
        if(strcmp(string, "|") == 0){
            multiple_commands = 1;
            break;
        }
    }

    free(original_command);

    return multiple_commands;
}

int main(int argc, char **argv){
    if(argc < 2){
        if(write(2, "Client: Formato errado.\n", 24) == -1){
            perror("Erro na escrita da mensagem para o stderr");
        }
        return 1;
    }

    Message msg;
    char fifo_name[35];
    pid_t pid_client = getpid();
    msg.pid = pid_client;
    sprintf(fifo_name, "%s%d", CLIENT, pid_client);

    if (mkfifo(fifo_name, 0666) == -1) {
        perror("Erro ao criar o FIFO do cliente");
        return -1;
    }

    int server_fifo = open(ORCHESTRATOR, O_WRONLY);
    if(server_fifo == -1){
        perror("Erro na abertura do FIFO do servidor");
        unlink(fifo_name);
        return -1;
    }

    if(strcmp(argv[1], "execute") == 0){
        if(argc != 5){
            if(write(2, "Client: Formato da opção 'execute' errado.\n", 45) == -1){
                perror("Erro na escrita da mensagem para o stderr");
            }
            return 1;
        }

        int expectedTime = atoi(argv[2]);
        msg.option = EXECUTE;
        msg.expectedTime = expectedTime;

        int multiple_commands = check_if_multiple_commands(argv[4]);
        strcpy(msg.command, argv[4]);

        if(strcmp(argv[3], "-u") == 0 && !multiple_commands){
            //execução de uma tarefa
            if(write(server_fifo, &msg, sizeof(Message)) == -1){
                perror("Erro no write para o FIFO do servidor (execute -u)");
                return -1;
            }
            close(server_fifo);
        }
        else if(strcmp(argv[3], "-p") == 0 && multiple_commands){
            //execução de pipeline
            if(write(server_fifo, &msg, sizeof(Message)) == -1){
                perror("Erro no write para o FIFO do servidor (execute -p)");
                return -1;
            }
            close(server_fifo);
        }
        else{
            if(write(2, "Client: Flag não suportada.\nUse '-u' para executar apenas um comando e '-p' para executar uma pipeline de comandos.\n", 117) == -1){
                perror("Erro na escrita da mensagem para o stderr");
            }
            return 1;
        }
    }
    else if(strcmp(argv[1], "status") == 0){
        //listar as tarefas
        if(argc != 2){
            if(write(2, "Client: Formato da opção 'status' errado.\n", 44) == -1){
                perror("Erro na escrita da mensagem para o stderr");
            }
            return 1;
        }

        msg.option = STATUS;
        if(write(server_fifo, &msg, sizeof(Message)) == -1){
            perror("Erro no write para o FIFO do servidor (status)");
            return -1;
        }
        close(server_fifo);
    }
    else if(strcmp(argv[1], "close") == 0){
        //opção para encerrar o servidor
        if(argc != 2){
            if(write(2 ,"Client: Formato da opção 'close' errado.\n", 43) == -1){
                perror("Erro na escrita da mensagem para o stderr");
            }
            return 1;
        }

        msg.option = CLOSE;
        if(write(server_fifo, &msg, sizeof(Message)) == -1){
            perror("Erro no write para o FIFO do servidor (close)");
            return -1;
        }
        close(server_fifo);
    }
    else{
        if(write(2, "Client: Opção não suportada.\n", 32) == -1){
            perror("Erro na escrita da mensagem para o stderr");
        }
        return 1;
    }

    //Receber identificador da tarefa do servidor
    int client_fifo = open(fifo_name, O_RDONLY);
    if(client_fifo == -1){
        perror("Erro na abertura do FIFO do cliente");
        return -1;
    }
    ssize_t write_bytes;
    if(msg.option == STATUS) {
        char status_message[MESSAGE_TO_USER_MAX_SIZE] = "";
        while((write_bytes = read(client_fifo, &status_message, MESSAGE_TO_USER_MAX_SIZE)) > 0) {
            if (write(1, &status_message, write_bytes) == -1) {
                perror("Erro na escrita da mensagem para o stdout");
                return -1;
            }
        }
    }
    else {
        char message_to_user[35] = "";
        int task_id;

        while ((read(client_fifo, &task_id, sizeof(int))) > 0) {
            if (task_id == -1) {
                strcpy(message_to_user, "A encerrar o servidor...\n");
                write_bytes = 25;
            } else {
                write_bytes = sprintf(message_to_user, "Tarefa submetida com ID %d\n", task_id);
            }

            if (write(1, &message_to_user, write_bytes) == -1) {
                perror("Erro na escrita da mensagem para o stdout");
                return -1;
            }
        }
    }

    close(client_fifo);
    unlink(fifo_name);

    return 0;
}