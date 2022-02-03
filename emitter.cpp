#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <string>
#include <iostream>
#include <sstream>

#define BUFSZ 512

int seqMsgs = 1;

unsigned short idServidor = 65535;

int idExibidor = 16384;

void tratarParametroIncorreto(char *comandoPrograma)
{
    // Imprime o uso correto dos parâmetros do programa e encerra o programa
    printf("Uso: %s <host:porta> <id_exibidor>\n", comandoPrograma);
    printf("Exemplo (por padrao o servidor usa IPv6): %s ::1:51511 4096\n", comandoPrograma);
    exit(EXIT_FAILURE);
}

void verificarParametros(int argc, char **argv)
{
    // Caso o programa possua menos de 2 argumentos imprime a mensagem do uso correto de seus parâmetros
    if (argc < 2)
    {
        tratarParametroIncorreto(argv[0]);
    }
}

void inicializarDadosSocket(const char *enderecoStr, const char *portaStr, struct sockaddr_storage *dadosSocket, char *comandoPrograma)
{
    // Se a porta ou o endereço não estiverem certos imprime o uso correto dos parâmetros
    if (enderecoStr == NULL || portaStr == NULL)
    {
        tratarParametroIncorreto(comandoPrograma);
    }

    // Converte o parâmetro da porta para unsigned short
    unsigned short porta = (unsigned short)atoi(portaStr);

    if (porta == 0)
    { // Se a porta dada é inválida imprime a mensagem de uso dos parâmetros
        tratarParametroIncorreto(comandoPrograma);
    }

    porta = htons(porta); // Converte a porta para network short

    struct in_addr inaddr4; // Struct do IPv4
    if (inet_pton(AF_INET, enderecoStr, &inaddr4))
    { // Tenta criar um socket IPv4 com os parâmetros
        struct sockaddr_in *dadosSocketv4 = (struct sockaddr_in *)dadosSocket;
        dadosSocketv4->sin_family = AF_INET;
        dadosSocketv4->sin_port = porta;
        dadosSocketv4->sin_addr = inaddr4;
    }
    else
    {
        struct in6_addr inaddr6; // Struct do IPv6
        if (inet_pton(AF_INET6, enderecoStr, &inaddr6))
        { // Tenta criar um socket IPv6 com os parâmetros
            struct sockaddr_in6 *dadosSocketv6 = (struct sockaddr_in6 *)dadosSocket;
            dadosSocketv6->sin6_family = AF_INET6;
            dadosSocketv6->sin6_port = porta;
            memcpy(&(dadosSocketv6->sin6_addr), &inaddr6, sizeof(inaddr6));
        }
        else
        { // Parâmetros incorretos, pois não é IPv4 nem IPv6
            tratarParametroIncorreto(comandoPrograma);
        }
    }
}

int inicializarSocketCliente(struct sockaddr_storage *dadosSocket)
{
    // Inicializa o socket do cliente por meio da função socket
    int socketCliente;
    socketCliente = socket(dadosSocket->ss_family, SOCK_STREAM, 0);
    if (socketCliente == -1)
    {
        sairComMensagem("Erro ao iniciar o socket");
    }
    return socketCliente;
}

void conectarAoServidor(struct sockaddr_storage *dadosSocket, int socketCliente)
{
    // Tenta conectar ao servidor especificado nos parâmetros
    struct sockaddr *enderecoSocket = (struct sockaddr *)(dadosSocket);
    if (connect(socketCliente, enderecoSocket, sizeof(*dadosSocket)) != 0)
    {
        sairComMensagem("Erro ao conectar no servidor");
    }
    // Printa um log para debug TODO
    char enderecoStr[BUFSZ];
    converterEnderecoParaString(enderecoSocket, enderecoStr, BUFSZ);
    printf("Conectado ao endereco %s\n", enderecoStr);
}

void leMensagemEntrada(char mensagem[BUFSZ])
{
    // Lê a mensagem digitada e a salva em 'mensagem'
    memset(mensagem, 0, sizeof(*mensagem));
    fgets(mensagem, BUFSZ - 1, stdin);
    // Caso a mensagem lida não termine com \n coloca um \n
    if (mensagem[strlen(mensagem) - 1] != '\n')
    {
        strcat(mensagem, "\n");
    }
}

unsigned short enviarHi(int socketCliente)
{
    Mensagem mensagem;
    mensagem.tipo = 3;
    mensagem.idOrigem = idExibidor;
    mensagem.idDestino = idServidor;
    mensagem.numSeq = seqMsgs++;
    mensagem.texto[0] = '\0';
    printf("> hi\n");
    enviarMensagem(socketCliente, mensagem);
    Mensagem resposta = receberMensagem(socketCliente);
    if (!resposta.valida || resposta.tipo == 2)
    {
        sairComMensagem("Erro ao enviar o hi");
    }
    printf("< ok\n");
    return resposta.idDestino;
}

