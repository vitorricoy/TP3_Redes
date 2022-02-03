#include "common.h"
#include "estruturas.h"

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
#include <set>
#include <string>

#define SUCESSO 0
#define PROXIMA_MENSAGEM 1
#define ENCERRAR 2

#define BUFSZ 512

const unsigned short idServidor = 65535;

Estruturas estruturas;

void tratarParametroIncorreto(char *comandoPrograma)
{
    // Imprime o uso correto dos parâmetros do programa e encerra o programa
    printf("Uso: %s <porta do servidor>\n", comandoPrograma);
    printf("Exemplo: %s 51511\n", comandoPrograma);
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

void enviarErro(Mensagem mensagem, int destId, int socketCliente)
{
    Mensagem resposta;
    resposta.tipo = 2; // Mensagem de erro
    resposta.idOrigem = idServidor;
    resposta.idDestino = destId;
    resposta.numSeq = mensagem.numSeq;
    enviarMensagem(socketCliente, resposta);
}

void enviarOk(Mensagem mensagem, int destId, int socketCliente)
{
    Mensagem resposta;
    resposta.tipo = 1; // Mensagem ok
    resposta.idOrigem = idServidor;
    resposta.idDestino = destId;
    resposta.numSeq = mensagem.numSeq;
    enviarMensagem(socketCliente, resposta);
}

void tratarMensagemHi(Mensagem mensagem, int socketCliente)
{
    if (mensagem.idOrigem == 0)
    { // Exibidor
        int idExibidor = estruturas.obterProximoIdExibidor();
        printf("Recebido HI de %d com origem %d\n", idExibidor, mensagem.idOrigem);
        estruturas.insereNovoExibidor(idExibidor, socketCliente);
        Mensagem resposta;
        resposta.tipo = 1; // Mensagem ok
        resposta.idOrigem = idServidor;
        resposta.idDestino = idExibidor;
        resposta.numSeq = mensagem.numSeq;
        enviarMensagem(socketCliente, resposta);
    }
    else
    { // Emissor
        int idEmissor = estruturas.obterProximoIdEmissor();
        printf("Recebido HI de %d com origem %d\n", idEmissor, mensagem.idOrigem);
        bool sucesso = estruturas.insereNovoEmissor(idEmissor, socketCliente, mensagem.idOrigem);
        if (sucesso)
        {
            enviarOk(mensagem, idEmissor, socketCliente);
        }
        else
        {
            enviarErro(mensagem, idEmissor, socketCliente);
        }
    }
}

void tratarMensagemKill(Mensagem mensagem, int socketCliente)
{
    printf("Recebido KILL de %d\n", mensagem.idOrigem);
    if (estruturas.isEmissor(mensagem.idOrigem))
    {
        if (estruturas.temExibidorAssociado(mensagem.idOrigem))
        { // Se o emissor tem exibidor associado
            Mensagem msgKill;
            msgKill.tipo = mensagem.tipo;
            msgKill.idOrigem = mensagem.idOrigem;
            msgKill.idDestino = estruturas.obterExibidorAssociado(mensagem.idOrigem);
            msgKill.numSeq = mensagem.numSeq;
            enviarMensagem(estruturas.obterSocket(msgKill.idDestino), msgKill);
            Mensagem confirmacao = receberMensagem(estruturas.obterSocket(msgKill.idDestino));
            if (!confirmacao.valida || confirmacao.tipo == 2)
            {
                printf("Erro ao receber a confirmação do KILL do cliente %d\n", msgKill.idDestino);
            }
            close(estruturas.obterSocket(msgKill.idDestino)); // Fecha o socket do exibidor
            estruturas.removeExibidor(msgKill.idDestino);
        }
        enviarOk(mensagem, mensagem.idOrigem, socketCliente);
        close(estruturas.obterSocket(mensagem.idOrigem)); // Fecha o socket do emissor
        estruturas.removeEmissor(mensagem.idOrigem);
    }
}

void tratarMensagemMsg(Mensagem mensagem, int socketCliente)
{
    printf("Recebido MSG de %d\n", mensagem.idOrigem);
    if (estruturas.isEmissor(mensagem.idOrigem))
    {
        if (estruturas.isEmissor(mensagem.idDestino))
        {
            // Encontra exibidor do emissor
            if (estruturas.temExibidorAssociado(mensagem.idDestino))
            {
                int exibidor = estruturas.obterExibidorAssociado(mensagem.idDestino);
                enviarMensagem(estruturas.obterSocket(exibidor), mensagem);
                Mensagem confirmacao = receberMensagem(estruturas.obterSocket(exibidor));
                if (!confirmacao.valida || confirmacao.tipo == 2)
                {
                    printf("Erro ao receber a confirmação da MSG do cliente %d\n", exibidor);
                }
            }
            enviarOk(mensagem, mensagem.idOrigem, socketCliente);
        }
        else
        {
            if (estruturas.isExibidor(mensagem.idDestino))
            {
                // Envia para o exibidor
                enviarMensagem(estruturas.obterSocket(mensagem.idDestino), mensagem);
                Mensagem confirmacao = receberMensagem(estruturas.obterSocket(mensagem.idDestino));
                if (!confirmacao.valida || confirmacao.tipo == 2)
                {
                    printf("Erro ao receber a confirmação da MSG do cliente %d\n", mensagem.idDestino);
                }
                enviarOk(mensagem, mensagem.idOrigem, socketCliente);
            }
            else
            {
                if (mensagem.idDestino == 0)
                {
                    std::set<unsigned short> exibidores = estruturas.obterExibidores();
                    // BROADCAST
                    for (unsigned short exibidor : exibidores)
                    {
                        enviarMensagem(estruturas.obterSocket(exibidor), mensagem);
                        Mensagem confirmacao = receberMensagem(estruturas.obterSocket(exibidor));
                        if (!confirmacao.valida || confirmacao.tipo == 2)
                        {
                            printf("Erro ao receber a confirmação da MSG do cliente %d\n", exibidor);
                        }
                    }
                    enviarOk(mensagem, mensagem.idOrigem, socketCliente);
                }
                else
                {
                    enviarErro(mensagem, mensagem.idOrigem, socketCliente);
                }
            }
        }
    }
    else
    {
        enviarErro(mensagem, mensagem.idOrigem, socketCliente);
    }
}

void tratarMensagemCreq(Mensagem mensagem, int socketCliente)
{
    printf("Recebido CREQ de %d\n", mensagem.idOrigem);
    if (estruturas.isEmissor(mensagem.idOrigem))
    {
        Mensagem mensagemList;
        mensagemList.tipo = 7;
        mensagemList.idOrigem = idServidor;
        mensagemList.idDestino = mensagem.idDestino;
        mensagemList.numSeq = mensagemList.numSeq + 1;
        std::set<unsigned short> emissores = estruturas.obterEmissores();
        std::set<unsigned short> exibidores = estruturas.obterExibidores();
        for (int emissor : emissores)
        {
            mensagemList.texto += std::to_string(emissor);
            mensagemList.texto += " ";
        }

        for (int exibidor : exibidores)
        {
            mensagemList.texto += std::to_string(exibidor);
            mensagemList.texto += " ";
        }
        mensagemList.texto.pop_back(); // Tira espaço do ultimo item
        if (mensagem.idDestino == 0)
        {
            for (int exibidor : exibidores)
            {
                enviarMensagem(estruturas.obterSocket(exibidor), mensagemList);
                Mensagem confirmacao = receberMensagem(estruturas.obterSocket(exibidor));
                if (!confirmacao.valida || confirmacao.tipo == 2)
                {
                    printf("Erro ao receber a confirmação de CREQ do cliente %d\n", exibidor);
                }
            }
        }
        else
        {
            if (estruturas.isExibidor(mensagem.idDestino))
            {
                enviarMensagem(estruturas.obterSocket(mensagem.idDestino), mensagemList);
                Mensagem confirmacao = receberMensagem(estruturas.obterSocket(mensagem.idDestino));
                if (!confirmacao.valida || confirmacao.tipo == 2)
                {
                    printf("Erro ao receber a confirmação de CREQ do cliente %d\n", mensagem.idDestino);
                }
            }
            else if (estruturas.isEmissor(mensagem.idDestino))
            {
                // Encontra exibidor do emissor
                if (estruturas.temExibidorAssociado(mensagem.idDestino))
                {
                    int exibidor = estruturas.obterExibidorAssociado(mensagem.idDestino);
                    enviarMensagem(estruturas.obterSocket(exibidor), mensagemList);
                    Mensagem confirmacao = receberMensagem(estruturas.obterSocket(exibidor));
                    if (!confirmacao.valida || confirmacao.tipo == 2)
                    {
                        printf("Erro ao receber a confirmação da MSG do cliente %d\n", exibidor);
                    }
                }
            }
            else
            {
                enviarErro(mensagem, mensagem.idOrigem, socketCliente);
                return;
            }
        }
        enviarOk(mensagem, mensagem.idOrigem, socketCliente);
    }
    else
    {
        enviarErro(mensagem, mensagem.idOrigem, socketCliente);
    }
}

void tratarMensagemOrigin(Mensagem mensagem, int socketCliente)
{
    printf("Recebido ORIGIN de %d\n", mensagem.idOrigem);

    if (!estruturas.isEmissor(mensagem.idOrigem) && !estruturas.isExibidor(mensagem.idOrigem))
    {
        enviarErro(mensagem, mensagem.idOrigem, socketCliente);
    }
    else
    {
        std::string planeta = mensagem.texto;
        estruturas.inserePlaneta(mensagem.idOrigem, planeta);
        enviarOk(mensagem, mensagem.idOrigem, socketCliente);
    }
}

void tratarMensagemPlanet(Mensagem mensagem, int socketCliente)
{
    printf("Recebido PLANET de %d\n", mensagem.idOrigem);
    if (!estruturas.isEmissor(mensagem.idDestino) && !estruturas.isExibidor(mensagem.idDestino))
    {
        enviarErro(mensagem, mensagem.idOrigem, socketCliente);
    }
    else
    {
        if (estruturas.temExibidorAssociado(mensagem.idOrigem))
        {
            Mensagem planet;
            planet.tipo = mensagem.tipo;
            planet.idOrigem = mensagem.idOrigem;
            planet.idDestino = mensagem.idDestino;
            planet.numSeq = mensagem.numSeq;
            planet.texto = estruturas.obterPlaneta(mensagem.idDestino);
            int exibidor = estruturas.obterExibidorAssociado(mensagem.idOrigem);
            enviarMensagem(estruturas.obterSocket(exibidor), planet);
            Mensagem confirmacao = receberMensagem(estruturas.obterSocket(exibidor));
            if (!confirmacao.valida || confirmacao.tipo == 2)
            {
                printf("Erro ao receber a confirmação de PLANET do cliente %d\n", exibidor);
            }
        }
        enviarOk(mensagem, mensagem.idOrigem, socketCliente);
    }
}

void tratarMensagemPlanetList(Mensagem mensagem, int socketCliente)
{
    printf("Recebido PLANETLIST de %d\n", mensagem.idOrigem);

    if (estruturas.temExibidorAssociado(mensagem.idOrigem))
    {
        Mensagem planetlist;
        planetlist.tipo = 10;
        planetlist.idOrigem = mensagem.idOrigem;
        planetlist.idDestino = estruturas.obterExibidorAssociado(mensagem.idOrigem);
        planetlist.numSeq = mensagem.numSeq;
        std::set<std::string> planetas = estruturas.obterTodosPlanetas();
        for (std::string planeta : planetas)
        {
            planetlist.texto += planeta;
            planetlist.texto += " ";
        }
        planetlist.texto.pop_back();

        enviarMensagem(estruturas.obterSocket(planetlist.idDestino), planetlist);
        Mensagem confirmacao = receberMensagem(estruturas.obterSocket(planetlist.idDestino));
        if (!confirmacao.valida || confirmacao.tipo == 2)
        {
            printf("Erro ao receber a confirmação de PLANETLIST do cliente %d\n", planetlist.idDestino);
        }
    }

    enviarOk(mensagem, mensagem.idOrigem, socketCliente);
}

int tratarMensagemRecebida(Mensagem mensagem, int socketCliente)
{
    switch (mensagem.tipo)
    {
    case 1:
        printf("Recebido OK de %d\n", mensagem.idOrigem);
        break; // OK
    case 2:
        printf("Recebido ERROR de %d\n", mensagem.idOrigem);
        break; // ERROR
    case 3:
        tratarMensagemHi(mensagem, socketCliente);
        break; // HI
    case 4:
        tratarMensagemKill(mensagem, socketCliente);
        return ENCERRAR; // KILL
    case 5:
        tratarMensagemMsg(mensagem, socketCliente);
        break; // MSG
    case 6:
        tratarMensagemCreq(mensagem, socketCliente);
        break; // CREQ
    case 8:
        tratarMensagemOrigin(mensagem, socketCliente);
        if (estruturas.isExibidor(mensagem.idOrigem))
        {
            // Encerra a thread do exibidor depois de registrar seu planeta
            return ENCERRAR;
        }
        break; // ORIGIN
    case 9:
        tratarMensagemPlanet(mensagem, socketCliente);
        break; // PLANET
    case 10:
        tratarMensagemPlanetList(mensagem, socketCliente);
        break; // PLANETLIST
    default:
        Mensagem resposta;
        resposta.tipo = 2; // Mensagem de erro
        resposta.idOrigem = idServidor;
        resposta.idDestino = mensagem.idOrigem;
        resposta.numSeq = mensagem.numSeq;
        enviarMensagem(socketCliente, resposta);
        enviarErro(mensagem, mensagem.idOrigem, socketCliente);
    }
    return PROXIMA_MENSAGEM;
}

int receberETratarMensagemCliente(int socketCliente)
{
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
    return PROXIMA_MENSAGEM;
}

void tratarConexaoCliente(int socketCliente)
{
    // Enquanto a conexão com o cliente esteja ativa
    while (1)
    {
        int retorno = receberETratarMensagemCliente(socketCliente);
        if (retorno == ENCERRAR)
        { // Se o servidor deve encerrar ou se conectar com outro cliente
            return;
        }
    }
}

int aceitarSocketCliente(int socketServidor)
{
    // Inicializa as estruturas do socket do cliente
    struct sockaddr_storage dadosSocketCliente;
    struct sockaddr *enderecoSocketCliente = (struct sockaddr *)(&dadosSocketCliente);
    socklen_t tamanhoEnderecoSocketCliente = sizeof(enderecoSocketCliente);

    // Aceita uma conexão de um cliente
    int socketCliente = accept(socketServidor, enderecoSocketCliente, &tamanhoEnderecoSocketCliente);

    if (socketCliente == -1)
    {
        sairComMensagem("Erro ao aceitar a conexao de um cliente");
    }
    return socketCliente;
}

void esperarPorConexoesCliente(int socketServidor)
{
    // Enquanto o servidor estiver ativo ele deve receber conexões de clientes
    while (1)
    {
        int socketCliente = aceitarSocketCliente(socketServidor);
        std::thread cliente(tratarConexaoCliente, socketCliente);
        cliente.detach();
    }
}

int main(int argc, char **argv)
{

    verificarParametros(argc, argv);

    char protocolo[] = "v6";

    if (argc > 2 && strcmp(argv[2], "v4") == 0)
    {
        strcpy(protocolo, "v4");
    }

    struct sockaddr_storage dadosSocket;

    inicializarDadosSocket(protocolo, argv[1], &dadosSocket, argv[0]);

    int socketServidor = inicializarSocketServidor(&dadosSocket);

    ativarEscutaPorConexoes(socketServidor, &dadosSocket);

    esperarPorConexoesCliente(socketServidor);

    exit(EXIT_SUCCESS);
}