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

// Envia uma mensagem hi ao servidor
unsigned short enviarHi(int socketCliente)
{
    // Constroi a mensagem hi com o corpo vazio e o id do exibidor
    Mensagem mensagem;
    mensagem.tipo = 3;
    mensagem.idOrigem = idExibidor;
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

    // Imprime que foi recebido um ok
    printf("< ok\n");

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

    // Imprime que foi recebido um ok
    printf("< ok\n");
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
    std::cin.ignore();

    // Envia o nome do planeta ao servidor
    enviarNomePlaneta(nomePlaneta, idCliente, socketCliente);

    // Laco de leitura de comandos da entrada
    while (1)
    {
        // Le o comando d aentrada
        std::string entrada;
        printf("> ");
        getline(std::cin, entrada);
        std::stringstream ss(entrada);

        // Le a operacao do comando recebido
        std::string operacao;
        ss >> operacao;

        // Se for um comando de mensagem
        if (operacao == "msg")
        {
            // Le o destino da mensagem
            unsigned short destino;
            ss >> destino;
            // Descarta o espaço que separa o destino do conteudo da mensagem
            char lixo;
            ss >> std::noskipws >> lixo >> std::skipws;
            // Le o conteudo da mensagem
            std::string texto;
            getline(ss, texto);
            // Constroi a mensagem que sera enviada
            Mensagem mensagem;
            mensagem.tipo = 5;
            mensagem.idOrigem = idCliente;
            mensagem.idDestino = destino;
            mensagem.numSeq = seqMsgs++;
            mensagem.texto = texto;
            // Envia a mensagem construida
            enviarMensagem(socketCliente, mensagem);
            // Recebe a resposta
            Mensagem resposta = receberMensagem(socketCliente);
            // Se ela for valida e de ok, imprime ok
            if (resposta.valida && resposta.tipo == 1)
            {
                printf("< ok\n");
            }
            else
            {
                // Senao imprime erro
                printf("< error\n");
            }
        }
        else if (operacao == "creq")
        {
            // Se for um comando de creq, le o destino
            unsigned short destino;
            ss >> destino;
            // Constroi a mensagem a ser enviada
            Mensagem mensagem;
            mensagem.tipo = 6;
            mensagem.idOrigem = idCliente;
            mensagem.idDestino = destino;
            mensagem.numSeq = seqMsgs++;
            // Envia a mensagem
            enviarMensagem(socketCliente, mensagem);
            // Recebe a resposta
            Mensagem resposta = receberMensagem(socketCliente);
            // Se ela for valida e de ok, imprime ok
            if (resposta.valida && resposta.tipo == 1)
            {
                printf("< ok\n");
            }
            else
            {
                // Senao imprime erro
                printf("< error\n");
            }
        }
        else if (operacao == "planet")
        {
            // Se for um comando de planet, le o destino
            unsigned short destino;
            ss >> destino;
            // Constroi a mensagem a ser enviada
            Mensagem mensagem;
            mensagem.tipo = 9;
            mensagem.idOrigem = idCliente;
            mensagem.idDestino = destino;
            mensagem.numSeq = seqMsgs++;
            // Envia a mensagem
            enviarMensagem(socketCliente, mensagem);
            // Recebe a resposta
            Mensagem resposta = receberMensagem(socketCliente);
            // Se ela for valida e de ok, imprime ok
            if (resposta.valida && resposta.tipo == 1)
            {
                printf("< ok\n");
            }
            else
            {
                // Senao imprime erro
                printf("< error\n");
            }
        }
        else if (operacao == "kill")
        {
            // Se for um comando de kill
            // Constroi a mensagem a ser enviada
            Mensagem mensagem;
            mensagem.tipo = 4;
            mensagem.idOrigem = idCliente;
            mensagem.idDestino = idServidor;
            mensagem.numSeq = seqMsgs++;
            // Envia a mensagem
            enviarMensagem(socketCliente, mensagem);
            // Recebe a resposta
            Mensagem resposta = receberMensagem(socketCliente);
            // Se ela for valida e de ok, imprime ok
            if (resposta.valida && resposta.tipo == 1)
            {
                printf("< ok\n");
            }
            else
            {
                // Senao imprime erro
                printf("< error\n");
            }
            // Encerra a execucao do emissor
            return;
        }
        else if (operacao == "planetlist")
        {
            // Se for um comando de planetlist
            // Constroi a mensagem a ser enviada
            Mensagem mensagem;
            mensagem.tipo = 10;
            mensagem.idOrigem = idCliente;
            mensagem.idDestino = idServidor;
            mensagem.numSeq = seqMsgs++;
            // Envia a mensagem
            enviarMensagem(socketCliente, mensagem);
            // Recebe a resposta
            Mensagem resposta = receberMensagem(socketCliente);
            // Se ela for valida e de ok, imprime ok
            if (resposta.valida && resposta.tipo == 1)
            {
                printf("< ok\n");
            }
            else
            {
                // Senao imprime erro
                printf("< error\n");
            }
        }
        else
        {
            // Se o comando nao for um comando valido, imprime que ele é invalido
            printf("Comando inválido!\n");
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
    // Se um id de exibidor foi passado por parametro
    if (argc > 2)
    {
        // Salva o id do exibidor recebido
        idExibidor = atoi(argv[2]);
    }
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