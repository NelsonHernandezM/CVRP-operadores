#include "miSwapMutation.h"
  

#include "tools/RandomNumber.h"
#include "solutions/SolutionSet.h"
#include "problems/Problem.h"
#include "tools/Requirements.h"
#include "../WindowsRequirements.h"
#include "utility"
#include <vector>
#include <cmath>
#include <problems/CVRP.h>


void miSwapMutation::initialize(Requirements* config) {
    // Configura la probabilidad de mutación.
    config->addValue("#SwapMutation-probability", Constantes::DOUBLE);  // Probabilidad de mutación.

    this->param = *(config->load());  // Cargar parámetros de configuración.
}

void miSwapMutation::execute(Solution y) {

    CVRP* problema = dynamic_cast<CVRP*>(y.getProblem());
   

    RandomNumber* rnd = rnd->getInstance();

    Problem* base = y.getProblem();
 
    int contadorFijos = 0;
    for (int i = 0; i < y.getNumVariables(); i++) {
        int value = y.getVariableValue(i).L;

        if (value == -1 || value == 0) {
            contadorFijos++;
        }
    }

    // Crear arreglos para guardar posiciones y valores fijos
    int* indicesFijos = new int[contadorFijos];
    int* valoresFijos = new int[contadorFijos];
    int k = 0;

    // Llenar arreglos con elementos fijos
    for (int i = 0; i < y.getNumVariables(); i++) {
        int value = y.getVariableValue(i).L;
        if (value==0 || value == -1) {
            indicesFijos[k] = i;
            valoresFijos[k] = value;
            k++;
        }
    }

    // Crear arreglo solo con elementos mutables (clientes normales)
    int numMutables = y.getNumVariables() - contadorFijos;
    int* elementosMutables = new int[numMutables];
    int j = 0;

    // Llenar arreglo con valores mutables
    for (int i = 0; i < y.getNumVariables(); i++) {
        int value = y.getVariableValue(i).L;
        if (value!=0 && value != -1) {
            elementosMutables[j++] = value;
        }
    }

    // Aplicar mutación solo a elementos mutables
    mutationProbability_ = this->param.get("#SwapMutation-probability").getDouble();
    for (int i = 0; i < numMutables; i++) {
        if (rnd->nextDouble() <= mutationProbability_) {
            int nextIdx = (i + 1) % numMutables;
            std::swap(elementosMutables[i], elementosMutables[nextIdx]);
        }
    }

    // Reconstruir solución con elementos fijos y mutables
    j = 0; // Índice para elementos mutables
    for (int i = 0; i < y.getNumVariables(); i++) {
        bool esFijo = false;
        // Verificar si es posición fija
        for (int idx = 0; idx < contadorFijos; idx++) {
            if (indicesFijos[idx] == i) {
                y.setVariableValue(i, valoresFijos[idx]);
                esFijo = true;
                break;
            }
        }
        // Si no es fijo, asignar valor mutable
        if (!esFijo) {
            y.setVariableValue(i, elementosMutables[j++]);
        }
    }


    /* y.getProblem()->evaluate(&y);
     y.getProblem()->evaluateConstraints(&y);*/
     // Liberar memoria
    delete[] indicesFijos;
    delete[] valoresFijos;
    delete[] elementosMutables;

}
