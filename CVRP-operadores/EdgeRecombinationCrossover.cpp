
#pragma once
#include "EdgeRecombinationCrossover.h"  
 
#include "problems/CVRP.h"
#include "solutions/Solution.h"
#include "tools/RandomNumber.h"
#include "tools/operators/interval/CVRP_Repair.h"  

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>

// =========================================================================
// Funciones de Utilidad (static)
// =========================================================================

/**
 * @brief Extrae la lista de clientes de una solución, ignorando depósitos y splitters.
 * Esta función es un requisito previo para que ERX funcione con cromosomas puros de clientes.
 */
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

/**
 * @brief Construye el mapa de adyacencias a partir de dos padres.
 * Para cada cliente, almacena un conjunto de sus vecinos en ambos padres.
 * Considera las rutas como cíclicas (el último cliente es vecino del primero).
 */
static void buildAdjacencyMap(
    const std::vector<int>& p1,
    const std::vector<int>& p2,
    std::unordered_map<int, std::unordered_set<int>>& adj_map) {

    auto add_parent_edges = [&](const std::vector<int>& p) {
        if (p.empty()) return;
        int n = p.size();
        for (int i = 0; i < n; ++i) {
            int current_node = p[i];
            int prev_node = p[(i + n - 1) % n]; // Vecino anterior (con ciclo)
            int next_node = p[(i + 1) % n];     // Vecino siguiente (con ciclo)
            adj_map[current_node].insert(prev_node);
            adj_map[current_node].insert(next_node);
        }
        };

    add_parent_edges(p1);
    add_parent_edges(p2);
}

static void buildChildWithERX(Solution child, Solution parent1, Solution parent2, CVRP* problem) {
    // 1. Obtener lista de clientes de ambos padres
    std::vector<int> p1 = getCustomerList(parent1, problem);
    std::vector<int> p2 = getCustomerList(parent2, problem);

    if (p1.empty()) return;

    int n = p1.size();
    RandomNumber& rnd = *RandomNumber::getInstance();

    // 2. Construir adyacencias (clientes van del 1..N)
    std::vector<std::vector<int>> adj(n + 1);
    auto add_edges = [&](const std::vector<int>& p) {
        int m = p.size();
        for (int i = 0; i < m; i++) {
            int curr = p[i];
            int prev = p[(i + m - 1) % m];
            int next = p[(i + 1) % m];
            if (std::find(adj[curr].begin(), adj[curr].end(), prev) == adj[curr].end())
                adj[curr].push_back(prev);
            if (std::find(adj[curr].begin(), adj[curr].end(), next) == adj[curr].end())
                adj[curr].push_back(next);
        }
        };
    add_edges(p1);
    add_edges(p2);

    // 3. Inicializar visitados
    std::vector<char> visited(n + 1, false);

    // 4. Nodo inicial aleatorio
    int current = p1[rnd.nextInt(n - 1)];

    std::vector<int> child_chromosome;
    child_chromosome.reserve(n);

    // 5. Construcción iterativa
    while ((int)child_chromosome.size() < n) {
        child_chromosome.push_back(current);
        visited[current] = true;

        // Eliminar current de sus vecinos
        for (int neighbor : adj[current]) {
            auto& list = adj[neighbor];
            list.erase(std::remove(list.begin(), list.end(), current), list.end());
        }
        adj[current].clear();

        if ((int)child_chromosome.size() == n) break;

        // Selección del siguiente
        int next = -1;
        if (!adj[current].empty()) {
            // Vecino con menos conexiones
            int min_deg = INT_MAX;
            for (int neighbor : adj[current]) {
                if (!visited[neighbor] && (int)adj[neighbor].size() < min_deg) {
                    min_deg = adj[neighbor].size();
                    next = neighbor;
                }
            }
        }

        if (next == -1) {
            // Elegir al azar de los no visitados
            std::vector<int> remaining;
            for (int i = 1; i <= n; i++) {
                if (!visited[i]) remaining.push_back(i);
            }
            next = remaining[rnd.nextInt((int)remaining.size() - 1)];
        }

        current = next;
    }

    // 6. Copiar cromosoma al hijo
    for (size_t i = 0; i < child_chromosome.size(); ++i) {
        child.setVariableValue(i, child_chromosome[i]);
    }
    for (size_t i = child_chromosome.size(); i < child.getNumVariables(); ++i) {
        child.setVariableValue(i, -1);
    }
}


 
void imprimirSolucionERX(Solution s) {
    Interval* vars = s.getDecisionVariables();
    for (int i = 0; i < s.getNumVariables(); i++) {
        std::cout << vars[i].L << " ";
    }
    std::cout << std::endl;
}

 

void EdgeRecombinationCrossover::initialize(Requirements* config) {
  
}

void EdgeRecombinationCrossover::execute(SolutionSet parents, SolutionSet children) {
    CVRP* problem = static_cast<CVRP*>(parents.get(0).getProblem());

    // Crear el primer hijo
    buildChildWithERX(children.get(0), parents.get(0), parents.get(1), problem);

    // Crear el segundo hijo (intercambiando el rol de los padres)
    buildChildWithERX(children.get(1), parents.get(1), parents.get(0), problem);

     
    CVRP_Repair rep;
    rep.execute(children.get(0));
    rep.execute(children.get(1));

   
     //std::cout << "PADRE 1 (ERX): "; imprimirSolucionERX(parents.get(0));
     //std::cout << "PADRE 2 (ERX): "; imprimirSolucionERX(parents.get(1));
     //std::cout << "HIJO 1 (ERX): "; imprimirSolucionERX(children.get(0));
     //std::cout << "HIJO 2 (ERX): "; imprimirSolucionERX(children.get(1));
     //std::cout << "-------------------------" << std::endl;
}