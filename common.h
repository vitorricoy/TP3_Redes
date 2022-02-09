#pragma once

#include <stdlib.h>
#include <string>
#include <arpa/inet.h>

// Encerra o programa com a mensagem 'msg'
void sairComMensagem(const char *msg);

// Define a estrutura da mensagem que trafega pelo sistema
struct Mensagem;

// Envia uma mensagem na conexão determinada pelo socket
void enviarMensagem(int socket, Mensagem mensagem);

// Recebe uma mensagem da conexão determinada pelo socket
Mensagem receberMensagem(int socket);

// Define a estrutura da mensagem que trafega pelo sistema
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