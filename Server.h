//
// Created by zhaoshangming on 17-10-25.
// Server struct and commands
//

#ifndef FTP_SERVER_H
#define FTP_SERVER_H
#include "defs.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>  // read and write
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>

typedef struct Server
{
    char client_host[100];
    char name_prefix[100]; // name prefix
    char rename_source[100];
    char host[100];
    int client_port;
    int cmd_port;
    int fport;
    int status; // 0: before USER cmd, 1: asking for password, 2: got password, 3: after RNFR
    int mode; // 0: origin, 1: port, 2: pasv
    char froot[100];
    int command_socket;
    int file_socket;
}Server;

void USER(Server *pserver, char *parameter);
void PASS(Server *pserver, char *parameter);
void PORT(Server *pserver, char *parameter);
void PASV(Server *pserver);
int RETR(Server *pserver, char *parameter);
int STOR(Server *pserver, char *parameter);
int getFsocket(Server *pserver, char *parameter);
void SYST(Server *pserver, char *parameter);
void TYPE(Server *pserver, char *parameter);
int QUIT(Server *pserver, char *parameter); // 1 for succeed, 0 for fail
void MKD(Server *pserver, char *parameter);
void CWD(Server *pserver, char *parameter);
int LIST(Server *pserver, char *parameter);
void RMD(Server *pserver, char *parameter);
void RNFR(Server *pserver, char *parameter);
void RNTO(Server *pserver, char *parameter);
void DELE(Server *pserver, char *parameter);
void getRepresentedPathname(char *name_prefix, char *parameter, char *result);
void getActualPathname(char *froot, char *represented_path_name, char *result);
void decodePathname(char *parameter); // decode all \000 to \012 in place
int isLegalDir(char *parameter);
int dealWithMessage(Server *pserver, char *msg) // -1 for fail, 0 for normal, 1 for quit
{
    // parse msg
    int i = 0, j = 0;
    char verb[10], parameter[MAX_MSG_LENGTH] = {'\0'};
    int flag = 0;
    int new_line_flag = 0;
    for (;i < strlen(msg);i++)
    {
        if(msg[i] == '\r')
        {
            if(!(i < strlen(msg) - 1 && msg[i + 1] == '\n'))
            {
                continue;
            }
            new_line_flag = 1;
            break;
        }
        if(msg[i] == ' ')
        {
            flag = 1;
            j = 0;
            continue;
        }
        if(flag == 0)
        {
            verb[j] = msg[i];
            j++;
        }
        else
        {
            parameter[j] = msg[i];
            j++;
        }
    }
    if(new_line_flag == 0)
    {
        printf("Invalid message from client.\n");
        return -1;
    }
    // deal with message
    if(strncmp(verb, "QUIT", 4) == 0 || strncmp(verb, "ABOR", 4) == 0)
    {
        int result = QUIT(pserver, parameter);
        if(result == 1)
        {
            return 1;
        }
        else
        {
            printf("Error in QUIT.\n");
            return -1;
        }
    }
    else if(pserver->status == 0)
    {
        if(strncmp(verb, "USER", 4) == 0) USER(pserver, parameter);
    }
    else
    {
        if(strncmp(verb, "PASS", 4) == 0) PASS(pserver, parameter);
        else if(strncmp(verb, "PORT", 4) == 0) PORT(pserver, parameter);
        else if(strncmp(verb, "PASV", 4) == 0) PASV(pserver);
        else if(strncmp(verb, "RETR", 4) == 0)
        {
            // close all file connections
            int fsocket = RETR(pserver, parameter);
            close(pserver->file_socket);
            if(fsocket != -1) close(fsocket);
            pserver->file_socket = -1;
            pserver->mode = 0;
        }
        else if(strncmp(verb, "STOR", 4) == 0)
        {
            int fsocket = STOR(pserver, parameter);
            close(pserver->file_socket);
            if(fsocket != -1) close(fsocket);
            pserver->file_socket = -1;
            pserver->mode = 0;
        }
        else if(strncmp(verb, "SYST", 4) == 0) SYST(pserver, parameter);
        else if(strncmp(verb, "TYPE", 4) == 0) TYPE(pserver, parameter);
        else if(strncmp(verb, "MKD", 3) == 0) MKD(pserver, parameter);
        else if(strncmp(verb, "CWD", 3) == 0) CWD(pserver, parameter);
        else if(strncmp(verb, "LIST", 4) == 0)
        {
            int fsocket = LIST(pserver, parameter);
            close(pserver->file_socket);
            if(fsocket != -1) close(fsocket);
            pserver->file_socket = -1;
            pserver->mode = 0;
        }
        else if(strncmp(verb, "RMD", 3) == 0) RMD(pserver, parameter);
        else if(strncmp(verb, "RNFR", 4) == 0) RNFR(pserver, parameter);
        else if(strncmp(verb, "RNTO", 4) == 0) RNTO(pserver, parameter);
        else if(strncmp(verb, "DELE", 4) == 0) DELE(pserver, parameter);
        else
        {
            char msg[MAX_MSG_LENGTH] = {'\0'};
            strcpy(msg, "500 Invalid command.\r\n");
            if(write(pserver->command_socket, msg, strlen(msg)) == -1)
            {
                printf("Error writing to client.\r\n");
                return -1;
            }
        }
    }
    return 0;
}

