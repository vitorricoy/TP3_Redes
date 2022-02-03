#include "common.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <arpa/inet.h>

#define BUFSZ 2000000

void sairComMensagem(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void converterEnderecoParaString(struct sockaddr *endereco, char *string, size_t tamanhoString)
{
    int versao;
    char stringEndereco[INET6_ADDRSTRLEN + 1] = "";
    unsigned short porta;

    if (endereco->sa_family == AF_INET)
    { // Endereço é IPv4
        versao = 4;
        struct sockaddr_in *enderecov4 = (struct sockaddr_in *)endereco;
        if (!inet_ntop(AF_INET, &(enderecov4->sin_addr), stringEndereco, INET_ADDRSTRLEN + 1))
        {
            sairComMensagem("Erro ao converter endereço para string com o comando ntop");
        }
        porta = ntohs(enderecov4->sin_port); // Converte a porta de network para host short
    }
    else
    {
        if (endereco->sa_family == AF_INET6)
        { // Endereço é IPv6
            versao = 6;
            struct sockaddr_in6 *enderecov6 = (struct sockaddr_in6 *)endereco;
            if (!inet_ntop(AF_INET6, &(enderecov6->sin6_addr), stringEndereco, INET6_ADDRSTRLEN + 1))
            {
                sairComMensagem("Erro ao converter endereço para string com o comando ntop");
            }
            porta = ntohs(enderecov6->sin6_port); // Converte a porta de network para host short
        }
        else
        {
            sairComMensagem("Familia de protocolo desconhecida");
        }
    }
    if (string)
    { // Formata o endereço em string para um formato legível
        snprintf(string, tamanhoString, "IPv%d %s %hu", versao, stringEndereco, porta);
    }
}

void enviarMensagem(int socket, Mensagem aEnviar)
{
    unsigned short tipo = htons(aEnviar.tipo);
    unsigned short idOrigem = htons(aEnviar.idOrigem);
    unsigned short idDestino = htons(aEnviar.idDestino);
    unsigned short numSeq = htons(aEnviar.numSeq);

    size_t numeroBytes = 10 + aEnviar.texto.size();

    unsigned char mensagem[numeroBytes + 1];

    mensagem[0] = tipo & 0xFF;
    mensagem[1] = (tipo >> 8) & 0xFF;

    mensagem[2] = idOrigem & 0xFF;
    mensagem[3] = (idOrigem >> 8) & 0xFF;

    mensagem[4] = idDestino & 0xFF;
    mensagem[5] = (idDestino >> 8) & 0xFF;

    mensagem[6] = numSeq & 0xFF;
    mensagem[7] = (numSeq >> 8) & 0xFF;

    size_t tamanhoTexto = aEnviar.texto.size();
    short tamanho = htons(tamanhoTexto);

    mensagem[8] = tamanho & 0xFF;
    mensagem[9] = (tamanho >> 8) & 0xFF;

    for (size_t i = 0; i < tamanhoTexto; i++)
    {
        mensagem[10 + i] = aEnviar.texto[i];
    }

    mensagem[10 + tamanhoTexto] = '\n';

    // Envia o conteudo de 'mensagem' para o cliente
    size_t tamanhoMensagemEnviada = send(socket, (char *)&mensagem[0], numeroBytes + 1, 0);
    if (numeroBytes + 1 != tamanhoMensagemEnviada)
    {
        sairComMensagem("Erro ao enviar mensagem ao cliente");
    }
}

Mensagem receberMensagem(int socket)
{
    char mensagem[BUFSZ];
    memset(mensagem, 0, BUFSZ);
    size_t tamanhoMensagem = 0;
    // Recebe mensagens do servidor enquanto elas não terminarem com \n
    do
    {
        size_t tamanhoLidoAgora = recv(socket, mensagem + tamanhoMensagem, BUFSZ - (int)tamanhoMensagem - 1, 0);
        if (tamanhoLidoAgora == 0)
        {
            Mensagem mensagem;
            mensagem.valida = false;
            return mensagem;
        }
        tamanhoMensagem += tamanhoLidoAgora;
    } while (mensagem[tamanhoMensagem - 1] != '\n');

    unsigned short tipo = *((unsigned short *)mensagem);
    unsigned short idOrigem = *((unsigned short *)(mensagem + 2));
    unsigned short idDestino = *((unsigned short *)(mensagem + 4));
    unsigned short numSeq = *((unsigned short *)(mensagem + 6));
    unsigned short tamanho = *((unsigned short *)(mensagem + 8));
    Mensagem retorno;
    retorno.tipo = ntohs(tipo);
    retorno.idOrigem = ntohs(idOrigem);
    retorno.idDestino = ntohs(idDestino);
    retorno.numSeq = ntohs(numSeq);
    tamanho = ntohs(tamanho);
    for (int i = 0; i < tamanho; i++)
    {
        retorno.texto += mensagem[10 + i];
    }
    return retorno;
}