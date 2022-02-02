#include "common.h"
#include "pokedex.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <thread>
#include <map>

#define BUFSZ 512
#define TAM_POKEDEX 40

#define SUCESSO 0
#define PROXIMA_MENSAGEM 1
#define ENCERRAR 2

std::map<int, int> mapaEmissorExibidor;

unsigned short proxIdEmissor = 1;
unsigned short proxIdExibidor = 4096;
unsigned short idServidor = 65535;

std::map<int, int> mapaClienteSocket;
std::set<int> exibidores, emissores;

void tratarParametroIncorreto(char *comandoPrograma)
{
    // Imprime o uso correto dos parâmetros do programa e encerra o programa
    printf("Uso: %s <v4|v6> <porta do servidor>\n", comandoPrograma);
    printf("Exemplo: %s v4 51511\n", comandoPrograma);
    exit(EXIT_FAILURE);
}

void verificarParametros(int argc, char **argv)
{
    // Caso o programa possua menos de 2 argumentos imprime a mensagem do uso correto de seus parâmetros
    if (argc < 3)
    {
        tratarParametroIncorreto(argv[0]);
    }
}

void inicializarDadosSocket(const char *protocolo, const char *portaStr, struct sockaddr_storage *dadosSocket, char *comandoPrograma)
{
    // Caso a porta seja nula, imprime o uso correto dos parâmetros
    if (portaStr == NULL)
    {
        tratarParametroIncorreto(comandoPrograma);
    }

    // Converte o parâmetro da porta para unsigned short
    unsigned short porta = (unsigned short)atoi(portaStr); // unsigned short

    if (porta == 0)
    { // Se a porta dada é inválida imprime a mensagem de uso dos parâmetros
        tratarParametroIncorreto(comandoPrograma);
    }

    // Converte a porta para network short
    porta = htons(porta);

    // Inicializa a estrutura de dados do socket
    memset(dadosSocket, 0, sizeof(*dadosSocket));
    if (strcmp(protocolo, "v4") == 0)
    { // Verifica se o protocolo escolhido é IPv4
        // Inicializa a estrutura do IPv4
        struct sockaddr_in *dadosSocketv4 = (struct sockaddr_in *)dadosSocket;
        dadosSocketv4->sin_family = AF_INET;
        dadosSocketv4->sin_addr.s_addr = INADDR_ANY;
        dadosSocketv4->sin_port = porta;
    }
    else
    {
        if (strcmp(protocolo, "v6") == 0)
        { // Verifica se o protocolo escolhido é IPv6
            // Inicializa a estrutura do IPv6
            struct sockaddr_in6 *dadosSocketv6 = (struct sockaddr_in6 *)dadosSocket;
            dadosSocketv6->sin6_family = AF_INET6;
            dadosSocketv6->sin6_addr = in6addr_any;
            dadosSocketv6->sin6_port = porta;
        }
        else
        { // O parâmetro do protocolo não era 'v4' ou 'v6', portanto imprime o uso correto dos parâmetros
            tratarParametroIncorreto(comandoPrograma);
        }
    }
}

