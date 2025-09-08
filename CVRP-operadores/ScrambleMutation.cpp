 

#include "ScrambleMutation.h"


#include "tools/RandomNumber.h"
#include "solutions/SolutionSet.h"
#include "problems/Problem.h"
#include "tools/Requirements.h"
#include "../WindowsRequirements.h"
#include "utility"
#include <vector>
#include <cmath>
#include <problems/CVRP.h>


void ScrambleMutation::initialize(Requirements* config) {
    // Configura la probabilidad de mutación.
    config->addValue("#ScrambleMutation-probability", Constantes::DOUBLE);  // Probabilidad de mutación.

    this->param = *(config->load());  // Cargar parámetros de configuración.
    mutationProbability_ = this->param.get("#ScrambleMutation-probability").getDouble();
}


void ScrambleMutation::execute(Solution y) {
    RandomNumber* rnd = RandomNumber::getInstance();

    // 1. Aplicar la mutación solo si se cumple la probabilidad
    if (rnd->nextDouble() > mutationProbability_) {
        return; // Salir temprano si no hay mutación
    }

    
    std::vector<int> clientes;
    std::vector<int> indices_clientes;
    clientes.reserve(y.getNumVariables());
    indices_clientes.reserve(y.getNumVariables());

    for (int i = 0; i < y.getNumVariables(); ++i) {
        int value = y.getVariableValue(i).L;
        if (value > 0) { // usar ISDEPOT //
            clientes.push_back(value);
            indices_clientes.push_back(i);
        }
    }

    // Si no hay suficientes clientes para mutar, salir.
    if (clientes.size() < 2) {
        return;
    }

    // 3. Seleccionar el rango para la mutación
    int numMutables = clientes.size();

    // a) Obtener dos puntos de corte distintos
    int pos1 = rnd->nextInt(numMutables - 1);
    int pos2;
    do {
        pos2 = rnd->nextInt(numMutables - 1);
    } while (pos1 == pos2);

    if (pos1 > pos2) {
        std::swap(pos1, pos2);
    }

    // b) Barajar el sub-rango [pos1, pos2] usando Fisher-Yates.
    for (int i = pos2; i > pos1; --i) {
        // Elige un índice aleatorio 'j' en la parte no barajada del rango [pos1, i]
        int j = pos1 + rnd->nextInt(i - pos1);

        // Intercambia el elemento actual con el elemento aleatorio
        std::swap(clientes[i], clientes[j]);
    }

    // 4. Reconstruir la solución con los clientes barajados// Cambiar logica para usar solo solution, y no vectores o nose
    for (size_t i = 0; i < clientes.size(); ++i) {
        y.setVariableValue(indices_clientes[i], clientes[i]);
    }
}