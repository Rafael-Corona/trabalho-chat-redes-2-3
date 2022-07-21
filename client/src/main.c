#include <arpa/inet.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include<signal.h>
#define PORT 8080

//Struct que usaremos para passarmos os parametros para a função readind.
typedef struct{
    int sock;
    char* buffer;
}args;

//Função que lidará com Ctrl+C, ou seja, esse sinal não fará nada.
void sigint_handler(int a){
    return;
}

//Função que lê uma string do usuário até o mesmo digitar 4096 caracteres ou Enter.
void read_strings(char* buffer){
    int tam = 0;
    char c = getchar();

    while(tam < 4096 && c != '\n' && c != EOF){
        buffer[tam] = c;
        tam++;
        c = getchar();
    }

    if(c == EOF){
        printf("Application finished!\n");
        exit(EXIT_SUCCESS);
    }

    buffer[tam] = '\0';

}

//Função responsável por enviar mensagens que o usuário digita para o servidor.
void* send_message(void* b){

    args* s = (args*) b;
    int aux = 0;

    while(aux == 0){

        __fpurge(stdin);
        read_strings(s->buffer);

        if(strcmp(s->buffer, "/help") == 0){
        printf("/ping - The server returns pong when receive this message\n");
            printf("/join <channelName> - Enters the channel. The first user to join the channel will be it's administrator\n");
            printf("/nickname <desiredNickname> - The client will now be called by the chosen nickname\n");
            printf("/help - Shows this help message again\n\n");
            printf("Administrator restricted commands:\n");
            printf("/kick <userName> - Kicks user from channel\n");
            printf("/mute <userName> - Mutes user on channel\n");
            printf("/unmute <userName> - Unmutes user on channel\n");
            printf("/whois <userName> - shows user IP only to administrator\n");
            continue;
        }
        //Se o usuário digitar o comando /quit finalizamos o cliente.
        else if(strcmp(s->buffer, "/quit") == 0){
            close(s->sock);
            printf("Application finished!\n");
            exit(EXIT_SUCCESS);
        }
        if(strlen(s->buffer) > 0){
            send(s->sock, s->buffer, strlen(s->buffer)+1, 0);
        }
    }

    return NULL;

}

int main(){

    //Adicionamos um handler para o sinal Ctrl+C.
    signal(SIGINT, sigint_handler);

    int sock = 0, client_fd;
    struct sockaddr_in serv_addr;
    char buffer[4097] = { 0 };

    //Cria o socket e verifica se o mesmo foi criado com sucesso.
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    char server_ip[30];

    serv_addr.sin_family = AF_INET; //IPV4
    serv_addr.sin_port = htons(PORT);   //Porta 8080.

    printf("Please type the command \"/connect\" and the local ip of the server's host: ");

    scanf("%s", buffer);

    //Enquanto o comando /connect não for digitado não se conecta ao servidor desejado.
    while(strcmp(buffer, "/connect") != 0){
        printf("Command not identified, please write a valid command:\n");
        scanf("%s", buffer);
    }

    //Pedimos para o usuário digitar o IP local do host do servidor.
    scanf("%s", server_ip);

    // Convertemos o endereço IPv4 de texto pra binário.
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    //Conectamos o cliente ao servidor e verificamos se a conexão foi realizada com sucesso.
    if ((client_fd = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0){

        printf("\nConnection Failed \n");
        return -1;

    } else {

        printf("Connected to server!\n");
        printf("Type the command \"/help\" to show avaliable commands\n");
    }

    args s;
    s.sock = sock;
    s.buffer = buffer;


    //Criamos uma thread que será responsável por mandar mensagens para o servidor.
    pthread_t id;
    pthread_create(&id, NULL, send_message, &s);

    //Loop responsável por receber mensagens do servidor.
    while(1){

        memset(buffer, '\0' , sizeof (buffer));

        //Verificamos se a mensagem recebida não é vazia.
        if(recv(sock, buffer, sizeof buffer, 0) != 1){

            //Printamos a mensagem recebida do servidor para o usuário.
            printf("%s\n", buffer);
        }
    }

    return 0;
}