int isLegalDir(char *parameter)
{
    int len = strlen(parameter);
    // debug
    printf("parameter length: %d.\n", len);
    int i = 0;
    for(;i < len - 2;i++)
    {
        if(parameter[i] == '.' && parameter[i + 1] == '.' && parameter[i + 2] == '/')
        {
            printf("Illegal dir.\n");
            return 0;
        }
    }
    // debug
    printf("Legal dir.\n");
    return 1;
}


void USER(Server *pserver, char *parameter)
{
    printf("USER %s.\n", parameter);
    pserver->status = 1;
    if(write(pserver->command_socket, MSG331, strlen(MSG331)) == -1)
    {
        printf("Error in handling USER command.\n");
        return;
    }
    printf("Use socket %d.\n", pserver->command_socket);
}

void PASS(Server *pserver, char *parameter)
{
    printf("PASS %s.\n", parameter);
    if(pserver->status != 1)
    {
        printf("Error server status in PASS.\n");
        return;
    }
    int i = 0, len = sizeof MSG230 / sizeof(char *);
    for(;i < len;i++)
    {
        if(write(pserver->command_socket, MSG230[i], strlen(MSG230[i])) == -1)
        {
            printf(WRITE_ERROR, "USER");
            return;
        }
    }
    pserver->status = 2;
}

void PORT(Server *pserver, char *parameter)
{
    printf("PORT %s.\n", parameter);
    if(pserver->status != 2)
    {
        printf("Wrong server status.\n");
        return;
    }
    int h1, h2, h3, h4, p1, p2;
    sscanf(parameter, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
    sprintf(pserver->client_host, "%d.%d.%d.%d", h1, h2, h3, h4);
    if(pserver->file_socket != -1)
    {
        close(pserver->file_socket);
    }
    pserver->mode = 1;
    pserver->client_port = 256 * p1 + p2; // Don't need to connect immediately
    char msg[MAX_MSG_LENGTH] = {'\0'};
    sprintf(msg, "200 Successfully set client address to %s:%d.\r\n", pserver->client_host, pserver->client_port);
    if(write(pserver->command_socket, msg, strlen(msg)) == -1)
    {
        printf(WRITE_ERROR, "PORT");
        return;
    }
    printf("Client address set to %s:%d.\n", pserver->client_host, pserver->client_port);
}

void PASV(Server *pserver)
{
    printf("PASV.\n");
    if(pserver->file_socket != -1)
    {
        close(pserver->file_socket);
    }
    int i = 0, new_socket;
    struct sockaddr_in new_addr;
    new_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (new_socket == -1)
    {
        printf("Error creating socket.\n");
        return;
    }
    new_addr.sin_family = AF_INET;
    new_addr.sin_addr.s_addr = inet_addr(pserver->host);
    for(i = 20000;i < 65535;i++)
    {
        new_addr.sin_port = htons(i);
        // try bind
        if(bind(new_socket, (struct sockaddr *)&new_addr, sizeof(new_addr)) == -1)
        {
            continue;
        }
        // try listen
        if(listen(new_socket, 10) == -1)
        {
            printf("Error in listening.\n");
            continue;
        }
        char msg[MAX_MSG_LENGTH];
        int h1, h2, h3, h4;
        sscanf(pserver->host, "%d.%d.%d.%d", &h1, &h2, &h3, &h4);
        sprintf(msg, "227 %d,%d,%d,%d,%d,%d\r\n", h1, h2, h3, h4, i / 256, i % 256);
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "PASV");
            return;
        }
        pserver->file_socket = new_socket;
        pserver->fport = i;
        pserver->mode = 2;
        break;
    }
}

