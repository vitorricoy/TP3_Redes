#include "estruturas.h"

// Obtem o proximo id do emissor
unsigned short Estruturas::obterProximoIdEmissor()
{
    // Bloqueia o mutex
    acesso.lock();
    // Id inicial do emissor é 1
    unsigned short idInicial = 1;
    // Enquanto nao achar um id livre
    while (emissores.find(idInicial) != emissores.end())
    {
        // Incrementa o id
        idInicial++;
    }
    // Libera o mutex
    acesso.unlock();
    // Retorna o id encontrado
    return idInicial;
}

// Obtem o proximo id do exibidor
unsigned short Estruturas::obterProximoIdExibidor()
{
    // Bloqueia o mutex
    acesso.lock();
    // Id inicial do exibidor é 4096
    unsigned short idInicial = 4096;
    // Enquanto nao achar um id livre
    while (exibidores.find(idInicial) != exibidores.end())
    {
        // Incrementa o id
        idInicial++;
    }
    // Libera o mutex
    acesso.unlock();
    // Retorna o id encontrado
    return idInicial;
}

// Insere um novo exibidor
void Estruturas::insereNovoExibidor(unsigned short id, int socket)
{
    // Bloqueia o mutex
    acesso.lock();
    // Associa o socket do exibidor ao seu id
    mapaClienteSocket[id] = socket;
    // Insere o exibidor no conjunto de exibidores
    exibidores.insert(id);
    // Libera o mutex
    acesso.unlock();
}

// Insere um novo emissor
bool Estruturas::insereNovoEmissor(unsigned short id, int socket, unsigned short exibidor)
{
    // Bloqueia o mutex
    acesso.lock();
    // Associa o socket do emissor ao seu id
    mapaClienteSocket[id] = socket;
    // Insere o emissor no conjunto de emissores
    emissores.insert(id);
    // Se o id do exibidor associado pode ser valido
    if (exibidor >= 4096 && exibidor <= 8191)
    { // Valor entre 2^12 e 2^13-1
        // Se o id corresponde a um exibidor valido
        if (exibidores.find(exibidor) != exibidores.end())
        {
            // Indica que o exibidor é o exibidor associado ao emissor
            mapaEmissorExibidor[id] = exibidor;
            // Libera o mutex
            acesso.unlock();
            return true;
        }
        else
        { // Id invalido
            // Libera o mutex
            acesso.unlock();
            // Erro ao associar o emissor ao exibidor
            return false;
        }
    }
    // Id sabidamente invalido, logo emissor nao tera exibidor associado
    return true;
}

// Associa um planeta a um cliente
void Estruturas::inserePlaneta(unsigned short id, std::string planeta)
{
    // Bloqueia o mutex
    acesso.lock();
    mapaClientePlaneta[id] = planeta;
    // Libera o mutex
    acesso.unlock();
}

// Obtem o planeta de um cliente
std::string Estruturas::obterPlaneta(unsigned short id)
{
    std::string retorno;
    // Bloqueia o mutex
    acesso.lock();
    retorno = mapaClientePlaneta[id];
    // Libera o mutex
    acesso.unlock();
    return retorno;
}

// Obtem todos planetas registrados
std::set<std::string> Estruturas::obterTodosPlanetas()
{
    std::set<std::string> retorno;
    // Bloqueia o mutex
    acesso.lock();
    // Lista os planetas
    for (std::pair<unsigned short, std::string> planeta : mapaClientePlaneta)
    {
        retorno.insert(planeta.second);
    }
    // Libera o mutex
    acesso.unlock();
    return retorno;
}

// Obtem todos exibidores registrados
std::set<unsigned short> Estruturas::obterExibidores()
{
    std::set<unsigned short> retorno;
    // Bloqueia o mutex
    acesso.lock();
    retorno = exibidores;
    // Libera o mutex
    acesso.unlock();
    return retorno;
}

// Obtem todos emissores registrados
std::set<unsigned short> Estruturas::obterEmissores()
{
    std::set<unsigned short> retorno;
    // Bloqueia o mutex
    acesso.lock();
    retorno = emissores;
    // Libera o mutex
    acesso.unlock();
    return retorno;
}

// Remove um exibidor dos registros
void Estruturas::removeExibidor(unsigned short id)
{
    // Bloqueia o mutex
    acesso.lock();
    // Apaga o exibidor das estruturas
    mapaClienteSocket.erase(id);
    mapaClientePlaneta.erase(id);
    exibidores.erase(id);
    // Libera o mutex
    acesso.unlock();
}

// Remove um emissor dos registros
void Estruturas::removeEmissor(unsigned short id)
{
    // Bloqueia o mutex
    acesso.lock();
    // Apaga o emissor das estruturas
    mapaClienteSocket.erase(id);
    mapaClientePlaneta.erase(id);
    emissores.erase(id);
    mapaEmissorExibidor.erase(id);
    // Libera o mutex
    acesso.unlock();
}

// Verifica se o id dado é de um emissor
bool Estruturas::isEmissor(unsigned short id)
{
    bool retorno;
    // Bloqueia o mutex
    acesso.lock();
    retorno = emissores.find(id) != emissores.end();
    // Libera o mutex
    acesso.unlock();
    return retorno;
}

// Verifica se o id dado é de um exibidor
bool Estruturas::isExibidor(unsigned short id)
{
    bool retorno;
    // Bloqueia o mutex
    acesso.lock();
    retorno = exibidores.find(id) != exibidores.end();
    // Libera o mutex
    acesso.unlock();
    return retorno;
}

// Verifica se o emissor possui exibidor associado
bool Estruturas::temExibidorAssociado(unsigned short idExibidor)
{
    bool retorno;
    // Bloqueia o mutex
    acesso.lock();
    retorno = mapaEmissorExibidor.find(idExibidor) != mapaEmissorExibidor.end();
    // Libera o mutex
    acesso.unlock();
    return retorno;
}

// Obtem o exibidor associado ao emissor
unsigned short Estruturas::obterExibidorAssociado(unsigned short idExibidor)
{
    unsigned short retorno;
    // Bloqueia o mutex
    acesso.lock();
    retorno = mapaEmissorExibidor[idExibidor];
    // Libera o mutex
    acesso.unlock();
    return retorno;
}

// Obtem o socket do cliente com o id dado
int Estruturas::obterSocket(unsigned short id)
{
    int retorno;
    // Bloqueia o mutex
    acesso.lock();
    retorno = mapaClienteSocket[id];
    // Libera o mutex
    acesso.unlock();
    return retorno;
}

// Construtor
Estruturas::Estruturas()
{
}