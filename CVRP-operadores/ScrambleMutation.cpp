 

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

    RandomNumber* rnd = rnd->getInstance();

    if (rnd->nextDouble() > mutationProbability_) {
       
    }else{

    CVRP* problema = dynamic_cast<CVRP*>(y.getProblem());



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
        if (value == 0 || value == -1) {
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
        if (value != 0 && value != -1) {
            elementosMutables[j++] = value;
        }
    }

    

    // a) Obtener dos puntos de corte distintos (pos1 < pos2) https://www.youtube.com/watch?v=lUTcgqbh-rc
    int pos1 = rnd->nextInt(numMutables-1);
    int pos2;
    do {
        pos2 = rnd->nextInt(numMutables-1);
    } while (pos1 == pos2);

    if (pos1 > pos2) {
        std::swap(pos1, pos2); // Aseguramos que pos1 sea el menor
    }

    // b) Realizar varios intercambios aleatorios DENTRO de ese rango
    // El ejemplo usa un bucle de 10 iteraciones. 
    int numeroDeIntercambios = 10;
    int tamanoRango = pos2 - pos1 + 1;

    for (int i = 0; i < numeroDeIntercambios; ++i) {
        // Generar dos índices aleatorios DENTRO del rango [pos1, pos2]
        // Para esto, generamos un número en [0, tamanoRango-1] y le sumamos pos1
        int indiceEnRango1 = pos1 + rnd->nextInt(tamanoRango-1);
        int indiceEnRango2 = pos1 + rnd->nextInt(tamanoRango-1);

        // Intercambiar los elementos en esas posiciones
        std::swap(elementosMutables[indiceEnRango1], elementosMutables[indiceEnRango2]);
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


     // Liberar memoria
    delete[] indicesFijos;
    delete[] valoresFijos;
    delete[] elementosMutables;


}

}
