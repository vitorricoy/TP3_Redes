#pragma once

#include <stdlib.h>

#include <arpa/inet.h>

// Encerra o programa com a mensagem 'msg'
void sairComMensagem(char *msg);

// Converte um endereço de socket para string
void converterEnderecoParaString(struct sockaddr *endereco, char *string, size_t tamanhoString);

struct Mensagem;

void enviarMensagem(int socket, Mensagem mensagem);

Mensagem receberMensagem(int socket);