int getFsocket(Server *pserver, char *parameter)
{
    int fsocket = -1;
    if(pserver->mode == 1)
    {
        struct sockaddr_in new_addr;
        if ((pserver->file_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        {
            printf("Error creating file socket in getFsocket.\n");
            return -1;
        }
        new_addr.sin_family = AF_INET;
        new_addr.sin_addr.s_addr = inet_addr(pserver->client_host);
        new_addr.sin_port = htons(pserver->client_port);
        if(connect(pserver->file_socket, (struct sockaddr*)&new_addr, sizeof(new_addr)) == -1)
        {
            printf("Error connecting to client in getFsocket: %s.\n", strerror(errno));
            return -1;
        }
        fsocket = pserver->file_socket;
    }
    else if(pserver->mode == 2)
    {
        struct sockaddr_in new_addr;
        int sockaddr_in_size = sizeof(struct sockaddr_in);
        fsocket = accept(pserver->file_socket, (struct sockaddr *)&new_addr, (socklen_t*)&sockaddr_in_size);
        if(fsocket == -1)
        {
            printf("Error in accepting from client in getFsocket.\n");
            return -1;
        }
    }
    return fsocket;
}

int RETR(Server *pserver, char *parameter)
{
    printf("RETR %s.\n", parameter);
    int fsocket = getFsocket(pserver, parameter);
    if(fsocket == -1)
    {
        printf("Error in getting fsocket in RETR.\n");
        if(write(pserver->command_socket, MSG425, strlen(MSG425)) == -1)
        {
            printf(WRITE_ERROR, "RETR");
        }
        return fsocket;
    }
    printf("Transport mode: %d, Using command socket %d, file socket %d.\n", pserver->mode, pserver->command_socket, fsocket);
    
    if(pserver->status != 2)
    {
        printf("Error server status in RETR.\n");
        if(write(pserver->command_socket, MSG425, strlen(MSG425)) == -1)
        {
            printf(WRITE_ERROR, "RETR");
        }
        return fsocket;
    }
    char msg[MAX_MSG_LENGTH];
    sprintf(msg, "%s%s\r\n", MSG150FILE, parameter);
    if(write(pserver->command_socket, msg, strlen(msg)) == -1)
    {
        printf(WRITE_ERROR, "RETR");
        return fsocket;
    }
    
    char represented_pathname[100] = {'\0'};
    char actual_pathname[100] = {'\0'};
    getRepresentedPathname(pserver->name_prefix, parameter, represented_pathname);
    getActualPathname(pserver->froot, represented_pathname, actual_pathname);
    // read from file
    int nread;
    char buf[CHUNK];
    FILE *file = fopen(actual_pathname, "rb"); // read byte
    char error_reading[MAX_MSG_LENGTH] = {'\0'};
    sprintf(error_reading, MSG451, parameter);
    if(file == NULL)
    {
        if(write(pserver->command_socket, error_reading, strlen(error_reading)) == -1)
        {
            printf(WRITE_ERROR, "RETR");
        }
        return fsocket;
    }
    while(1)
    {
        nread = fread(buf, 1, CHUNK, file);
        if(nread < 0)
        {
            if(write(pserver->command_socket, error_reading, sizeof(error_reading)) == -1)
            {
                printf(WRITE_ERROR, "RETR");
            }
            return fsocket;
        }
        if(nread == 0) break;
        int p = 0;
        char error_writing[MAX_MSG_LENGTH] = {'\0'};
        sprintf(error_writing, MSG426, parameter);
        while(p < nread)
        {
            int n = write(fsocket, buf + p, nread - p); // num of bytes actually wrote
            if(n == -1)
            {
                printf("Error in writing file to client in RETR.\n");
                if(write(pserver->command_socket, error_writing, strlen(error_writing)) == -1)
                {
                    printf(WRITE_ERROR, "RETR");
                }
                return fsocket;
            }
            p += n;
        }
    }
    fclose(file);
    if (write(pserver->command_socket, MSG226, strlen(MSG226)) == -1)
    {
        printf(WRITE_ERROR, "RETR");
        return fsocket;
    }
    return fsocket;
}

int STOR(Server *pserver, char *parameter)
{
    printf("STOR %s.\n", parameter);
    int fsocket = getFsocket(pserver, parameter);
    if(fsocket == -1)
    {
        printf(MSG425);
        if(write(pserver->command_socket, MSG425, strlen(MSG425)) == -1)
        {
            printf(WRITE_ERROR, "STOR");
        }
        return fsocket;
    }
    printf("Transport mode: %d, Using command socket %d, file socket %d.\n", pserver->mode, pserver->command_socket, pserver->file_socket);
    
    if(pserver->mode == 0)
    {
        printf("Server not in port/pasv mode in STOR.\n");
        if(write(pserver->command_socket, MSG425, strlen(MSG425)) == -1)
        {
            printf(WRITE_ERROR, "STOR");
        }
        return fsocket;
    }
    char msg[MAX_MSG_LENGTH];
    sprintf(msg, "%s%s\r\n", MSG150FILE, parameter);
    if(write(pserver->command_socket, msg, strlen(msg)) == -1)
    {
        printf(WRITE_ERROR, "RETR");
        return fsocket;
    }

    char represented_pathname[100] = {'\0'};
    getRepresentedPathname(pserver->name_prefix, parameter, represented_pathname);
    char actual_pathname[100] = {'\0'};
    getActualPathname(pserver->froot, represented_pathname, actual_pathname);
    int nread;
    char buf[CHUNK];
    char fread_error[MAX_MSG_LENGTH] = {'\0'};
    sprintf(fread_error, MSG451, parameter);
    char fwrite_error[MAX_MSG_LENGTH] = {'\0'};
    sprintf(fwrite_error, MSG451WRITE, parameter);
    FILE *file = fopen(actual_pathname, "wb");
    if(file == NULL)
    {
        if(write(pserver->command_socket, fwrite_error, strlen(fwrite_error)) == -1)
        {
            printf(WRITE_ERROR, "STOR");
        }
        return fsocket;
    }
    while(1)
    {
        nread = read(fsocket, buf, CHUNK);
        if(nread < 0)
        {
            if(write(pserver->command_socket, fread_error, strlen(fread_error)) == -1)
            {
                printf(WRITE_ERROR, "STOR");
            }
            return fsocket;
        }
        else if(nread == 0) break;
        else if(nread > 0)
        {
            int nw = fwrite(buf, nread, 1, file);
            if(nw <= 0)
            {
                if(write(pserver->command_socket, fwrite_error, strlen(fwrite_error)) == -1)
                {
                    printf(WRITE_ERROR, "STOR");
                }
                return fsocket;
            }
        }
    }
    fclose(file);
    if(write(pserver->command_socket, MSG226, strlen(MSG226)) == -1)
    {
        printf(WRITE_ERROR, "STOR");
        return fsocket;
    }
    return fsocket;
}

void SYST(Server *pserver, char *parameter)
{
    printf("SYST %s.\n", parameter);
    if(write(pserver->command_socket, MSG215, strlen(MSG215)) == -1)
    {
        printf(WRITE_ERROR, "SYST");
    }
}

void TYPE(Server *pserver, char *parameter)
{
    printf("TYPE %s.\n", parameter);
    if(write(pserver->command_socket, MSG200TYPE, strlen(MSG200TYPE)) == -1)
    {
        printf(WRITE_ERROR, "TYPE");
    }
}

int QUIT(Server *pserver, char *parameter)
{
    printf("QUIT.\n");
    int i = 0;
    for(;i < sizeof(MSG221) / sizeof(char *);i++)
    {
        if(write(pserver->command_socket, MSG221[i], strlen(MSG221[i])) == -1)
        {
            printf(WRITE_ERROR, "QUIT");
            return 0;
        }
    }
    close(pserver->command_socket);
    if(pserver->file_socket != -1)
    {
        close(pserver->file_socket);
    }
    return 1;
}

void decodePathname(char *parameter)
{
    int i = 0, len = strlen(parameter);
    for(;i < len;i++)
    {
        if(parameter[i] == '\000')
        {
            parameter[i] = '\012';
        }
    }
}

void getRepresentedPathname(char *name_prefix, char *parameter, char *result)
{
    decodePathname(parameter);
    memset(result, '\0', strlen(result));
    if(parameter[0] == '/') // begin with slash
    {
        sprintf(result, "%s", parameter);
    }
    else if(strcmp(name_prefix, "/") == 0)
    {
        sprintf(result, "%s%s", name_prefix, parameter);
    }
    else
    {
        sprintf(result, "%s/%s", name_prefix, parameter);
    }
}

void getActualPathname(char *froot, char *represented_path_name, char *result)
{
    memset(result, '\0', strlen(result));
    sprintf(result, "%s%s", froot, represented_path_name);
}

void MKD(Server *pserver, char *parameter)
{
    printf("MKD %s.\n", parameter);
    char msg[MAX_MSG_LENGTH] = {'\0'};
    if(isLegalDir(parameter) == 0)
    {
        sprintf(msg, "550 You don't have permission to create dir %s.\r\n", parameter);
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "MKD");
        }
        return;
    }

    char cwd[100] = {'\0'};
    getcwd(cwd, 100);
    
    char presented_dir[100] = {'\0'};
    char actual_dir[100] = {'\0'};
    decodePathname(parameter);
    getRepresentedPathname(pserver->name_prefix, parameter, presented_dir);
    getActualPathname(pserver->froot, presented_dir, actual_dir);
    if(chdir(actual_dir) == 0) // already created
    {
        sprintf(msg, "250 dir %s already exists.\r\n", parameter);
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "MKD");
        }
        chdir(cwd);
        return;
    }
    chdir(cwd);
    
    char cmd[100] = {'\0'};
    sprintf(cmd, "mkdir %s", actual_dir);
    system(cmd);
    int created = 0;
    if(chdir(actual_dir) == 0)
    {
        created = 1;
    }
    chdir(cwd);
    
    if(created == 1)
    {
        sprintf(msg, "250 successfully created dir %s.\r\n", parameter);
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "MKD");
            return;
        }
    }
    else
    {
        sprintf(msg, "550 Failed to create dir %s.\r\n", parameter);
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "MKD");
            return;
        }
    }
}