void enviarNomePlaneta(std::string nomePlaneta, unsigned short idCliente, int socketCliente)
{
    Mensagem mensagem;
    mensagem.tipo = 8;
    mensagem.idOrigem = idCliente;
    mensagem.idDestino = idServidor;
    mensagem.numSeq = seqMsgs++;
    printf("> origin %s\n", nomePlaneta.c_str());
    mensagem.texto = nomePlaneta;
    enviarMensagem(socketCliente, mensagem);
    Mensagem resposta = receberMensagem(socketCliente);
    if (!resposta.valida || resposta.tipo == 2)
    {
        sairComMensagem("Erro ao enviar o origin");
    }
    printf("< ok\n");
}

void comunicarComServidor(int socketCliente)
{
    unsigned short idCliente = enviarHi(socketCliente);
    printf("Id recebido pelo servidor = %d\n", idCliente);
    printf("Digite o nome do planeta: ");
    std::string nomePlaneta;
    std::cin >> nomePlaneta;
    std::cin.ignore();
    enviarNomePlaneta(nomePlaneta, idCliente, socketCliente);
    // Laço para a comunicação do cliente com o servidor
    while (1)
    {
        std::string entrada;
        printf("> ");
        getline(std::cin, entrada);
        std::stringstream ss(entrada);
        std::string operacao;
        ss >> operacao;
        if (operacao == "msg")
        {
            unsigned short destino;
            ss >> destino;
            char lixo;
            ss >> std::noskipws >> lixo >> std::skipws;
            std::string texto;
            getline(ss, texto);
            Mensagem mensagem;
            mensagem.tipo = 5;
            mensagem.idOrigem = idCliente;
            mensagem.idDestino = destino;
            mensagem.numSeq = seqMsgs++;
            mensagem.texto = texto;
            enviarMensagem(socketCliente, mensagem);
            Mensagem resposta = receberMensagem(socketCliente);
            if (resposta.valida && resposta.tipo == 1)
            {
                printf("< ok\n");
            }
            else
            {
                printf("< error\n");
            }
        }
        else if (operacao == "creq")
        {
            unsigned short destino;
            ss >> destino;
            Mensagem mensagem;
            mensagem.tipo = 6;
            mensagem.idOrigem = idCliente;
            mensagem.idDestino = destino;
            mensagem.numSeq = seqMsgs++;
            enviarMensagem(socketCliente, mensagem);
            Mensagem resposta = receberMensagem(socketCliente);
            if (resposta.valida && resposta.tipo == 1)
            {
                printf("< ok\n");
            }
            else
            {
                printf("< error\n");
            }
        }
        else if (operacao == "planet")
        {
            unsigned short destino;
            ss >> destino;
            Mensagem mensagem;
            mensagem.tipo = 9;
            mensagem.idOrigem = idCliente;
            mensagem.idDestino = destino;
            mensagem.numSeq = seqMsgs++;
            enviarMensagem(socketCliente, mensagem);
            Mensagem resposta = receberMensagem(socketCliente);
            if (resposta.valida && resposta.tipo == 1)
            {
                printf("< ok\n");
            }
            else
            {
                printf("< error\n");
            }
        }
        else if (operacao == "kill")
        {
            unsigned short destino;
            ss >> destino;
            Mensagem mensagem;
            mensagem.tipo = 4;
            mensagem.idOrigem = idCliente;
            mensagem.idDestino = idServidor;
            mensagem.numSeq = seqMsgs++;
            enviarMensagem(socketCliente, mensagem);
            Mensagem resposta = receberMensagem(socketCliente);
            if (resposta.valida && resposta.tipo == 1)
            {
                printf("< ok\n");
            }
            else
            {
                printf("< error\n");
            }
            return;
        }
        else if (operacao == "planetlist")
        {
            unsigned short destino;
            ss >> destino;
            Mensagem mensagem;
            mensagem.tipo = 10;
            mensagem.idOrigem = idCliente;
            mensagem.idDestino = destino;
            mensagem.numSeq = seqMsgs++;
            enviarMensagem(socketCliente, mensagem);
            Mensagem resposta = receberMensagem(socketCliente);
            if (resposta.valida && resposta.tipo == 1)
            {
                printf("< ok\n");
            }
            else
            {
                printf("< error\n");
            }
        }
        else
        {
            printf("Comando inválido!\n");
        }
    }
}

int main(int argc, char **argv)
{
    verificarParametros(argc, argv);
    char host[BUFSZ];
    char *p = strrchr(argv[1], ':');
    *p = '\0';
    strcpy(host, argv[1]);
    p++;
    char porta[BUFSZ];
    strcpy(porta, p);
    if (argc > 2)
    {
        idExibidor = atoi(argv[2]);
    }
    struct sockaddr_storage dadosSocket;
    inicializarDadosSocket(host, porta, &dadosSocket, argv[0]);
    int socketCliente = inicializarSocketCliente(&dadosSocket);
    conectarAoServidor(&dadosSocket, socketCliente);
    comunicarComServidor(socketCliente);
    exit(EXIT_SUCCESS);
}