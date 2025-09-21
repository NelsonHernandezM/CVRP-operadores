#include "AlternatingEdgesCrossover.h"

#include "problems/CVRP.h"
#include "solutions/Solution.h"
#include "tools/RandomNumber.h"
#include "tools/operators/interval/CVRP_Repair.h"

#include <vector>
#include <iostream>

// ----------------------------
// Obtener la lista de clientes
// ----------------------------
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
 
static void buildChildWithAEX(Solution child, Solution parent1, Solution parent2, CVRP* problem) {
    // 1. Obtener lista de clientes
    std::vector<int> p1 = getCustomerList(parent1, problem);
    std::vector<int> p2 = getCustomerList(parent2, problem);

    if (p1.empty()) return;

    int n = p1.size();
    RandomNumber& rnd = *RandomNumber::getInstance();

    // 2. Mapas de sucesores (vectores en lugar de hash map)
    std::vector<int> p1_succ(n + 1), p2_succ(n + 1);
    for (int i = 0; i < n; ++i) {
        p1_succ[p1[i]] = p1[(i + 1) % n];
        p2_succ[p2[i]] = p2[(i + 1) % n];
    }

    // 3. Estructuras auxiliares
    std::vector<int> child_chromosome;
    child_chromosome.reserve(n);

    std::vector<char> visited(n + 1, 0); // marca de visitados (0 = no visitado, 1 = visitado)
    std::vector<int> unvisited = p1;     // nodos no visitados, se irán quitando en O(1)

    // 4. Inicializar
    int current_node = p1[0];
    bool is_parent1_turn = true;

    // 5. Construcción del hijo
    while ((int)child_chromosome.size() < n) {
        child_chromosome.push_back(current_node);
        visited[current_node] = 1;

        // Quitar nodo de "unvisited" en O(1)
        auto it = std::find(unvisited.begin(), unvisited.end(), current_node);
        if (it != unvisited.end()) {
            *it = unvisited.back();
            unvisited.pop_back();
        }

        if (unvisited.empty()) break;

        // Selección del sucesor
        int successor = is_parent1_turn ? p1_succ[current_node] : p2_succ[current_node];
        int next_node;

        if (!visited[successor]) {
            next_node = successor;
        }
        else {
            // Elegir nodo aleatorio de los no visitados en O(1)
            int rand_idx = rnd.nextInt((int)unvisited.size() - 1);
            next_node = unvisited[rand_idx];
        }

        current_node = next_node;
        is_parent1_turn = !is_parent1_turn;
    }

    // 6. Pasar al hijo
    for (size_t i = 0; i < child_chromosome.size(); ++i) {
        child.setVariableValue((int)i, child_chromosome[i]);
    }
    for (size_t i = child_chromosome.size(); i < child.getNumVariables(); ++i) {
        child.setVariableValue((int)i, -1);
    }
}

 
void AlternatingEdgesCrossover::initialize(Requirements* config) {
}

void imprimirSolucionAEX(Solution s) {
    Interval* vars = s.getDecisionVariables();
    for (int i = 0; i < s.getNumVariables(); i++) {
        std::cout << vars[i].L << " ";
    }
    std::cout << std::endl;
}

void AlternatingEdgesCrossover::execute(SolutionSet parents, SolutionSet children) {
    CVRP* problem = static_cast<CVRP*>(parents.get(0).getProblem());

 
    buildChildWithAEX(children.get(0), parents.get(0), parents.get(1), problem);
    buildChildWithAEX(children.get(1), parents.get(1), parents.get(0), problem);

 
    CVRP_Repair rep;
    rep.execute(children.get(0));
    rep.execute(children.get(1));
 
    /*std::cout << "PADRE 1 (aex): "; imprimirSolucionAEX(parents.get(0));
    std::cout << "PADRE 2 (aex): "; imprimirSolucionAEX(parents.get(1));
    std::cout << "HIJO 1 (aex): "; imprimirSolucionAEX(children.get(0));
    std::cout << "HIJO 2 (aex): "; imprimirSolucionAEX(children.get(1));
    std::cout << "-------------------------" << std::endl;*/
}
