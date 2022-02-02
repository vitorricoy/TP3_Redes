#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 512

void tratarParametroIncorreto(char* comandoPrograma) {
    // Imprime o uso correto dos parâmetros do programa e encerra o programa
    printf("Uso: %s <ip do servidor> <porta do servidor>\n", comandoPrograma);
    printf("Exemplo: %s 127.0.0.1 51511\n", comandoPrograma);
    exit(EXIT_FAILURE);
}

void verificarParametros(int argc, char **argv) {
    // Caso o programa possua menos de 2 argumentos imprime a mensagem do uso correto de seus parâmetros
    if(argc < 3) {
        tratarParametroIncorreto(argv[0]);
    }
}

void inicializarDadosSocket(const char *enderecoStr, const char *portaStr, struct sockaddr_storage *dadosSocket, char* comandoPrograma) {
    // Se a porta ou o endereço não estiverem certos imprime o uso correto dos parâmetros
    if(enderecoStr == NULL || portaStr == NULL) {
        tratarParametroIncorreto(comandoPrograma);
    }

    // Converte o parâmetro da porta para unsigned short
    unsigned short porta = (unsigned short)atoi(portaStr);

    if(porta == 0) { // Se a porta dada é inválida imprime a mensagem de uso dos parâmetros
        tratarParametroIncorreto(comandoPrograma);
    }

    porta = htons(porta); // Converte a porta para network short

    struct in_addr inaddr4; // Struct do IPv4
    if(inet_pton(AF_INET, enderecoStr, &inaddr4)) { // Tenta criar um socket IPv4 com os parâmetros
        struct sockaddr_in *dadosSocketv4 = (struct sockaddr_in*)dadosSocket;
        dadosSocketv4->sin_family = AF_INET;
        dadosSocketv4->sin_port = porta;
        dadosSocketv4->sin_addr = inaddr4;
    } else {
        struct in6_addr inaddr6; // Struct do IPv6
        if (inet_pton(AF_INET6, enderecoStr, &inaddr6)) { // Tenta criar um socket IPv6 com os parâmetros
            struct sockaddr_in6 *dadosSocketv6 = (struct sockaddr_in6*)dadosSocket;
            dadosSocketv6->sin6_family = AF_INET6;
            dadosSocketv6->sin6_port = porta;
            memcpy(&(dadosSocketv6->sin6_addr), &inaddr6, sizeof(inaddr6));
        } else { // Parâmetros incorretos, pois não é IPv4 nem IPv6
            tratarParametroIncorreto(comandoPrograma);
        }
    }
}


int inicializarSocketCliente(struct sockaddr_storage* dadosSocket) {
    // Inicializa o socket do cliente por meio da função socket
    int socketCliente;
    socketCliente = socket(dadosSocket->ss_family, SOCK_STREAM, 0);
    if(socketCliente == -1) {
        sairComMensagem("Erro ao iniciar o socket");
    }
    return socketCliente;
}

void conectarAoServidor(struct sockaddr_storage* dadosSocket, int socketCliente) {
    // Tenta conectar ao servidor especificado nos parâmetros
    struct sockaddr *enderecoSocket = (struct sockaddr*)(dadosSocket);
    if(connect(socketCliente, enderecoSocket, sizeof(*dadosSocket)) != 0) {
        sairComMensagem("Erro ao conectar no servidor");
    }
    // Printa um log para debug TODO
    char enderecoStr[BUFSZ];
    converterEnderecoParaString(enderecoSocket, enderecoStr, BUFSZ);
    printf("Conectado ao endereco %s\n", enderecoStr);
}

void leMensagemEntrada(char mensagem[BUFSZ]) {
    // Lê a mensagem digitada e a salva em 'mensagem'
    memset(mensagem, 0, sizeof(*mensagem));
    fgets(mensagem, BUFSZ-1, stdin);
    // Caso a mensagem lida não termine com \n coloca um \n
    if(mensagem[strlen(mensagem)-1] != '\n') {
        strcat(mensagem, "\n");
    }
}

void enviaMensagemServidor(int socketCliente, char mensagem[BUFSZ]) {
    // Envia o parâmetro 'mensagem' para o servidor
    size_t tamanhoMensagemEnviada = send(socketCliente, mensagem, strlen(mensagem), 0);
    if (strlen(mensagem) != tamanhoMensagemEnviada) {
        sairComMensagem("Erro ao enviar mensagem");
    }
}

void recebeMensagemServidor(int socketCliente, char mensagem[BUFSZ]) {
    memset(mensagem, 0, BUFSZ);
    size_t tamanhoMensagem = 0;
    // Recebe mensagens do servidor enquanto elas não terminarem com \n
    do {
        size_t tamanhoLidoAgora = recv(socketCliente, mensagem+tamanhoMensagem, BUFSZ-(int)tamanhoMensagem-1, 0);
        if(tamanhoLidoAgora == 0) {
            break;
        }
        tamanhoMensagem += tamanhoLidoAgora;
    }while(mensagem[strlen(mensagem)-1] != '\n');
    mensagem[tamanhoMensagem] = '\0';

    // Caso a mensagem lida tenha tamanho zero, o servidor foi desconectado
    if(strlen(mensagem) == 0) {
        // Conexão caiu
        exit(EXIT_SUCCESS);
    }
}

void comunicarComServidor(int socketCliente) {
    // Laço para a comunicação do cliente com o servidor
    char mensagem[BUFSZ];
    while(1) {
        leMensagemEntrada(mensagem);
        enviaMensagemServidor(socketCliente, mensagem);
        recebeMensagemServidor(socketCliente, mensagem);
        printf("%s", mensagem);
    }
}

int main(int argc, char **argv) {
    verificarParametros(argc, argv);
    struct sockaddr_storage dadosSocket;
    inicializarDadosSocket(argv[1], argv[2], &dadosSocket, argv[0]);
    int socketCliente = inicializarSocketCliente(&dadosSocket);
    conectarAoServidor(&dadosSocket, socketCliente);
    comunicarComServidor(socketCliente);
	exit(EXIT_SUCCESS);
}