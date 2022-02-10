#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 512

unsigned short idServidor = 65535;

unsigned short seqMsgs = 1;

void tratarParametroIncorreto(char *comandoPrograma)
{
    // Imprime o uso correto dos parâmetros do programa e encerra o programa
    printf("Uso: %s <host:porta>\n", comandoPrograma);
    printf("Exemplo (por padrao o servidor usa IPv6): %s ::1:51511\n", comandoPrograma);
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
}

// Envia uma mensagem hi ao servidor
unsigned short enviarHi(int socketCliente)
{
    // Constroi a mensagem hi com o corpo vazio e o id do exibidor
    Mensagem mensagem;
    mensagem.tipo = 3;
    mensagem.idOrigem = 0;
    mensagem.idDestino = idServidor;
    mensagem.numSeq = seqMsgs++;
    mensagem.texto[0] = '\0';

    // Envia a mensagem hi
    printf("> hi\n");
    enviarMensagem(socketCliente, mensagem);

    // Recebe a resposta
    Mensagem resposta = receberMensagem(socketCliente);

    // Caso a resposta seja invalida ou do tipo erro encerra a execução do emissor
    if (!resposta.valida || resposta.tipo == 2)
    {
        sairComMensagem("Erro ao enviar o hi");
    }

    // Retorna o id recebido pelo servidor
    return resposta.idDestino;
}

// Envia o nome do planeta lido para o servidor
void enviarNomePlaneta(std::string nomePlaneta, unsigned short idCliente, int socketCliente)
{
    // Constroi a mensagem origin
    Mensagem mensagem;
    mensagem.tipo = 8;
    mensagem.idOrigem = idCliente;
    mensagem.idDestino = idServidor;
    mensagem.numSeq = seqMsgs++;
    mensagem.texto = nomePlaneta;

    // Envia a mensagem origin
    printf("> origin %s\n", nomePlaneta.c_str());
    enviarMensagem(socketCliente, mensagem);

    // Recebe a resposta
    Mensagem resposta = receberMensagem(socketCliente);

    // Caso a resposta seja invalida ou do tipo erro encerra a execução do emissor
    if (!resposta.valida || resposta.tipo == 2)
    {
        sairComMensagem("Erro ao enviar o origin");
    }
}

// Trata o recebimento de uma mensagem kill
void tratarKill(int socketCliente, unsigned short idCliente, Mensagem mensagem)
{
    // Imprime o comando recebido e de quem ele foi recebido
    printf("< kill de %d\n", mensagem.idOrigem);
    // Envia a mensagem de ok
    Mensagem resposta;
    resposta.tipo = 1; // Mensagem ok
    resposta.idOrigem = idCliente;
    resposta.idDestino = idServidor;
    resposta.numSeq = seqMsgs - 1;
    enviarMensagem(socketCliente, resposta);
}

// Trata o recebimento de uma mensagem do tipo mensagem
void tratarMsg(int socketCliente, unsigned short idCliente, Mensagem mensagem)
{
    // Se a mensagem é de broadcast
    if (mensagem.idDestino == 0)
    {
        // Imprime que uma mensagem de broadcast foi recebida, a origem da mensagem e o seu conteudo
        printf("< msg de broadcast de %d: \"%s\"\n", mensagem.idOrigem, mensagem.texto.c_str());
    }
    else
    {
        // Senao, imprime que uma mensagem foi recebida, a origem da mensagem e o seu conteudo
        printf("< msg de %d: \"%s\"\n", mensagem.idOrigem, mensagem.texto.c_str());
    }
    // Envia a mensagem de ok
    Mensagem resposta;
    resposta.tipo = 1; // Mensagem ok
    resposta.idOrigem = idCliente;
    resposta.idDestino = idServidor;
    resposta.numSeq = seqMsgs++;
    enviarMensagem(socketCliente, resposta);
}

// Trata o recebimento de uma mensagem clist
void tratarCList(int socketCliente, unsigned short idCliente, Mensagem mensagem)
{
    // Imprime o comando recebido, de quem ele foi recebido e o seu conteudo
    printf("< clist de %d: \"%s\"\n", mensagem.idOrigem, mensagem.texto.c_str());
    // Envia a mensagem de ok
    Mensagem resposta;
    resposta.tipo = 1; // Mensagem ok
    resposta.idOrigem = idCliente;
    resposta.idDestino = idServidor;
    resposta.numSeq = seqMsgs++;
    enviarMensagem(socketCliente, resposta);
}

// Trata o recebimento de uma mensagem planet
void tratarPlanet(int socketCliente, unsigned short idCliente, Mensagem mensagem)
{
    // Imprime o comando recebido, de quem ele foi recebido e o seu conteudo
    printf("< planet de %d: \"%s\"\n", mensagem.idDestino, mensagem.texto.c_str());
    // Envia a mensagem de ok
    Mensagem resposta;
    resposta.tipo = 1; // Mensagem ok
    resposta.idOrigem = idCliente;
    resposta.idDestino = idServidor;
    resposta.numSeq = seqMsgs - 1;
    enviarMensagem(socketCliente, resposta);
}

