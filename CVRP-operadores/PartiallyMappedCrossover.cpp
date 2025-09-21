#include "PartiallyMappedCrossover.h"

//#include "../../../../../include/tools/operators/interval/MOSADOperators/PMXCrossover.h" // <-- RECOMENDACIÓN: Renombrar el header
//#include "../../../../../include/tools/RandomNumber.h"
//#include "../../../../../include/problems/Problem.h"

#include <vector>
#include <numeric>
#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

//#include "../../../../../include/tools/operators/interval/CVRP_Repair.h"
//#include "../../../../../include/tools/operators/interval/TournamentSelection.h"
//#include "../../../../../include/problems/CVRP.h"


 

void PartiallyMappedCrossover::initialize(Requirements* config) {
 
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

static void buildChildWithPMX(Solution child,  Solution parent1,  Solution parent2, CVRP* problem) {
    // 1. Obtener la lista de clientes de ambos padres
    std::vector<int> p1 = getCustomerList(parent1, problem);
    std::vector<int> p2 = getCustomerList(parent2, problem);

    int n = p1.size()-1;
    
    if (n < 2) {
        //// Si hay menos de 2 clientes, simplemente copia el padre 1
        //for (size_t i = 0; i < n; ++i) child.setVariableValue(i, p1[i]);
        //for (size_t i = n; i < child.getNumVariables(); ++i) child.setVariableValue(i, -1);
        return;
    }

    RandomNumber& rnd = *RandomNumber::getInstance();

    // 2. Elegir dos puntos de corte
    int cut1 = rnd.nextInt(n - 1);                   // [0 .. n-2]
    int cut2 = cut1 + 1 + rnd.nextInt(n - cut1 - 1); // [cut1+1 .. n-1]


    std::vector<int> child_chromosome(p1.size(), -1);
    std::unordered_map<int, int> mapping;      // p2[i] -> p1[i]
    std::unordered_set<int> segment_genes;     // genes del segmento

    // 3. Copiar el segmento del padre 1 y construir mapeo
    for (int i = cut1; i <= cut2; ++i) {
        child_chromosome[i] = p1[i];
        mapping[p2[i]] = p1[i];
        segment_genes.insert(p1[i]);
    }

    // 4. Llenar el resto con genes del padre 2
    for (int i = 0; i < n; ++i) {
        if (i >= cut1 && i <= cut2) continue;

        int gene_from_p2 = p2[i];
        // Resolver conflictos usando el mapeo
        while (segment_genes.count(gene_from_p2) > 0) {
            gene_from_p2 = mapping[gene_from_p2];
        }
        child_chromosome[i] = gene_from_p2;
    }

    // 5. Pasar cromosoma al hijo
    for (size_t i = 0; i < child_chromosome.size(); ++i) {
        child.setVariableValue(i, child_chromosome[i]);
    }
    for (size_t i = child_chromosome.size(); i < child.getNumVariables(); ++i) {
        child.setVariableValue(i, -1);
    }
}


void imprimirSolucionCCC(Solution s) {
    Interval* vars = s.getDecisionVariables();
    for (int i = 0; i < s.getNumVariables(); i++) {
        std::cout << vars[i].L << " ";
    }
    std::cout << std::endl;
}


void PartiallyMappedCrossover::execute(SolutionSet parents, SolutionSet children) {
    TournamentSelection Torneo;
 

    // Se asume que el SolutionSet 'parents' ya contiene a los padres seleccionados.
    // Si no es así, la selección por torneo es una buena opción:

    //parents.set(0, Torneo.execute(parents, 2));
    //parents.set(1, Torneo.execute(parents, 2));
    CVRP_Repair rep;
    rep.execute(parents.get(0));
    rep.execute(parents.get(1));

    std::cout << "PADRE 1: "; imprimirSolucionCCC(parents.get(0));
    std::cout << "PADRE 2: "; imprimirSolucionCCC(parents.get(1));


    CVRP* problem = static_cast<CVRP*>(parents.get(0).getProblem());

   

    // Crear el primer hijo
    buildChildWithPMX(children.get(0), parents.get(0), parents.get(1), problem);

    // Crear el segundo hijo (intercambiando el rol de los padres)
    buildChildWithPMX(children.get(1), parents.get(1), parents.get(0), problem);

    // Reparar ambos hijos para reinsertar los depósitos (splitters) y asegurar que sean soluciones válidas
  
    rep.execute(children.get(0));
    rep.execute(children.get(1));

    /// depuración

     std::cout << "HIJO 1: "; imprimirSolucionCCC(children.get(0));
     std::cout << "HIJO 2: "; imprimirSolucionCCC(children.get(1));
     std::cout << "-------------------------" << std::endl;
}