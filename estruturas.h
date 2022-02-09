
#include <mutex>
#include <map>
#include <set>

class Estruturas
{
public:
    unsigned short obterProximoIdEmissor();
    unsigned short obterProximoIdExibidor();
    void insereNovoExibidor(unsigned short id, int socket);
    bool insereNovoEmissor(unsigned short id, int socket, unsigned short exibidor);
    void inserePlaneta(unsigned short id, std::string planeta);
    std::string obterPlaneta(unsigned short id);
    std::set<std::string> obterTodosPlanetas();
    std::set<unsigned short> obterExibidores();
    std::set<unsigned short> obterEmissores();
    void removeExibidor(unsigned short id);
    void removeEmissor(unsigned short id);
    bool isEmissor(unsigned short id);
    bool isExibidor(unsigned short id);
    bool temExibidorAssociado(unsigned short idExibidor);
    unsigned short obterExibidorAssociado(unsigned short idExibidor);
    int obterSocket(unsigned short id);

    Estruturas();

private:
    std::set<unsigned short> idEmissorAssociado;
    std::set<unsigned short> idExibidorAssociado;
    std::map<unsigned short, unsigned short> mapaEmissorExibidor;
    std::map<unsigned short, int> mapaClienteSocket;
    std::map<unsigned short, std::string> mapaClientePlaneta;
    std::set<unsigned short> exibidores, emissores;
    std::mutex acesso;
};