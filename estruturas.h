
#include <mutex>
#include <map>
#include <set>

// Classe das estruturas
// Todo metodo garante a exclusao mutua do acesso à classe
class Estruturas
{
public:
    // Obtem o proximo id do emissor
    unsigned short obterProximoIdEmissor();
    // Obtem o proximo id do exibidor
    unsigned short obterProximoIdExibidor();
    // Insere um novo exibidor
    void insereNovoExibidor(unsigned short id, int socket);
    // Insere um novo emissor
    bool insereNovoEmissor(unsigned short id, int socket, unsigned short exibidor);
    // Associa um planeta a um cliente
    void inserePlaneta(unsigned short id, std::string planeta);
    // Obtem o planeta de um cliente
    std::string obterPlaneta(unsigned short id);
    // Obtem todos planetas registrados
    std::set<std::string> obterTodosPlanetas();
    // Obtem todos exibidores registrados
    std::set<unsigned short> obterExibidores();
    // Obtem todos emissores registrados
    std::set<unsigned short> obterEmissores();
    // Remove um exibidor dos registros
    void removeExibidor(unsigned short id);
    // Remove um emissor dos registros
    void removeEmissor(unsigned short id);
    // Verifica se o id dado é de um emissor
    bool isEmissor(unsigned short id);
    // Verifica se o id dado é de um exibidor
    bool isExibidor(unsigned short id);
    // Verifica se o emissor possui exibidor associado
    bool temExibidorAssociado(unsigned short idExibidor);
    // Obtem o exibidor associado ao emissor
    unsigned short obterExibidorAssociado(unsigned short idExibidor);
    // Obtem o socket do cliente com o id dado
    int obterSocket(unsigned short id);

    // Construtor
    Estruturas();

private:
    // Mapeia um exibidor a um emissor
    std::map<unsigned short, unsigned short> mapaEmissorExibidor;
    // Mapeia um socket a um cliente
    std::map<unsigned short, int> mapaClienteSocket;
    // Mapeia um planeta a um cliente
    std::map<unsigned short, std::string> mapaClientePlaneta;
    // Conjunto de emissores e de exibidores
    std::set<unsigned short> exibidores, emissores;
    // Mutex de acesso aos metodos da classe
    std::mutex acesso;
};