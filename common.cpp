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

void enviarMensagem(int socket, Mensagem aEnviar)
{
    // Converte os valores do cabecalho para network byte order
    unsigned short tipo = htons(aEnviar.tipo);
    unsigned short idOrigem = htons(aEnviar.idOrigem);
    unsigned short idDestino = htons(aEnviar.idDestino);
    unsigned short numSeq = htons(aEnviar.numSeq);

    size_t tamanhoTexto = aEnviar.texto.size();
    short tamanho = htons(tamanhoTexto);

    // Determina o numemro de bytes da mensagem que sera enviada
    size_t numeroBytes = 10 + tamanhoTexto;

    // Declara o vetor de caracteres sem sinal com o tamanho determinado
    unsigned char mensagem[numeroBytes + 1];

    // Converte os valores do cabecalho para dois bytes que representam seu valor como unsigned short
    mensagem[0] = tipo & 0xFF;
    mensagem[1] = (tipo >> 8) & 0xFF;

    mensagem[2] = idOrigem & 0xFF;
    mensagem[3] = (idOrigem >> 8) & 0xFF;

    mensagem[4] = idDestino & 0xFF;
    mensagem[5] = (idDestino >> 8) & 0xFF;

    mensagem[6] = numSeq & 0xFF;
    mensagem[7] = (numSeq >> 8) & 0xFF;

    mensagem[8] = tamanho & 0xFF;
    mensagem[9] = (tamanho >> 8) & 0xFF;

    // Adiciona o corpo da mensagem no vetor de caracteres sem sinal
    for (size_t i = 0; i < tamanhoTexto; i++)
    {
        mensagem[10 + i] = aEnviar.texto[i];
    }

    // Adiciona o caractere '\n' no fim da mensagem
    mensagem[10 + tamanhoTexto] = '\n';

    // Envia o conteudo de 'mensagem' para o cliente
    size_t tamanhoMensagemEnviada = send(socket, (char *)&mensagem[0], numeroBytes + 1, 0);

    // Se houve um erro no envio, encerra com mensagem
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
        // Recebe uma parte da mensagem
        size_t tamanhoLidoAgora = recv(socket, mensagem + tamanhoMensagem, BUFSZ - (int)tamanhoMensagem - 1, 0);
        // Se foi lido zero bytes, ocorreu um erro
        if (tamanhoLidoAgora == 0)
        {
            // Retorna uma mensagem invalida, indicando que houve um erro
            Mensagem mensagem;
            mensagem.valida = false;
            return mensagem;
        }
        // Adiciona no tamanho total da mensagem o tamanho da parte lida
        tamanhoMensagem += tamanhoLidoAgora;
    } while (mensagem[tamanhoMensagem - 1] != '\n');

    // Converte os 10 primeiros bytes recebidos para os valores do cabecalho como unsigned short
    unsigned short tipo = *((unsigned short *)mensagem);
    unsigned short idOrigem = *((unsigned short *)(mensagem + 2));
    unsigned short idDestino = *((unsigned short *)(mensagem + 4));
    unsigned short numSeq = *((unsigned short *)(mensagem + 6));
    unsigned short tamanho = *((unsigned short *)(mensagem + 8));

    // Salva os valores do cabecalho originais, sem network byte order, na mensagem que será retornada
    Mensagem retorno;
    retorno.tipo = ntohs(tipo);
    retorno.idOrigem = ntohs(idOrigem);
    retorno.idDestino = ntohs(idDestino);
    retorno.numSeq = ntohs(numSeq);

    // Salva o corpo da mensnagem na mensagem que será retornada
    tamanho = ntohs(tamanho);
    for (int i = 0; i < tamanho; i++)
    {
        retorno.texto += mensagem[10 + i];
    }

    // Retorna a mensagem construida
    return retorno;
}