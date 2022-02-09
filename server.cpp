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

// Envia uma mensagem do tipo erro
void enviarErro(Mensagem mensagem, int destId, int socketCliente)
{
    Mensagem resposta;
    resposta.tipo = 2; // Mensagem de erro
    resposta.idOrigem = idServidor;
    resposta.idDestino = destId;
    resposta.numSeq = mensagem.numSeq;
    enviarMensagem(socketCliente, resposta);
}

// Envia uma mensagem do tipo ok
void enviarOk(Mensagem mensagem, int destId, int socketCliente)
{
    Mensagem resposta;
    resposta.tipo = 1; // Mensagem ok
    resposta.idOrigem = idServidor;
    resposta.idDestino = destId;
    resposta.numSeq = mensagem.numSeq;
    enviarMensagem(socketCliente, resposta);
}

// Trata o recebimento de uma mensagem do tipo hi
void tratarMensagemHi(Mensagem mensagem, int socketCliente)
{
    // Se o cliente sera um exibidor
    if (mensagem.idOrigem == 0)
    {
        // Obtem o seu id
        int idExibidor = estruturas.obterProximoIdExibidor();
        // Imprime que a mensagem foi recebida
        printf("Recebido hi de %d com origem %d\n", idExibidor, mensagem.idOrigem);
        // Insere o exibidor nas estruturas de dados
        estruturas.insereNovoExibidor(idExibidor, socketCliente);
        // Envia uma resposta do tipo ok
        enviarOk(mensagem, idExibidor, socketCliente);
    }
    else
    { // Se o cliente é emissor
        // Obtem o seu id
        int idEmissor = estruturas.obterProximoIdEmissor();
        // Imprime que a mensagem foi recebida
        printf("Recebido hi de %d com origem %d\n", idEmissor, mensagem.idOrigem);
        // Insere o emissor nas estruturas de dados
        bool sucesso = estruturas.insereNovoEmissor(idEmissor, socketCliente, mensagem.idOrigem);
        // Se o emissor foi inserido com sucesso
        if (sucesso)
        {
            // Envia a resposta de ok
            enviarOk(mensagem, idEmissor, socketCliente);
        }
        else
        {
            // Senao, envia erro
            enviarErro(mensagem, idEmissor, socketCliente);
        }
    }
}

// Trata o recebimento de uma mensagem do tipo kill
void tratarMensagemKill(Mensagem mensagem, int socketCliente)
{
    // Imprime que a mensagem foi recebida
    printf("Recebido kill de %d\n", mensagem.idOrigem);
    // Se o remetente é um emissor
    if (estruturas.isEmissor(mensagem.idOrigem))
    {
        // Se o remetente tem um exibidor associado
        if (estruturas.temExibidorAssociado(mensagem.idOrigem))
        {
            // Envia uma mensagem de kill para o exibidor associado
            Mensagem msgKill;
            msgKill.tipo = mensagem.tipo;
            msgKill.idOrigem = mensagem.idOrigem;
            msgKill.idDestino = estruturas.obterExibidorAssociado(mensagem.idOrigem);
            msgKill.numSeq = mensagem.numSeq;
            enviarMensagem(estruturas.obterSocket(msgKill.idDestino), msgKill);
            Mensagem confirmacao = receberMensagem(estruturas.obterSocket(msgKill.idDestino));
            // Se o exibidor nao confirmou a mesnagem
            if (!confirmacao.valida || confirmacao.tipo == 2)
            {
                printf("Erro ao receber a confirmação do KILL do cliente %d\n", msgKill.idDestino);
            }
            // Fecha o socket do exibidor
            close(estruturas.obterSocket(msgKill.idDestino));
            // Remove o exibidor das estruturas de dados
            estruturas.removeExibidor(msgKill.idDestino);
        }
        // Envia um ok para o remetente
        enviarOk(mensagem, mensagem.idOrigem, socketCliente);
        // Fecha o socket do remetente
        close(estruturas.obterSocket(mensagem.idOrigem));
        // Remove o remetente das estruturas de dados
        estruturas.removeEmissor(mensagem.idOrigem);
    }
    else
    {
        // Envia um erro pois o remetente do kill deve ser emissor
        enviarErro(mensagem, mensagem.idOrigem, socketCliente);
    }
}

