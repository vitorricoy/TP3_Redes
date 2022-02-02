#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

struct Mensagem
{
    unsigned short tipo;
    unsigned short idOrigem;
    unsigned short idDestino;
    unsigned short numSeq;
    char texto[BUFSZ];
    bool valida;

    Mensagem() : valida(true);
};

void sairComMensagem(char *msg)
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

    int numeroBytes = 10 + strlen(aEnviar.texto);

    char mensagem[numeroBytes + 1];

    mensagem[0] = ((char *)(&tipo))[0];
    mensagem[1] = ((char *)(&tipo))[1];

    mensagem[2] = ((char *)(&idOrigem))[0];
    mensagem[3] = ((char *)(&idOrigem))[1];

    mensagem[4] = ((char *)(&idDestino))[0];
    mensagem[5] = ((char *)(&idDestino))[1];

    mensagem[6] = ((char *)(&numSeq))[0];
    mensagem[7] = ((char *)(&numSeq))[1];

    size_t tamanhoTexto = strlen(aEnviar.texto);
    short tamanho = htons(tamanhoTexto);

    mensagem[8] = ((char *)(&tamanho))[0];
    mensagem[9] = ((char *)(&tamanho))[1];

    for (int i = 0; i < tamanhoTexto; i++)
    {
        mensagem[10 + i] = aEnviar.texto[i];
    }

    mensagem[10 + tamanhoTexto] = '\n';

    // Envia o conteudo de 'mensagem' para o cliente
    size_t tamanhoMensagemEnviada = send(socket, mensagem, strlen(mensagem), 0);
    if (strlen(mensagem) != tamanhoMensagemEnviada)
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
    } while (mensagem[strlen(mensagem) - 1] != '\n');

    unsigned short tipo;
    unsigned short idOrigem;
    unsigned short idDestino;
    unsigned short numSeq;
    char *texto;

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
    retorno.tamanho = ntohs(tamanho);
    for (int i = 0; i < retorno.tamanho; i++)
    {
        retorno.texto[i] = mensagem[10 + i];
    }
    retorno.texto[retorno.tamanho] = '\0';
    return retorno;
}