#include "Server.h"
#include <pthread.h>
#include <ifaddrs.h>

// run server

void *serve(void *);
int main(int argc, char **argv) {
    char cmd_port[100] = {'\0'};
    sprintf(cmd_port, "%d", SERVER_CMD_PORT_DEFAULT);
    char root[100] = {'\0'};
    strcpy(root, SERVER_ROOT_DEFAULT);
    int iarg = 0;
    for(;iarg < argc;iarg++)
    {
        if(!strcmp(argv[iarg], "-root"))
        {
            if(iarg + 1 == argc)
            {
                printf("Error argvs.\n");
                return 1;
            }
            memset(root, '\0', sizeof root);
            strcpy(root, argv[iarg + 1]);
        }
        else if(!strcmp(argv[iarg], "-port"))
        {
            if(iarg + 1 == argc)
            {
                printf("Error argvs.\n");
                return 1;
            }
            memset(cmd_port, '\0', sizeof cmd_port);
            strcpy(cmd_port, argv[iarg + 1]);
        }
    }
    
    char cwd[100] = {'\0'};
    getcwd(cwd, 100);
    if(chdir(root) != 0)
    {
//        printf("%d", chdirresult);
        char mkcmd[100] = {'\0'};
        sprintf(mkcmd, "mkdir %s", root);
        int status_initroot = system(mkcmd);
        if(status_initroot == -1 || !WIFEXITED(status_initroot) || WEXITSTATUS(status_initroot) != 0)
        {
            // debug
//            printf("debug: first chdir.\n");
            printf("Error creating root dir %s.\n", root);
            return 1;
        }
    }
    chdir(cwd);
    
    int new_socket_temp, server_socket;
    struct sockaddr_in server, client;
    
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == -1)
    {
        printf("Error in creating socket.\n");
    }
    server.sin_family = AF_INET;
    
    server.sin_addr.s_addr = inet_addr(SERVER_HOST_DEFAULT);
    server.sin_port = htons((int)strtol(cmd_port, (char **)NULL, 10));
    
    if(bind(server_socket, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        printf("Can't bind using ip address %s: %s.\n", SERVER_HOST_DEFAULT, strerror(errno));
        return 1;
    }
    printf("Bound.\n");
    
    if(listen(server_socket, 10) == -1)
    {
        printf("Can't listen using IP address %s: %s.\n", SERVER_HOST_DEFAULT, strerror(errno));
        return 1;
    }
    
    printf("Server listening at %s:%s.\n", SERVER_HOST_DEFAULT, cmd_port);
    
    int sockaddr_in_size = sizeof(struct sockaddr_in);
    while(1)
    {
        new_socket_temp = accept(server_socket, (struct sockaddr *)&client, (socklen_t*)&sockaddr_in_size);
        if(new_socket_temp == -1) continue;
        Server *new_server = (Server *)malloc(sizeof(Server));
        new_server->command_socket = new_socket_temp;
        new_server->file_socket = -1;
        new_server->status = 0;
        new_server->mode = 0;
        new_server->fport = SERVER_FILE_PORT_DEFAULT;
        strcpy(new_server->froot, root);
        strcpy(new_server->host, SERVER_HOST_DEFAULT);
        
        strcpy(new_server->name_prefix, SERVER_NAME_PREFIX_DEFAULT);
        // check if default dir exists, if not, create it
        char dir_path[100] = {'\0'};
        sprintf(dir_path, "%s%s", new_server->froot, new_server->name_prefix);
        if(chdir(dir_path) != 0)
        {
            char cmd[100] = {'\0'};
            sprintf(cmd, "mkdir %s", dir_path);
            int status = system(cmd);
            if(status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
            {
//                printf("debug: second chdir.\n");
                printf("Error creating dir %s.\n", dir_path);
                return 1;
            }
        }
        chdir(cwd);
        
        char msg[MAX_MSG_LENGTH];
        sprintf(msg, MSG220, SERVER_NAME_PREFIX_DEFAULT);
        write(new_socket_temp, msg, strlen(msg));
        
        pthread_t sniffer_thread;
        if(pthread_create(&sniffer_thread, NULL, serve, (void *)new_server) < 0)
        {
            printf("Error in creating thread for new client.\n");
            return 1;
        }
        printf("Accepted new client. Use socket %d.\n", new_socket_temp);
    }
}
void *serve(void *server_ptr)
{
    Server *pserver = (Server *)server_ptr;
    while(1)
    {
        char msg[MAX_MSG_LENGTH] = {'\0'};
        int n = read(pserver->command_socket, msg, MAX_MSG_LENGTH);
        if(n <= 0)
        {
            if(n == 0) printf("Client exit.\n");
            else printf("Error in reading from client.\n");
            if(pserver->file_socket != -1) close(pserver->file_socket);
            if(pserver->command_socket != -1) close(pserver->command_socket);
            free(server_ptr);
            return NULL;
        }
        int result = dealWithMessage(pserver, msg);
        if(result == 1) // quit
        {
            printf("Connection end.\n");
            // free
            // sockets were already closed
            free(server_ptr);
            break;
        }
    }
    return NULL;
}