// Trata o recebimento de uma mensagem planetlist
void tratarPlanetList(int socketCliente, unsigned short idCliente, Mensagem mensagem)
{
    // Imprime o comando recebido, de quem ele foi recebido e o seu conteudo
    printf("< planetlist de %d: \"%s\"\n", mensagem.idOrigem, mensagem.texto.c_str());
    // Envia a mensagem de ok
    Mensagem resposta;
    resposta.tipo = 1; // Mensagem ok
    resposta.idOrigem = idCliente;
    resposta.idDestino = idServidor;
    resposta.numSeq = seqMsgs - 1;
    enviarMensagem(socketCliente, resposta);
}

// Trata a comunicacao do cliente com o servidor
void comunicarComServidor(int socketCliente)
{
    // Envia um hi para o servidor e recebe o id
    unsigned short idCliente = enviarHi(socketCliente);

    // Imprime o id recebido
    printf("Id recebido pelo servidor = %d\n", idCliente);

    // Le o nome do planeta do teclado
    printf("Digite o nome do planeta: ");
    std::string nomePlaneta;
    std::cin >> nomePlaneta;

    // Envia o nome do planeta ao servidor
    enviarNomePlaneta(nomePlaneta, idCliente, socketCliente);

    // Laço para a comunicacao, em que o cliente espera por mensagens do servidor
    while (1)
    {
        // Recebe uma mensagem do servidor
        Mensagem mensagem = receberMensagem(socketCliente);
        // Se a mensagem for invalida
        if (!mensagem.valida)
        {
            // Envia uma resposta indicando erro
            Mensagem resposta;
            resposta.tipo = 2; // Mensagem erro
            resposta.idOrigem = idCliente;
            resposta.idDestino = idServidor;
            resposta.numSeq = seqMsgs - 1;
            enviarMensagem(socketCliente, resposta);
            continue;
        }
        // Verifica o tipo da mensagem
        switch (mensagem.tipo)
        {
        case 1:
            // Se for mensagem de ok imprime que foi recebido um ok do cliente de origem
            printf("< ok de %d\n", mensagem.idOrigem);
            break; // OK
        case 2:
            // Se for mensagem de erro imprime que foi recebido um erro do cliente de origem
            printf("< error de %d\n", mensagem.idOrigem);
            break; // ERROR
        case 4:
            // Se for mensagem de kill, chama o tratamento adequado
            tratarKill(socketCliente, idCliente, mensagem);
            return; // KILL
        case 5:
            // Se for mensagem do tipo mensagem, chama o tratamento adequado
            tratarMsg(socketCliente, idCliente, mensagem);
            break; // MSG
        case 7:
            // Se for mensagem de clist, chama o tratamento adequado
            tratarCList(socketCliente, idCliente, mensagem);
            break; // CLIST
        case 9:
            // Se for mensagem de planet, chama o tratamento adequado
            tratarPlanet(socketCliente, idCliente, mensagem);
            break; // PLANET
        case 10:
            // Se for mensagem de planetlist, chama o tratamento adequado
            tratarPlanetList(socketCliente, idCliente, mensagem);
            break; // PLANETLIST
        default:
            // Se for uma mensagem de tipo desconhecido retorna uma resposta de erro
            Mensagem resposta;
            resposta.tipo = 2; // Mensagem de erro
            resposta.idOrigem = idServidor;
            resposta.idDestino = mensagem.idDestino;
            resposta.numSeq = mensagem.numSeq;
            enviarMensagem(socketCliente, resposta);
        }
    }
}

int main(int argc, char **argv)
{
    // Verifica os parametros recebidos
    verificarParametros(argc, argv);
    // Constroi o host e porta a partir da string 'host:porta' recebida
    char host[BUFSZ];
    char *p = strrchr(argv[1], ':');
    *p = '\0';
    strcpy(host, argv[1]);
    p++;
    char porta[BUFSZ];
    strcpy(porta, p);
    // Inicializa os dados do socket
    struct sockaddr_storage dadosSocket;
    inicializarDadosSocket(host, porta, &dadosSocket, argv[0]);
    // Inicializa os dados do socket do cliente
    int socketCliente = inicializarSocketCliente(&dadosSocket);
    // Conecta ao servidor
    conectarAoServidor(&dadosSocket, socketCliente);
    // Comunica com o servidor
    comunicarComServidor(socketCliente);
    // Fecha o socket criado
    close(socketCliente);
    // Encerra o programa
    exit(EXIT_SUCCESS);
}