// Trata o recebimento de uma mensagem do tipo mensagem
void tratarMensagemMsg(Mensagem mensagem, int socketCliente)
{
    // Imprime que a mensagem foi recebida
    printf("Recebido MSG de %d\n", mensagem.idOrigem);
    // Se a mensagem veio de um emissor
    if (estruturas.isEmissor(mensagem.idOrigem))
    {
        // Se a mensagem deve ser enviada a um emissor
        if (estruturas.isEmissor(mensagem.idDestino))
        {
            // Se o emissor tem um exibidor associado
            if (estruturas.temExibidorAssociado(mensagem.idDestino))
            {
                // Encontra o exibidor associado
                int exibidor = estruturas.obterExibidorAssociado(mensagem.idDestino);
                // Envia a mensagem ao exibidor associado
                enviarMensagem(estruturas.obterSocket(exibidor), mensagem);
                // Recebe a confirmacao da mensagem
                Mensagem confirmacao = receberMensagem(estruturas.obterSocket(exibidor));
                // Se a confirmacao for invalida ou de erro
                if (!confirmacao.valida || confirmacao.tipo == 2)
                {
                    printf("Erro ao receber a confirmação da MSG do cliente %d\n", exibidor);
                }
            }
            // Envia resposta do tipo ok
            enviarOk(mensagem, mensagem.idOrigem, socketCliente);
        }
        else
        {
            // Se o destino é um exibidor
            if (estruturas.isExibidor(mensagem.idDestino))
            {
                // Envia a mensagem para o exibidor
                enviarMensagem(estruturas.obterSocket(mensagem.idDestino), mensagem);
                // Espera a confirmacao do exibidor
                Mensagem confirmacao = receberMensagem(estruturas.obterSocket(mensagem.idDestino));
                // Se a confirmacao for invalida ou do tipo erro
                if (!confirmacao.valida || confirmacao.tipo == 2)
                {
                    printf("Erro ao receber a confirmação da MSG do cliente %d\n", mensagem.idDestino);
                }
                // Envia resposta do tipo ok
                enviarOk(mensagem, mensagem.idOrigem, socketCliente);
            }
            else
            {
                // Se a mensagem for de broadcast
                if (mensagem.idDestino == 0)
                {
                    // Obtem o conjunto de exibidores
                    std::set<unsigned short> exibidores = estruturas.obterExibidores();
                    // Para cada exibidor registrado no sistema
                    for (unsigned short exibidor : exibidores)
                    {
                        // Envia a mensagem para o exibidor
                        enviarMensagem(estruturas.obterSocket(exibidor), mensagem);
                        // Espera a confirmacao
                        Mensagem confirmacao = receberMensagem(estruturas.obterSocket(exibidor));
                        // Se a confirmaca for invalida ou de erro
                        if (!confirmacao.valida || confirmacao.tipo == 2)
                        {
                            printf("Erro ao receber a confirmação da MSG do cliente %d\n", exibidor);
                        }
                    }
                    // Envia resposta do tipo ok
                    enviarOk(mensagem, mensagem.idOrigem, socketCliente);
                }
                else
                { // Se o destino nao é emissor, exibidor nem broadcast
                    // Envia resposta do tipo erro
                    enviarErro(mensagem, mensagem.idOrigem, socketCliente);
                }
            }
        }
    }
    else
    { // Mensagem nao veio de um emissor
        // Envia resposta do tipo erro
        enviarErro(mensagem, mensagem.idOrigem, socketCliente);
    }
}

