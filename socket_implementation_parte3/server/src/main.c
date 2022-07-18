#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#define PORT 8080

typedef struct{
    char name[30];
    int admin_socket;
}channel;

typedef struct {
    int client_socket;
    int channel;
    int mute;
    char nickname[30];
}client;

int main(){

    //Master socket é o servidor, client_0 e client_1 são os sockets das aplicações que iram se comunicar, client_sockets é o vetor que os sockets das aplicações seram armazenados.
    int master_socket , addrlen , new_socket ,
              max_clients = 30 , activity, i , sd;
    int max_sd;
    struct sockaddr_in address;

    client* clients = (client*) malloc(sizeof (client)*max_clients);
    channel* channels = (channel*) malloc(sizeof (channel));

    int num_channels = 0;

    //Buffer de leitura  e str_final string que será enviada para as aplicações.
    char buffer[4097], str_final[5050];

    //Estrutura de dados usada para o select.
    fd_set readfds;

    //Criamos o socket tcp.
    master_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(master_socket == 0){
        printf("Socket creation failed.");
        exit(EXIT_SUCCESS);
    }

    for(int i = 0; i < max_clients; i++){
        clients[i].client_socket = 0;
        clients[i].channel = -1;
        clients[i].nickname[0] = '\0';
        clients[i].mute = 0;
    }

    address.sin_family = AF_INET;   //IPV4
    address.sin_addr.s_addr = INADDR_ANY;   //Se liga a todos os IPs disponíveis.
    address.sin_port = htons(PORT); //porta 8080.

    //Amarramos o servidor ao endereço e porta especificadas.
    int bind_return = bind(master_socket, (struct sockaddr*)&address, sizeof(address));

    if(bind_return < 0){
        printf("Binding socket to address failed.");
        exit(EXIT_SUCCESS);
    }

    int listen_return = listen(master_socket, 8);

    if(listen_return < 0){
        printf("Listening failed.");
        exit(EXIT_SUCCESS);
    }

    addrlen = sizeof(address);

    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;

    //Pegamos o nome do host.
    gethostname(hostbuffer, sizeof(hostbuffer));

    //Com o nome do host recuperamos informações sobre o mesmo.
    host_entry = gethostbyname(hostbuffer);

    //Com as informações do host pegamos seu IP local (com o qual você usará na parte do cliente para se conectar ao servidor).
    IPbuffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));

    //Informamos ao usuário o nome do host e seu ip local.
    printf("Hostname: %s\n", hostbuffer);
    printf("Host IP: %s\n", IPbuffer);

    int cmdExit = 0;

    //Enquanto nenhuma das aplicações tenha digitado o comando exit, o servidor continua trocando mensagens entre as duas.
    while(cmdExit == 0){

        //Limpa o fd_set
        FD_ZERO(&readfds);
        //Adiciona o socket master ao fd_set.
        FD_SET(master_socket, &readfds);

        max_sd = master_socket;

        //Procura o socket de maior valor (essa parte serve para o select).
        for(int i = 0; i < max_clients; i++){

            if(clients[i].client_socket > 0){
                FD_SET(clients[i].client_socket, &readfds);
            }

            if(clients[i].client_socket > max_sd){
                max_sd = clients[i].client_socket;
            }

        }

        //Espera por uma atividade em algum dos sockets indefinidamente.
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

        if((activity < 0) && (errno!=EINTR)){
            printf("select error");
        }

        //Se algo ocorreu no master_socket então existe um novo pedido de conexão.
        if (FD_ISSET(master_socket, &readfds))  {

            new_socket = accept(master_socket,(struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (new_socket<0) {
                printf("Failed to accept new connection");
                exit(EXIT_SUCCESS);
            }

            //Informa o usuário sobre o socket - usado para enviar e receber mensagens
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            //adiciona novo socket no vetor de sockets
            for (i = 0; i < max_clients; i++){

                //se a posição está vazia:
                if( clients[i].client_socket == 0 ){
                    clients[i].client_socket = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);
                    break;
                }
            }
        }

        //Verifica cada um dos sockets.
        for(int i = 0; i < max_clients; i++){
            sd = clients[i].client_socket;

            //Se algo aconteceu em algum dos sockets é uma mensagem chegando.
            if (FD_ISSET( sd , &readfds)){
                memset(buffer, '\0', sizeof (buffer));

                int len = read( sd , buffer, 4097);

                char aux[4097];
                strcpy(aux, buffer);
                char *cmd = strtok(aux, " ");
                if (len == 0)  {

                    //Alguém se desconectou
                    getpeername(sd , (struct sockaddr*)&address , \
                                (socklen_t*)&addrlen);

                    //Mostra ao usuário dados do socket desconectado
                    printf("Host disconnected , ip %s , port %d \n" ,
                           inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

                    //Fecha o socket que era usado e o seta em 0 para reuso.
                    close( sd );
                    clients[i].client_socket = 0;

                }else if(strcmp(cmd, "/join") == 0){
                    char* channel_name = strtok(NULL, " ");

                    int j = 0;

                    while(j < num_channels){
                        if(strcmp(channels[j].name, channel_name) == 0){
                            break;
                        }
                        j++;
                    }

                    if(j >= num_channels){
                        num_channels++;
                        channels = (channel*) realloc(channels, sizeof (channel) * num_channels);

                        strcpy(channels[j].name, channel_name);
                        channels[j].admin_socket = clients[i].client_socket;
                    }

                    clients[i].channel = j;

                    printf("Joined channel %s\n", channel_name);

                }else if(strcmp(cmd, "/nickname") == 0){
                    char* nickname = strtok(NULL, " ");

                    strcpy(clients[i].nickname, nickname);

                }else if(strcmp(cmd, "/kick") == 0){
                    char* nickname = strtok(NULL, " ");

                    int j = 0;

                    if(channels[clients[i].channel].admin_socket == clients[i].client_socket){

                        while(j < max_clients){
                            if(strcmp(clients[j].nickname, nickname) == 0){

                                if(clients[j].channel == clients[i].channel){
                                    clients[j].channel = -1;
                                    printf("User %s kicked of %s channel\n", nickname, channels[clients[i].channel].name);
                                }

                                break;
                            }
                            j++;
                        }
                    }

                }else if(strcmp(cmd, "/mute") == 0){
                    char* nickname = strtok(NULL, " ");

                    int j = 0;

                    if(channels[clients[i].channel].admin_socket == clients[i].client_socket){

                        while(j < max_clients){
                            if(strcmp(clients[j].nickname, nickname) == 0){

                                if(clients[j].channel == clients[i].channel){
                                    clients[j].mute = 1;
                                    printf("User %s muted in %s channel\n", nickname, channels[clients[i].channel].name);
                                }

                                break;
                            }
                            j++;
                        }
                    }

                }else if(strcmp(cmd, "/unmute") == 0){
                    char* nickname = strtok(NULL, " ");

                    int j = 0;

                    if(channels[clients[i].channel].admin_socket == clients[i].client_socket){

                        while(j < max_clients){
                            if(strcmp(clients[j].nickname, nickname) == 0){

                                if(clients[j].channel == clients[i].channel){
                                    clients[j].mute = 0;
                                    printf("User %s unmuted in %s channel\n", nickname, channels[clients[i].channel].name);
                                }

                                break;
                            }
                            j++;
                        }
                    }

                }else if(strcmp(cmd, "/whois") == 0){
                    char* nickname = strtok(NULL, " ");

                    int j = 0;

                    if(channels[clients[i].channel].admin_socket == clients[i].client_socket){

                        while(j < max_clients){
                            if(strcmp(clients[j].nickname, nickname) == 0){

                                if(clients[j].channel == clients[i].channel){
                                    getpeername(sd , (struct sockaddr*)&address , \
                                                (socklen_t*)&addrlen);

                                    //Mostra ao administrador dados do usuário desejado.
                                    sprintf(str_final, "User %s ip is %s" , nickname,
                                           inet_ntoa(address.sin_addr));

                                    send(clients[i].client_socket, str_final, strlen(str_final)+1, 0);

                                }

                                break;
                            }
                            j++;
                        }
                    }

                }
                else{

                    //Caso contrário alguém mandou uma mensagem para o servidor.
                    printf("Message from client %d received.\n%s\n", i,buffer);

                    //Se a mensagem for o comando ping o servidor manda a mensagem pong para o client que enviou o comando ping.
                    if(strcmp(buffer, "/ping") == 0){
                        strcpy(buffer, "/pong");
                        send(clients[i].client_socket, buffer, strlen(buffer)+1, 0);
                    }else if(clients[i].mute != 1){
                        //Caso contrário mandamos uma mensagem formatada para todos os clients.
                        if(strcmp(clients[i].nickname, "") != 0){
                            sprintf(str_final, "%s: %s", clients[i].nickname, buffer);
                        }else{
                            sprintf(str_final, "Client %d: %s", i, buffer);
                        }

                        for(int j = 0; j < max_clients; j++){
                            if(clients[j].client_socket > 0 && clients[j].channel == clients[i].channel){
                                send(clients[j].client_socket, str_final, strlen(str_final)+1, 0);
                            }
                        }

                    }

                }

            }

        }

    }
    return 0;
}