void CWD(Server *pserver, char *parameter)
{
    printf("CWD %s.\n", parameter);

    char msg[MAX_MSG_LENGTH] = {'\0'};
    if(isLegalDir(parameter) == 0)
    {
        sprintf(msg, "550 You don't have permission to change to dir %s.\r\n", parameter);
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "CWD");
        }
        return;
    }

    char cwd[100] = {'\0'};
    getcwd(cwd, 100);
    decodePathname(parameter);
    char expected_dir[100] = {'\0'};
    char actual_dir[100] = {'\0'};
    getRepresentedPathname(pserver->name_prefix, parameter, expected_dir);
    getActualPathname(pserver->froot, expected_dir, actual_dir);
    int created = 0;
    if(chdir(actual_dir) == 0)
    {
        created = 1;
    }
    chdir(cwd);

    if(created)
    {
        sprintf(msg, "250 Successfully changed to dir %s.\r\n", parameter);
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "CWD");
            return;
        }
        memset(pserver->name_prefix, '\0', strlen(pserver->name_prefix));
        strcpy(pserver->name_prefix, expected_dir);
    }
    else
    {
        sprintf(msg, "550 failed change to dir %s.\r\n", parameter);
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "CWD");
            return;
        }
    }
}

int LIST(Server *pserver, char *parameter)
{
    printf("LIST.\n");
    int fsocket = getFsocket(pserver, parameter);
    if(fsocket == -1)
    {
        printf("Error getting file socket in LIST.\n");
        if(write(pserver->command_socket, MSG425, strlen(MSG425)) == -1)
        {
            printf(WRITE_ERROR, "LIST");
        }
        return fsocket;
    }
    printf("Transport mode: %d, Using command socket %d, file socket %d.\n", pserver->mode, pserver->command_socket, pserver->file_socket);
    
    if(pserver->mode == 0)
    {
        printf("Error server mode in LIST.\n");
        if(write(pserver->command_socket, MSG425, strlen(MSG425)) == -1)
        {
            printf(WRITE_ERROR, "LIST");
        }
        return fsocket;
    }
    char *msg = "150 Okay.\r\n";
    if(write(pserver->command_socket, msg, strlen(msg)) == -1)
    {
        printf(WRITE_ERROR, "LIST");
        return fsocket;
    }
    char dir[100] = {'\0'};
    char actual_dir[100] = {'\0'};
    char cmd[100] = {'\0'};
    getRepresentedPathname(pserver->name_prefix, parameter, dir);
    getActualPathname(pserver->froot, dir, actual_dir);
    sprintf(cmd, "ls -l %s", actual_dir);
    int nread;
    char buf[CHUNK];
    FILE *cmd_result = popen(cmd, "r");
    char read_error[MAX_MSG_LENGTH] = {'\0'};
    sprintf(read_error, MSG451LIST, parameter);
    if(cmd_result == NULL)
    {
        if(write(pserver->command_socket, read_error, strlen(read_error)) == -1)
        {
            printf(WRITE_ERROR, "LIST");
            return fsocket;
        }
    }
    while(1)
    {
        nread = fread(buf, 1, CHUNK, cmd_result);
        if(nread < 0)
        {
            printf("Error listing dir.\n");
            if(write(pserver->command_socket, read_error, strlen(read_error)) == -1)
            {
                printf(WRITE_ERROR, "LIST");
            }
            return fsocket;
        }
        if(nread == 0) break;
        int p = 0;
        while(p < nread)
        {
            int n = write(fsocket, buf + p, nread - p); // num of bytes actually wrote
            char list_error[MAX_MSG_LENGTH] = {'\0'};
            if(n == -1)
            {
                printf("Error in writing file %s to client in LIST.\n", parameter);
                sprintf(list_error, MSG426LIST, parameter);
                if(write(pserver->command_socket, list_error, strlen(list_error)) == -1)
                {
                    printf(WRITE_ERROR, "LIST");
                }
                return fsocket;
            }
            p += n;
        }
    }
    pclose(cmd_result);
    if(write(pserver->command_socket, MSG226LIST, strlen(MSG226LIST)) == -1)
    {
        printf(WRITE_ERROR, "LIST");
        return fsocket;
    }
    return fsocket;
}