// Trata o recebimento de uma mensagem do tipo creq
void tratarMensagemCreq(Mensagem mensagem, int socketCliente)
{
    // Imprime que a mensagem foi recebida
    printf("Recebido CREQ de %d\n", mensagem.idOrigem);
    // Se a mensagem veio de um emissor
    if (estruturas.isEmissor(mensagem.idOrigem))
    {
        // Constroi a mensagem do tipo clist
        Mensagem mensagemList;
        mensagemList.tipo = 7;
        mensagemList.idOrigem = idServidor;
        mensagemList.idDestino = mensagem.idDestino;
        mensagemList.numSeq = mensagemList.numSeq + 1;
        // Lista os emissores e exibidores
        std::set<unsigned short> emissores = estruturas.obterEmissores();
        std::set<unsigned short> exibidores = estruturas.obterExibidores();
        // Adiciona no corpo da mensagem a lista de emissores separados por espaco
        for (int emissor : emissores)
        {
            mensagemList.texto += std::to_string(emissor);
            mensagemList.texto += " ";
        }
        // Adiciona no corpo da mensagem a lista de exibidores separados por espaco
        for (int exibidor : exibidores)
        {
            mensagemList.texto += std::to_string(exibidor);
            mensagemList.texto += " ";
        }
        // Tira espaço do ultimo item da lista
        mensagemList.texto.pop_back();
        // Se o destino é de broadcast
        if (mensagem.idDestino == 0)
        {
            // Envia o clist para todos os exibidores registrados no sistema
            for (int exibidor : exibidores)
            {
                // Envia a mensagem para o exibidor
                enviarMensagem(estruturas.obterSocket(exibidor), mensagemList);
                // Espera a confirmacao
                Mensagem confirmacao = receberMensagem(estruturas.obterSocket(exibidor));
                // Se a confirmacao for invalida ou do tipo erro
                if (!confirmacao.valida || confirmacao.tipo == 2)
                {
                    printf("Erro ao receber a confirmação de CREQ do cliente %d\n", exibidor);
                }
            }
        }
        else
        {
            // Se o destino é um exibidor
            if (estruturas.isExibidor(mensagem.idDestino))
            {
                // Envia a mensagem ao exibidor
                enviarMensagem(estruturas.obterSocket(mensagem.idDestino), mensagemList);
                // Espera a confirmacao
                Mensagem confirmacao = receberMensagem(estruturas.obterSocket(mensagem.idDestino));
                // Se a confirmacao for invalida ou do tipo erro
                if (!confirmacao.valida || confirmacao.tipo == 2)
                {
                    printf("Erro ao receber a confirmação de CREQ do cliente %d\n", mensagem.idDestino);
                }
            }
            else if (estruturas.isEmissor(mensagem.idDestino))
            { // Se o destino é um emissor
                // Se o emissor tem um exibidor associado
                if (estruturas.temExibidorAssociado(mensagem.idDestino))
                {
                    int exibidor = estruturas.obterExibidorAssociado(mensagem.idDestino);
                    // Envia a mensagem ao exibidor
                    enviarMensagem(estruturas.obterSocket(exibidor), mensagemList);
                    // Espera a confirmacao
                    Mensagem confirmacao = receberMensagem(estruturas.obterSocket(exibidor));
                    // Se a confirmacao for invalida ou do tipo erro
                    if (!confirmacao.valida || confirmacao.tipo == 2)
                    {
                        printf("Erro ao receber a confirmação da MSG do cliente %d\n", exibidor);
                    }
                }
            }
            else
            { // Destino nao é emissor, exibidor nem broadcast
                // Responde mensagem do tipo erro
                enviarErro(mensagem, mensagem.idOrigem, socketCliente);
                return;
            }
        }
        // Envia mensagem do tipo ok
        enviarOk(mensagem, mensagem.idOrigem, socketCliente);
    }
    else
    { // A mensagem nao veio de um emissor
        // Envia resposta do tipo erro
        enviarErro(mensagem, mensagem.idOrigem, socketCliente);
    }
}

// Trata o recebimento de uma mensagem do tipo origin
void tratarMensagemOrigin(Mensagem mensagem, int socketCliente)
{
    // Imprime que a mensagem foi recebida
    printf("Recebido ORIGIN de %d\n", mensagem.idOrigem);

    // Se a mensagem nao veio de um emissor ou exibidor registrado
    if (!estruturas.isEmissor(mensagem.idOrigem) && !estruturas.isExibidor(mensagem.idOrigem))
    {
        // Retorna resposta do tipo erro
        enviarErro(mensagem, mensagem.idOrigem, socketCliente);
    }
    else
    {
        // Senao, registra o planeta associado ao cliente nas estruturas de dados
        std::string planeta = mensagem.texto;
        estruturas.inserePlaneta(mensagem.idOrigem, planeta);
        // Envia resposta do tipo ok
        enviarOk(mensagem, mensagem.idOrigem, socketCliente);
    }
}