int inicializarSocketServidor(struct sockaddr_storage *dadosSocket)
{
    // Inicializa o socket do servidor
    int socketServidor;
    socketServidor = socket(dadosSocket->ss_family, SOCK_STREAM, 0);

    if (socketServidor == -1)
    {
        sairComMensagem("Erro ao iniciar o socket");
    }

    // Define as opções do socket do servidor
    int enable = 1;
    if (setsockopt(socketServidor, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0)
    {
        sairComMensagem("Erro ao definir as opcoes do socket");
    }

    return socketServidor;
}

void ativarEscutaPorConexoes(int socketServidor, struct sockaddr_storage *dadosSocket)
{
    struct sockaddr *enderecoSocket = (struct sockaddr *)(dadosSocket);

    // Dá bind do servidor na porta recebida por parâmetro
    if (bind(socketServidor, enderecoSocket, sizeof(*dadosSocket)) != 0)
    {
        sairComMensagem("Erro ao dar bind no endereço e porta para o socket");
    }

    // Faz o servidor escutar por conexões
    // Limite de 100 conexões na fila de espera
    if (listen(socketServidor, 100) != 0)
    {
        sairComMensagem("Erro ao escutar por conexoes no servidor");
    }
}

void tratarMensagemHi(Mensagem mensagem, int socketCliente)
{
    if (mensagem.idOrigem == 0)
    { // Exibidor
        int idExibidor = proxIdExibidor++;
        exibidores.insert(idExibidor);
        printf("Recebido HI de %d\n", idExibidor);
        Mensagem resposta;
        resposta.tipo = 1; // Mensagem ok
        resposta.idOrigem = idServidor;
        resposta.idDestino = idExibidor;
        resposta.numSeq = mensagem.numSeq;
        resposta.texto[0] = '\0';
        mapaClienteSocket[idExibidor] = socketCliente;
        enviarMensagem(socketCliente, resposta);
    }
    else
    { // Emissor
        int idEmissor = proxIdEmissor++;
        emissores.insert(idEmissor);
        printf("Recebido HI de %d\n", idEmissor);
        mapaClienteSocket[idEmissor] = socketCliente;
        if (mensagem.idOrigem >= 4096 && mensagem.idOrigem <= 8191)
        { // Valor entre 2^12 e 2^13-1
            if (mapaClienteSocket.find(mensage.idOrigem) == mapaClienteSocket.end())
            { // Exibidor não existe
                Mensagem resposta;
                resposta.tipo = 2; // Mensagem de erro
                resposta.idOrigem = idServidor;
                resposta.idDestino = idEmissor;
                resposta.numSeq = mensagem.numSeq;
                resposta.texto[0] = '\0';
                enviarMensagem(socketCliente, resposta);
                return;
            }
            mapaEmissorExibidor[idEmissor] = mensagem.idOrigem;
        }
        Mensagem resposta;
        resposta.tipo = 1; // Mensagem ok
        resposta.idOrigem = idServidor;
        resposta.idDestino = idExibidor;
        resposta.numSeq = mensagem.numSeq;
        resposta.texto[0] = '\0';
        mapaClienteSocket[idExibidor] = socketCliente;
        enviarMensagem(socketCliente, resposta);
    }
}

void tratarMensagemKill(Mensagem mensagem, int socketCliente)
{
    if (emissores.find(mensagem.idOrigem) != emissores.end())
    {
        if (mapaEmissorExibidor.find(mensagem.idOrigem) != mapaEmissorExibidor.end())
        { // Se o emissor tem exibidor associado
            Mensagem resposta;
            resposta.tipo = mensagem.tipo;
            resposta.idOrigem = idServidor;
            resposta.idDestino = mapaEmissorExibidor[mensagem.idOrigem];
            resposta.numSeq = mensagem.numSeq;
            resposta.texto[0] = '\0';
            enviarMensagem(mapaClienteSocket[resposta.idDestino], resposta);
            close(mapaClienteSocket[resposta.idDestino]); // Fecha o socket do exibidor
        }
        Mensagem resposta;
        resposta.tipo = 1; // Mensagem ok
        resposta.idOrigem = idServidor;
        resposta.idDestino = mensagem.idOrigem;
        resposta.numSeq = mensagem.numSeq;
        resposta.texto[0] = '\0';
        mapaClienteSocket[idExibidor] = socketCliente;
        enviarMensagem(socketCliente, resposta);
    }
}

void tratarMensagemMsg(Mensagem mensagem, int socketCliente)
{
    if (mensagem.idOrigem)
    {
    }
    CONTINUAR
}

int tratarMensagemRecebida(Mensagem mensagem, int socketCliente)
{
    switch (mensagem.tipo)
    {
    case 1:
        printf("Recebido OK de %d\n", mensagem.idOrigem);
        break; //OK
    case 2:
        printf("Recebido ERROR de %d\n", mensagem.idOrigem);
        break; //ERROR
    case 3:
        tratarMensagemHi(mensagem, socketCliente);
        break; //HI
    case 4:
        tratarMensagemKill(mensagem, socketCliente);
        return ENCERRAR; // KILL
    case 5:
        tratarMensagemMsg(mensagem, socketCliente);
        break; //MSG
    case 6:
        break; //CREQ
    case 8:
        break; //ORIGIN
    case 9:
        break; //PLANET
    case 10:
        break; //PLANETLIST
    default:
        Mensagem resposta;
        resposta.tipo = 2; // Mensagem de erro
        resposta.idOrigem = idServidor;
        resposta.idDestino = mensagem.idDestino;
        resposta.numSeq = mensagem.numSeq;
        resposta.texto[0] = '\0';
        enviarMensagem(socketCliente, resposta);
    }
}

int receberETratarMensagemCliente(int socketCliente)
{
    char mensagem[BUFSZ];
    Mensagem mensagem = receberMensagem(socketCliente);

    if (!mensagem.valida)
    {
        return ENCERRAR;
    }

    int retorno = tratarMensagemRecebida(mensagem, socketCliente);

    if (retorno == ENCERRAR)
    { // Se o servidor deve encerrar ou se conectar com outro cliente
        return retorno;
    }
    // Identifica que o servidor deve receber o próximo recv do cliente
    return PROXIMA_COMUNICACAO;
}

void tratarConexaoCliente(int socketCliente)
{
    // Enquanto a conexão com o cliente esteja ativa
    while (1)
    {
        int retorno = receberETratarMensagemCliente(socketCliente);
        if (retorno == ENCERRAR)
        { // Se o servidor deve encerrar ou se conectar com outro cliente
            // Encerra a conexão com o cliente
            close(socketCliente);
            return retorno;
        }
    }
    // Encerra a conexão com o cliente
    close(socketCliente);
    // Indica que o servidor deve se conectar com outro cliente
    return PROXIMO_CLIENTE;
}

void esperarPorConexoesCliente(int socketServidor)
{
    // Enquanto o servidor estiver ativo ele deve receber conexões de clientes
    while (1)
    {
        int socketCliente = esperarConexoesClientes(socketServidor);
        std::thread cliente(tratarConexaoCliente, socketCliente)
    }
}

int main(int argc, char **argv)
{

    verificarParametros(argc, argv);

    struct sockaddr_storage dadosSocket;

    inicializarDadosSocket(argv[1], argv[2], &dadosSocket, argv[0]);

    int socketServidor = inicializarSocketServidor(&dadosSocket);

    ativarEscutaPorConexoes(socketServidor, &dadosSocket);

    esperarPorConexoesCliente(socketServidor);

    exit(EXIT_SUCCESS);
}