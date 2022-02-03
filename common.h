#pragma once

#include <stdlib.h>
#include <string>
#include <arpa/inet.h>

// Encerra o programa com a mensagem 'msg'
void sairComMensagem(const char *msg);

// Converte um endereço de socket para string
void converterEnderecoParaString(struct sockaddr *endereco, char *string, size_t tamanhoString);

struct Mensagem;

void enviarMensagem(int socket, Mensagem mensagem);

Mensagem receberMensagem(int socket);

struct Mensagem
{
    unsigned short tipo;
    unsigned short idOrigem;
    unsigned short idDestino;
    unsigned short numSeq;
    std::string texto;
    bool valida;

    Mensagem() : texto(""), valida(true) {}
};