// Trata o recebimento de uma mensagem do tipo planet
void tratarMensagemPlanet(Mensagem mensagem, int socketCliente)
{
    // Imprime que a mensagem foi recebida
    printf("Recebido PLANET de %d\n", mensagem.idOrigem);
    // Se o destino nao é um emissor ou exibidor válido
    if (!estruturas.isEmissor(mensagem.idDestino) && !estruturas.isExibidor(mensagem.idDestino))
    {
        // Envia resposta do tipo erro
        enviarErro(mensagem, mensagem.idOrigem, socketCliente);
    }
    else
    {
        // Se o destino é um emissor
        if (estruturas.isEmissor(mensagem.idDestino))
        {
            // Verifica se ele tem um exibidor associado
            if (estruturas.temExibidorAssociado(mensagem.idDestino))
            {
                // Constroi a mensagem planet
                Mensagem planet;
                planet.tipo = mensagem.tipo;
                planet.idOrigem = mensagem.idOrigem;
                planet.idDestino = mensagem.idDestino;
                planet.numSeq = mensagem.numSeq;
                planet.texto = estruturas.obterPlaneta(mensagem.idDestino);
                // Obtem o exibidor associado
                int exibidor = estruturas.obterExibidorAssociado(mensagem.idDestino);
                // Envia a mensagem ao exibidor
                enviarMensagem(estruturas.obterSocket(exibidor), planet);
                // Espera confirmacao
                Mensagem confirmacao = receberMensagem(estruturas.obterSocket(exibidor));
                // Confirmacao foi invalida ou do tipo erro
                if (!confirmacao.valida || confirmacao.tipo == 2)
                {
                    printf("Erro ao receber a confirmação de PLANET do cliente %d\n", exibidor);
                }
            }
        }
        else
        { // Se o destino é um exibidor
            // Constroi a mensagem planet
            Mensagem planet;
            planet.tipo = mensagem.tipo;
            planet.idOrigem = mensagem.idOrigem;
            planet.idDestino = mensagem.idDestino;
            planet.numSeq = mensagem.numSeq;
            planet.texto = estruturas.obterPlaneta(mensagem.idDestino);
            // Envia a mensagem ao exibidor
            enviarMensagem(estruturas.obterSocket(mensagem.idDestino), planet);
            // Espera confirmacao
            Mensagem confirmacao = receberMensagem(estruturas.obterSocket(mensagem.idDestino));
            // Confirmacao foi invalida ou do tipo erro
            if (!confirmacao.valida || confirmacao.tipo == 2)
            {
                printf("Erro ao receber a confirmação de PLANET do cliente %d\n", mensagem.idDestino);
            }
        }
        // Envia resposta do tipo ok
        enviarOk(mensagem, mensagem.idOrigem, socketCliente);
    }
}

// Trata o recebimento de uma mensagem do tipo planetlist
void tratarMensagemPlanetList(Mensagem mensagem, int socketCliente)
{
    // Imprime que a mensagem foi recebida
    printf("Recebido PLANETLIST de %d\n", mensagem.idOrigem);

    // Se o remetente nao é emissor
    if (!estruturas.isEmissor(mensagem.idOrigem))
    {
        // Envia resposta do tipo erro
        enviarErro(mensagem, mensagem.idOrigem, socketCliente);
        return;
    }

    // Se o remetente tem exibidor associado
    if (estruturas.temExibidorAssociado(mensagem.idOrigem))
    {
        // Constroi a mensagem de planetlist
        Mensagem planetlist;
        planetlist.tipo = 10;
        planetlist.idOrigem = mensagem.idOrigem;
        planetlist.idDestino = estruturas.obterExibidorAssociado(mensagem.idOrigem);
        planetlist.numSeq = mensagem.numSeq;
        // Obtem todos os planetas registrados
        std::set<std::string> planetas = estruturas.obterTodosPlanetas();
        // Adiciona no corpo da mensagem a lista de planetas separados por espaco
        for (std::string planeta : planetas)
        {
            planetlist.texto += planeta;
            planetlist.texto += " ";
        }
        // Remove o espaco depois do ultimo item
        planetlist.texto.pop_back();

        // Envia mensagem ao exibidor
        enviarMensagem(estruturas.obterSocket(planetlist.idDestino), planetlist);
        // Espera confirmacao
        Mensagem confirmacao = receberMensagem(estruturas.obterSocket(planetlist.idDestino));
        // Confirmacao foi invalida ou do tipo erro
        if (!confirmacao.valida || confirmacao.tipo == 2)
        {
            printf("Erro ao receber a confirmação de PLANETLIST do cliente %d\n", planetlist.idDestino);
        }
    }
    // Envia resposta do tipo ok
    enviarOk(mensagem, mensagem.idOrigem, socketCliente);
}

