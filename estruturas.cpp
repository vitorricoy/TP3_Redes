#include "estruturas.h"

unsigned short Estruturas::obterProximoIdEmissor()
{
    acesso.lock();
    unsigned short idInicial = 1;
    while (idEmissorAssociado.find(idInicial) != idEmissorAssociado.end())
    {
        idInicial++;
    }
    acesso.unlock();
    return idInicial;
}

unsigned short Estruturas::obterProximoIdExibidor()
{
    acesso.lock();
    unsigned short idInicial = 4096;
    while (idExibidorAssociado.find(idInicial) != idExibidorAssociado.end())
    {
        idInicial++;
    }
    acesso.unlock();
    return idInicial;
}

void Estruturas::insereNovoExibidor(unsigned short id, int socket)
{
    acesso.lock();
    mapaClienteSocket[id] = socket;
    exibidores.insert(id);
    acesso.unlock();
}

bool Estruturas::insereNovoEmissor(unsigned short id, int socket, unsigned short exibidor)
{
    acesso.lock();
    mapaClienteSocket[id] = socket;
    emissores.insert(id);
    if (exibidor >= 4096 && exibidor <= 8191)
    { // Valor entre 2^12 e 2^13-1
        if (exibidores.find(exibidor) != exibidores.end())
        { // Exibidor existe
            mapaEmissorExibidor[id] = exibidor;
            acesso.unlock();
            return true;
        }
        else
        {
            acesso.unlock();
            return false;
        }
    }
    return false;
}

void Estruturas::inserePlaneta(unsigned short id, std::string planeta)
{
    acesso.lock();
    mapaClientePlaneta[id] = planeta;
    acesso.unlock();
}

std::string Estruturas::obterPlaneta(unsigned short id)
{
    std::string retorno;
    acesso.lock();
    retorno = mapaClientePlaneta[id];
    acesso.unlock();
    return retorno;
}
std::set<std::string> Estruturas::obterTodosPlanetas()
{
    std::set<std::string> retorno;
    acesso.lock();
    for (std::pair<unsigned short, std::string> planeta : mapaClientePlaneta)
    {
        retorno.insert(planeta.second);
    }
    acesso.unlock();
    return retorno;
}

std::set<unsigned short> Estruturas::obterExibidores()
{
    std::set<unsigned short> retorno;
    acesso.lock();
    retorno = exibidores;
    acesso.unlock();
    return retorno;
}

std::set<unsigned short> Estruturas::obterEmissores()
{
    std::set<unsigned short> retorno;
    acesso.lock();
    retorno = emissores;
    acesso.unlock();
    return retorno;
}

void Estruturas::removeExibidor(unsigned short id)
{
    acesso.lock();
    mapaClienteSocket.erase(id);
    mapaClientePlaneta.erase(id);
    exibidores.erase(id);
    acesso.unlock();
}
void Estruturas::removeEmissor(unsigned short id)
{
    acesso.lock();
    mapaClienteSocket.erase(id);
    mapaClientePlaneta.erase(id);
    emissores.erase(id);
    mapaEmissorExibidor.erase(id);
    acesso.unlock();
}

bool Estruturas::isEmissor(unsigned short id)
{
    bool retorno;
    acesso.lock();
    retorno = emissores.find(id) != emissores.end();
    acesso.unlock();
    return retorno;
}

bool Estruturas::isExibidor(unsigned short id)
{
    bool retorno;
    acesso.lock();
    retorno = exibidores.find(id) != exibidores.end();
    acesso.unlock();
    return retorno;
}

bool Estruturas::temExibidorAssociado(unsigned short idExibidor)
{
    bool retorno;
    acesso.lock();
    retorno = mapaEmissorExibidor.find(idExibidor) != mapaEmissorExibidor.end();
    acesso.unlock();
    return retorno;
}

unsigned short Estruturas::obterExibidorAssociado(unsigned short idExibidor)
{
    unsigned short retorno;
    acesso.lock();
    retorno = mapaEmissorExibidor[idExibidor];
    acesso.unlock();
    return retorno;
}

int Estruturas::obterSocket(unsigned short id)
{
    int retorno;
    acesso.lock();
    retorno = mapaClienteSocket[id];
    acesso.unlock();
    return retorno;
}

Estruturas::Estruturas()
{
}