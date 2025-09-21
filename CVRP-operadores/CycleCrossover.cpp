#include "CycleCrossover.h"

#include "problems/CVRP.h"
#include "solutions/Solution.h"
#include "tools/operators/interval/CVRP_Repair.h"  


#include <vector>
#include <unordered_map>
#include <iostream>
void imprimirSolucionCY(Solution s) {
    Interval* vars = s.getDecisionVariables();
    for (int i = 0; i < s.getNumVariables(); i++) {
        std::cout << vars[i].L << " ";
    }
    std::cout << std::endl;
}

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

 
static void buildChildWithCX(Solution child, Solution parent1, Solution parent2, CVRP* problem) {
    // 1. Obtener la lista de clientes de ambos padres
    std::vector<int> p1 = getCustomerList(parent1, problem);
    std::vector<int> p2 = getCustomerList(parent2, problem);

    if (p1.empty()) {
        return; // No hay clientes para cruzar
    }

    int n = p1.size();

    // Cromosoma del hijo (sin inicializar con valores) y vector de control
    std::vector<int> child_chromosome(n);
    std::vector<bool> filled_indices(n, false); // ✅ Vector de control, todo inicializado a 'falso'

    // 2. Crear un mapa para buscar eficientemente la posición de un gen en el padre 1
    std::unordered_map<int, int> p1_gene_to_index_map;
    p1_gene_to_index_map.reserve(n); // Optimización: reservar memoria
    for (int i = 0; i < n; ++i) {
        p1_gene_to_index_map[p1[i]] = i;
    }

    // 3. Identificar y copiar el primer ciclo desde el Padre 1
    int current_index = 0;

    // El bucle se ejecuta mientras el índice actual no haya sido llenado
    while (!filled_indices[current_index]) { // ✅ Comprobación con el vector booleano
        // Marcar la posición actual como llenada
        filled_indices[current_index] = true; // ✅ Actualización del vector de control

        // Copiar el gen del padre 1 en la posición actual
        child_chromosome[current_index] = p1[current_index];

        // Buscar el siguiente eslabón del ciclo
        int gene_from_p2 = p2[current_index];
        current_index = p1_gene_to_index_map[gene_from_p2];
    }

    // 4. Rellenar los huecos restantes con los genes del Padre 2
    for (int i = 0; i < n; ++i) {
        // Si el índice no fue llenado durante el ciclo...
        if (!filled_indices[i]) { // ✅ Comprobación con el vector booleano
            child_chromosome[i] = p2[i];
        }
    }

    // 5. Pasar el cromosoma de clientes al objeto Solution hijo
    for (size_t i = 0; i < child_chromosome.size(); ++i) {
        child.setVariableValue(i, child_chromosome[i]);
    }
    for (size_t i = child_chromosome.size(); i < child.getNumVariables(); ++i) {
        child.setVariableValue(i, -1); // Relleno final si es necesario
    }
}
 

void CycleCrossover::initialize(Requirements* config) {
 
}

void CycleCrossover::execute(SolutionSet parents, SolutionSet children) {
    CVRP* problem = static_cast<CVRP*>(parents.get(0).getProblem());

    // Crear el primer hijo (Ciclo de P1, resto de P2)
    buildChildWithCX(children.get(0), parents.get(0), parents.get(1), problem);

    // Crear el segundo hijo (Ciclo de P2, resto de P1)
    buildChildWithCX(children.get(1), parents.get(1), parents.get(0), problem);

    // Reparar los hijos para que sean soluciones válidas para el CVRP
    CVRP_Repair rep;
    rep.execute(children.get(0));
    rep.execute(children.get(1));

   /* std::cout << "PADRE 1 (CY): "; imprimirSolucionCY(parents.get(0));
   std::cout << "PADRE 2 (CY): "; imprimirSolucionCY(parents.get(1));
   std::cout << "HIJO 1 (CY): "; imprimirSolucionCY(children.get(0));
   std::cout << "HIJO 2 (CY): "; imprimirSolucionCY(children.get(1));
   std::cout << "-------------------------" << std::endl;*/

}