// Trata o recebimento de uma mensagem pelo serivodr
int tratarMensagemRecebida(Mensagem mensagem, int socketCliente)
{
    // Se o socket do id de origem da mensagem for diferente do socket em que ela foi recebida
    if (mensagem.tipo != 3 && estruturas.obterSocket(mensagem.idOrigem) != socketCliente)
    {
        // Cliente se passando por outro, envia erro
        enviarErro(mensagem, mensagem.idOrigem, socketCliente);
    }

    // Verifica o tipo da mensagem
    switch (mensagem.tipo)
    {
    case 1:
        // Se for mensagem de ok imprime que foi recebido um ok do cliente de origem
        printf("Recebido ok de %d\n", mensagem.idOrigem);
        break; // OK
    case 2:
        // Se for mensagem de erro imprime que foi recebido um erro do cliente de origem
        printf("Recebido error de %d\n", mensagem.idOrigem);
        break; // ERROR
    case 3:
        // Se for mensagem de hi, chama o tratamento adequado
        tratarMensagemHi(mensagem, socketCliente);
        break; // HI
    case 4:
        // Se for mensagem de kill, chama o tratamento adequado
        tratarMensagemKill(mensagem, socketCliente);
        return ENCERRAR; // KILL
    case 5:
        // Se for mensagem do tipo mensagem, chama o tratamento adequado
        tratarMensagemMsg(mensagem, socketCliente);
        break; // MSG
    case 6:
        // Se for mensagem de creq, chama o tratamento adequado
        tratarMensagemCreq(mensagem, socketCliente);
        break; // CREQ
    case 8:
        // Se for mensagem de origin, chama o tratamento adequado
        tratarMensagemOrigin(mensagem, socketCliente);
        // Se o cliente que enviou o origin é um exibidor
        if (estruturas.isExibidor(mensagem.idOrigem))
        {
            // Encerra a thread do exibidor depois de registrar seu planeta
            return ENCERRAR;
        }
        break; // ORIGIN
    case 9:
        // Se for mensagem de planet, chama o tratamento adequado
        tratarMensagemPlanet(mensagem, socketCliente);
        break; // PLANET
    case 10:
        // Se for mensagem de planetlist, chama o tratamento adequado
        tratarMensagemPlanetList(mensagem, socketCliente);
        break; // PLANETLIST
    default:
        // Envia mensagem de erro
        enviarErro(mensagem, mensagem.idOrigem, socketCliente);
    }
    // Espera a proxima mensagem do cliente
    return PROXIMA_MENSAGEM;
}

int receberETratarMensagemCliente(int socketCliente)
{
    // Recebe uma mensagem do cliente
    Mensagem mensagem = receberMensagem(socketCliente);

    // Se a mensagem for invalida, encerra a thread
    if (!mensagem.valida)
    {
        return ENCERRAR;
    }

    // Trata a mensagem recebida
    int retorno = tratarMensagemRecebida(mensagem, socketCliente);

    // Se a thread deve ser encerrada
    if (retorno == ENCERRAR)
    {
        return retorno;
    }
    // Espera a proxima mensagem do cliente
    return PROXIMA_MENSAGEM;
}

void tratarConexaoCliente(int socketCliente)
{
    // Enquanto a conexão com o cliente estiver ativa
    while (1)
    {
        // Recebe e trata mensagens
        int retorno = receberETratarMensagemCliente(socketCliente);
        // Se o servidor deve encerrar
        if (retorno == ENCERRAR)
        {
            // Encerra a thread
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
        // Aceita a conexao de um cliente
        int socketCliente = aceitarSocketCliente(socketServidor);
        // Cria uma thread para tratar as mensagens enviadas por esse cliente
        std::thread cliente(tratarConexaoCliente, socketCliente);
        // Indica que a thread deve executar mesmo após sua variável nesse escopo ser destruída
        cliente.detach();
    }
}

int main(int argc, char **argv)
{
    // Verifica os parâmetros
    verificarParametros(argc, argv);

    // Por padrao o protocolo é IPv6
    char protocolo[] = "v6";

    // Se foi recebido um parâmetro indicando que o protocolo é IPv4
    if (argc > 2 && strcmp(argv[2], "v4") == 0)
    {
        // Define o protocolo como IPv4
        strcpy(protocolo, "v4");
    }

    // Inicializa os dados do socket
    struct sockaddr_storage dadosSocket;
    inicializarDadosSocket(protocolo, argv[1], &dadosSocket, argv[0]);

    // Inicializa o socket do servidor
    int socketServidor = inicializarSocketServidor(&dadosSocket);
    ativarEscutaPorConexoes(socketServidor, &dadosSocket);

    // Espera por conexoes de clientes
    esperarPorConexoesCliente(socketServidor);
}