void RMD(Server *pserver, char *parameter)
{
    printf("RMD %s.\n", parameter);
    char msg[MAX_MSG_LENGTH] = {'\0'};
    if(isLegalDir(parameter) == 0 || strcmp("/", parameter) == 0)
    {
        // debug
//        printf("Permission denied.\n");
        sprintf(msg, "550 You don't have permission to remove dir %s.\r\n", parameter);
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "RMD");
        }
        return;
    }

    char cwd[100] = {'\0'};
    getcwd(cwd, 100);
    
    decodePathname(parameter);
    char dir[100] = {'\0'};
    char actual_dir[100] = {'\0'};
    getRepresentedPathname(pserver->name_prefix, parameter, dir);
    getActualPathname(pserver->froot, dir, actual_dir);
    if(chdir(actual_dir) != 0 || rmdir(actual_dir) != 0) // no such dir or failed to remove
    {
        sprintf(msg, "550 Removing dir %s failed.\r\n", parameter);
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "RMD");
            chdir(cwd);
        }
        return;
    }
    chdir(cwd);
    
//    char cmd[100] = {'\0'};
//    sprintf(cmd, "rm -r %s", actual_dir);
//    system(cmd);
    memset(msg, '\0', sizeof msg);
    sprintf(msg, "250 Successfully removed dir %s.\r\n", parameter);
    if(write(pserver->command_socket, msg, strlen(msg)) == -1)
    {
        printf(WRITE_ERROR, "RMD");
    }
}

