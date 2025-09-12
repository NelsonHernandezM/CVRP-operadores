
#include "miBRBAX.h"
#include "problems/CVRP.h"
#include "solutions/Solution.h"
#include "solutions/SolutionSet.h"
#include "tools/RandomNumber.h"
#include "miCVRP_Repair.h"
#include <vector>
#include <numeric>
#include <algorithm>
#include <utility>
#include <random>

// Estructura para representar una ruta
struct Route {
    std::vector<int> customers;
    int total_demand = 0;
    double total_cost = 0.0;
    int splitter_id = -1;
};

// Nueva estructura para pre-calcular el valor de bondad
struct OptimizedRoute {
    Route route;
    double goodness;
};

// =========================================================================
// Funciones de Utilidad (pueden ser static para el archivo .cpp)
// =========================================================================

/**
 * @brief Extrae las rutas de una solución.
 */
static std::vector<Route> extractRoutes( Solution s, CVRP* cvrp) {
    std::vector<Route> routes;
    Route current_route;
    int depot_id = 0;
    Interval* vars = s.getDecisionVariables();

    for (int i = 0; i < s.getNumVariables(); ++i) {
        int node = vars[i].L;

        if (cvrp->isCustomer(node)) {
            if (!current_route.customers.empty()) {
                int prev_node = current_route.customers.back();
                current_route.total_cost += cvrp->getCost_Matrix()[prev_node][node];
            }
            current_route.customers.push_back(node);
            current_route.total_demand += cvrp->getCustomerDemand()[node];
        }
        else if (node != depot_id) {
            current_route.splitter_id = node;
            routes.push_back(current_route);
            current_route = Route(); // Resetear para la siguiente ruta
        }
    }
    if (!current_route.customers.empty()) {
        routes.push_back(current_route);
    }
    return routes;
}

/**
 * @brief Pre-calcula el valor de "bondad" para cada ruta.
 */
static std::vector<OptimizedRoute> calculateGoodness(const std::vector<Route>& routes, CVRP* cvrp) {
    std::vector<OptimizedRoute> opt_routes;
    for (const auto& route : routes) {
        opt_routes.push_back({ route, (double)route.total_demand / cvrp->getMaxCapacity() - route.total_cost });
    }
    return opt_routes;
}

/**
 * @brief Construye un hijo a partir de las rutas heredadas y los genes del padre complementario.
 */
static void buildChild(Solution child,  Solution parent_to_fill, const std::vector<Route>& inherited_routes, CVRP* problem) {
    int child_idx = 0;
    std::vector<bool> used_genes(problem->getDimension(), false);

    // Fase 1: Copiar rutas heredadas
    for (const auto& route : inherited_routes) {
        for (int customer : route.customers) {
            child.setVariableValue(child_idx++, customer);
            used_genes[customer] = true;
        }
        if (route.splitter_id != -1) {
            child.setVariableValue(child_idx++, route.splitter_id);
        }
    }

    // Fase 2: Rellenar con los genes no usados del padre complementario
    Interval* vars_fill = parent_to_fill.getDecisionVariables();
    int n = parent_to_fill.getNumVariables();
    for (int i = 0; i < n; ++i) {
        int gene = vars_fill[i].L;
        if (gene != -1 && !used_genes[gene]) {
            child.setVariableValue(child_idx++, gene);
            used_genes[gene] = true;
        }
    }
}

// =========================================================================
// Miembro de la Clase miBRBAX
// =========================================================================

void miBRBAX::execute(SolutionSet parents, SolutionSet children) {
    CVRP* problem = static_cast<CVRP*>(parents.get(0).getProblem());

    // Extraer y evaluar las rutas de ambos padres
    std::vector<Route> routes_p1 = extractRoutes(parents.get(0), problem);
    std::vector<Route> routes_p2 = extractRoutes(parents.get(1), problem);

    // Pre-calcular la bondad y ordenar
    std::vector<OptimizedRoute> opt_routes_p1 = calculateGoodness(routes_p1, problem);
    std::vector<OptimizedRoute> opt_routes_p2 = calculateGoodness(routes_p2, problem);

    std::sort(opt_routes_p1.begin(), opt_routes_p1.end(), [](const OptimizedRoute& a, const OptimizedRoute& b) {
        return a.goodness > b.goodness;
        });
    std::sort(opt_routes_p2.begin(), opt_routes_p2.end(), [](const OptimizedRoute& a, const OptimizedRoute& b) {
        return a.goodness > b.goodness;
        });

    // Seleccionar las mejores m/2 rutas de cada padre
    int num_to_select_p1 = opt_routes_p1.size() / 2;
    std::vector<Route> inherited_routes_from_p1;
    for (int i = 0; i < num_to_select_p1; ++i) {
        inherited_routes_from_p1.push_back(opt_routes_p1[i].route);
    }

    int num_to_select_p2 = opt_routes_p2.size() / 2;
    std::vector<Route> inherited_routes_from_p2;
    for (int i = 0; i < num_to_select_p2; ++i) {
        inherited_routes_from_p2.push_back(opt_routes_p2[i].route);
    }

    // Construir los hijos
    Solution child1 = children.get(0);
    buildChild(child1, parents.get(1), inherited_routes_from_p1, problem);

    Solution child2 = children.get(1);
    buildChild(child2, parents.get(0), inherited_routes_from_p2, problem);

    // Reparar ambos hijos
    CVRP_Repair rep2;
    rep2.execute(children.get(0));
    rep2.execute(children.get(1));
}

void miBRBAX::initialize(Requirements* config) {
    
}