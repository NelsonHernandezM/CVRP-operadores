#include "HeuristicCrossover.h"

#include "problems/CVRP.h"
#include "solutions/Solution.h"
#include "tools/RandomNumber.h"
#include "tools/operators/interval/CVRP_Repair.h"  
#include "tools/StringHandler.h"                  

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <iterator>


static std::vector<int> getCustomerList(Solution& s, CVRP* cvrp) {
    std::vector<int> customers;
    Interval* vars = s.getDecisionVariables();
    for (int i = 0; i < s.getNumVariables(); ++i) {
        int node = vars[i].L;
        if (cvrp->isCustomer(node)) {
            customers.push_back(node);
        }
    }
    return customers;
}

static void buildChildWithHeuristicX(Solution child, Solution parent1, Solution parent2, CVRP* problem, HeuristicCrossover::HeuristicMode mode) {
    // 1. Obtener datos clave
    std::vector<int> p1 = getCustomerList(parent1, problem);
    std::vector<int> p2 = getCustomerList(parent2, problem);

    if (p1.empty()) return;

    int** cost_matrix = problem->getCost_Matrix();
    int n = static_cast<int>(p1.size());
    RandomNumber& rnd = *RandomNumber::getInstance();

 
    std::unordered_map<int, int> p1_successor_map, p2_successor_map;
    for (int i = 0; i < n; ++i) {
        p1_successor_map[p1[i]] = p1[(i + 1) % n];
        p2_successor_map[p2[i]] = p2[(i + 1) % n];
    }

 
    int max_id = -1;
    for (int v : p1) if (v > max_id) max_id = v;
    for (int v : p2) if (v > max_id) max_id = v;
    std::vector<char> visited(max_id + 1, 0);  
 

    // 3. Inicializar child chromosome y la lista de no visitados
    std::vector<int> child_chromosome;
    child_chromosome.reserve(n);

    std::vector<int> unvisited_nodes = p1;  
    std::unordered_map<int, int> position;
    position.reserve(n * 2);
    for (int i = 0; i < (int)unvisited_nodes.size(); ++i) {
        position[unvisited_nodes[i]] = i;
    }

   
    int start_idx = rnd.nextInt(n - 1);
    int current_node = p1[start_idx];

 
    while ((int)child_chromosome.size() < n) {
        child_chromosome.push_back(current_node);
        // marcar visitado (aseguramos que current_node <= max_id)
        visited[current_node] = 1;

        // Eliminar current_node de unvisited_nodes en O(1) usando position map
        auto posIt = position.find(current_node);
        if (posIt != position.end()) {
            int idx = posIt->second;
            int last_node = unvisited_nodes.back();

            // Si current_node no es el último, swap y actualizar posición del último
            if (idx != (int)unvisited_nodes.size() - 1) {
                unvisited_nodes[idx] = last_node;
                position[last_node] = idx;
            }
            // borrar el último
            unvisited_nodes.pop_back();
            position.erase(posIt);
        }

        if (unvisited_nodes.empty()) break;

        // 6. Identificar candidatos sucesores de cada padre
        int cand1 = p1_successor_map[current_node];
        int cand2 = p2_successor_map[current_node];
        bool is_cand1_valid = (cand1 <= max_id && !visited[cand1]);
        bool is_cand2_valid = (cand2 <= max_id && !visited[cand2]);

        int next_node = -1;

 
        if (is_cand1_valid && is_cand2_valid) {
            int cost1 = cost_matrix[current_node][cand1];
            int cost2 = cost_matrix[current_node][cand2];

            switch (mode) {
            case HeuristicCrossover::GREEDY:
                next_node = (cost1 <= cost2) ? cand1 : cand2;
                break;

            case HeuristicCrossover::RANDOM:
 
                next_node = (rnd.nextInt(1) == 0) ? cand1 : cand2;
                break;

            case HeuristicCrossover::PROBABILISTIC:
                if (cost1 + cost2 == 0) {
                    next_node = (rnd.nextInt(1) == 0) ? cand1 : cand2;
                }
                else {
                    int total_cost = cost1 + cost2;
 
                    if (rnd.nextInt(total_cost - 1) < cost2) {
                        next_node = cand1;
                    }
                    else {
                        next_node = cand2;
                    }
                }
                break;
            }
        }
        else if (is_cand1_valid) {
            next_node = cand1;
        }
        else if (is_cand2_valid) {
            next_node = cand2;
        }
        else {
            // Ambos inválidos -> elegimos un índice aleatorio de unvisited_nodes 
            int sz = static_cast<int>(unvisited_nodes.size());
           
            int rand_idx = rnd.nextInt(sz - 1);
            next_node = unvisited_nodes[rand_idx];
        }

        current_node = next_node;
    }

    // 8. Pasar cromosoma al objeto Solution hijo
    for (size_t i = 0; i < child_chromosome.size(); ++i) {
        child.setVariableValue(i, child_chromosome[i]);
    }
    for (size_t i = child_chromosome.size(); i < child.getNumVariables(); ++i) {
        child.setVariableValue(i, -1);
    }
}


 
void HeuristicCrossover::initialize(Requirements* config) {
    config->addValue("#HeuristicCrossover-mode", Constantes::STRING);
    Parameters param = *(config->load());
    const char* mode_str = param.get("#HeuristicCrossover-mode").getString();

    // Comparar con los nombres actualizados que me diste
    if (StringHandler::compare(mode_str, "Greedy") == 0) {
        this->mode_ = GREEDY;
    }
    else if (StringHandler::compare(mode_str, "Random") == 0) {
        this->mode_ = RANDOM;
    }
    else if (StringHandler::compare(mode_str, "Probabilistic") == 0) {
        this->mode_ = PROBABILISTIC;
    }
    else if (StringHandler::compare(mode_str, "RandomChoice") == 0) {
        this->mode_ = RANDOM_CHOICE;
    }
    else {
        this->mode_ = GREEDY; // Modo por defecto
        std::cout << "Advertencia: Modo '" << mode_str << "' no reconocido para HeuristicCrossover. "
            << "Usando GREEDY por defecto." << std::endl;
    }
}

/**
 * @brief Sobrecarga para ser usado en un operador que combine varios operadores.
 */
void HeuristicCrossover::initialize(HeuristicMode mode) {
    this->mode_ = mode;
}

/**
 * @brief Ejecuta el cruce heurístico sobre los padres para generar los hijos.
 */
void HeuristicCrossover::execute(SolutionSet parents, SolutionSet children) {
    CVRP* problem = static_cast<CVRP*>(parents.get(0).getProblem());
    RandomNumber& rnd = *RandomNumber::getInstance();

    HeuristicMode mode_to_use;
 
    if (this->mode_ == RANDOM_CHOICE) {
        // Si el modo configurado es RANDOM_CHOICE, elige uno de los 3 al azar
        int random_choice = rnd.nextInt(2); 
        mode_to_use = static_cast<HeuristicMode>(random_choice); // Convierte a GREEDY, RANDOM, o PROBABILISTIC
    }
    else {
        // Si el modo es fijo, úsalo directamente
        mode_to_use = this->mode_;
    }

    // Llama a la construcción de hijos con el modo determinado (fijo o aleatorio)
    buildChildWithHeuristicX(children.get(0), parents.get(0), parents.get(1), problem, mode_to_use);
    buildChildWithHeuristicX(children.get(1), parents.get(1), parents.get(0), problem, mode_to_use);

    // Se reparan los hijos para asegurar que sean soluciones válidas
    CVRP_Repair rep;
    rep.execute(children.get(0));
    rep.execute(children.get(1));
}