void RNFR(Server *pserver, char *parameter)
{
    printf("RNFR.\n");
    char msg[MAX_MSG_LENGTH] = {'\0'};
    if(pserver->status != 2)
    {
        printf("RNFR: Wrong server status.\n");
        strcpy(msg, "450 Wrong server mode in RETR.\r\n");
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "RNFR");
        }
        return;
    }
    memset(pserver->rename_source, '\0', 100);
    char represented_filepath[100] = {'\0'};
    char actual_path[100] = {'\0'};
    getRepresentedPathname(pserver->name_prefix, parameter, represented_filepath);
    getActualPathname(pserver->froot, represented_filepath, actual_path);
    char froot1[100] = {'\0'};
    sprintf(froot1, "%s/", pserver->froot);
    if(strcmp(pserver->froot, actual_path) == 0 || strcmp(froot1, actual_path) == 0)
    {
        printf("RNFR: Clients are not allowed to rename root dir.\n");
        strcpy(msg, "450 Clients are not allowed to rename root dir.\r\n");
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "RNFR");
        }
        return;
    }
    
    strcpy(pserver->rename_source, actual_path);
    if(access(actual_path, 0) == 0)
    {
        sprintf(msg, "350 Successfully found file/dir %s.\r\n", parameter);
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "RNFR");
        }
        pserver->status = 3;
        return;
    }
    sprintf(msg, "450 No such file or directory: %s.\r\n", parameter);
    if(write(pserver->command_socket, msg, strlen(msg)) == -1)
    {
        printf(WRITE_ERROR, "RNFR");
    }
}

void RNTO(Server *pserver, char *parameter)
{
    printf("RNTO.\n");
    char msg[MAX_MSG_LENGTH] = {'\0'};
    if(pserver->status != 3)
    {
        strcpy(msg, "503 RNTO: Wrong server status.\r\n");
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "RNTO");
            return;
        }
    }
    pserver->status = 2;
    char represented_fpath[100] = {'\0'};
    char actual_path[100] = {'\0'};
    getRepresentedPathname(pserver->name_prefix, parameter, represented_fpath);
    getActualPathname(pserver->froot, represented_fpath, actual_path);
    char cmd[100] = {'\0'};
    sprintf(cmd, "mv %s %s", pserver->rename_source, actual_path);
    int status = system(cmd);
    if(status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        strcpy(msg, "550 Error executing command.\r\n");
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "RNTO");
        }
        return;
    }
    strcpy(msg, "250 Successfully renamed dir/file.\r\n");
    if(write(pserver->command_socket, msg, strlen(msg)) == -1)
    {
        printf(WRITE_ERROR, "RNTO");
    }
}

void DELE(Server *pserver, char *parameter)
{
    char msg[MAX_MSG_LENGTH] = {'\0'};
    if(pserver->status != 2)
    {
        printf("DELE: Wrong server status.\n");
        strcpy(msg, "450 DELE: Wrong server status.\r\n");
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "DELE");
        }
        return;
    }
    chdir(pserver->froot);
    char cmd[100] = {'\0'};
    char represented_fname[100] = {'\0'};
    char actual_fpath[100] = {'\0'};
    getRepresentedPathname(pserver->name_prefix, parameter, represented_fname);
    getActualPathname(pserver->froot, represented_fname, actual_fpath);
    sprintf(cmd, "rm %s", actual_fpath);
    int status = system(cmd);
    if(status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        strcpy(msg, "450 Error executing command.\r\n");
        if(write(pserver->command_socket, msg, strlen(msg)) == -1)
        {
            printf(WRITE_ERROR, "DELE");
        }
        return;
    }
    sprintf(msg, "250 Successfully removed file: %s.\r\n", parameter);
    if(write(pserver->command_socket, msg, strlen(msg)) == -1)
    {
        printf(WRITE_ERROR, "DELE");
    }
}


#endif //FTP_SERVER_H
