#include "CVRP_Repair.h"    
#include <problems/CVRP.h>  

#include <algorithm>
#include <vector>
#include <set>
#include <iostream>
#include <limits> // Necesario para std::numeric_limits
 

int calcularCarga(const std::vector<int>& ruta, CVRP* problema) {
    int carga = 0;
    for (int nodo : ruta) {
        if (problema->isCustomer(nodo)) {
            carga += problema->getCustomerDemand()[nodo];
        }
    }
    return carga;
}

// --- FUNCIÓN AUXILIAR PARA CALCULAR LA DISTANCIA TOTAL DE UNA RUTA ---
// Se usa para evaluar el coste final o cuando el cálculo delta no es posible.
int calcularDistanciaRuta(const std::vector<int>& ruta, CVRP* problema) {
    int distanciaTotal = 0;
    if (ruta.size() < 2) {
        return 0;
    }

    for (size_t i = 0; i < ruta.size() - 1; ++i) {
        int nodoDesde = ruta[i];
        int nodoHasta = ruta[i + 1];
        distanciaTotal += problema->getCost_Matrix()[nodoDesde][nodoHasta];
    }
    return distanciaTotal;
}
 
std::vector<std::vector<int>> separarSolucionPorRutas(Solution* s) {
    std::vector<std::vector<int>> rutas;
    if (s->getNumVariables() == 0) return rutas;

    std::vector<int> rutaActual;
    Interval* vars = s->getDecisionVariables();

    rutaActual.push_back(0); // Toda ruta empieza en el depósito.

    for (int i = 0; i < s->getNumVariables(); ++i) {
        int nodo = vars[i].L;

        if (nodo == -1) { // Fin de todas las rutas.
            break;
        }

        if (nodo == 0) { // Fin de una ruta.
            if (rutaActual.size() > 1) { // Si la ruta tiene al menos un cliente.
                rutaActual.push_back(0);
                rutas.push_back(rutaActual);
            }
            rutaActual.clear();
            rutaActual.push_back(0); // Iniciar la siguiente ruta.
        }
        else {
            rutaActual.push_back(nodo);
        }
    }

    // Asegurarse de añadir la última ruta si no terminó explícitamente con un 0 o -1.
    if (rutaActual.size() > 1) {
        rutaActual.push_back(0);
        rutas.push_back(rutaActual);
    }

    return rutas;
}

void repararSolucion(Solution sol) {
    CVRP* problema = dynamic_cast<CVRP*>(sol.getProblem());
    if (!problema) return;

    // Obtener la estructura de rutas actual de la solución.
    std::vector<std::vector<int>> rutas = separarSolucionPorRutas(&sol);
    int numClientes = problema->getNumberCustomers();
    int maxVehiculos = problema->getNumVehicles();

    // --- PASO 1: Identificar clientes faltantes en la solución ---
    std::set<int> clientesPresentes;
    for (const auto& ruta : rutas) {
        for (int nodo : ruta) {
            if (problema->isCustomer(nodo)) {
                clientesPresentes.insert(nodo);
            }
        }
    }

    std::vector<int> clientesFaltantes;
    for (int i = 1; i <= numClientes; ++i) {
        if (clientesPresentes.find(i) == clientesPresentes.end()) {
            clientesFaltantes.push_back(i);
        }
    }

    // --- PASO 2: Reparar rutas con exceso de capacidad ---
    std::vector<int> clientesParaReubicar;
    for (auto& ruta : rutas) {
        int cargaActual = calcularCarga(ruta, problema);
        while (cargaActual > problema->getMaxCapacity()) {
         //remover el primer cliente que se encuentre.
            int clienteARemover = -1;
            int posCliente = -1;

            // Empezamos en 1 para no remover el depósito inicial. //YA QUE EN RUTAS SI SE EMPIEZA EN 0, EN SOLUTION SE EMPIEZA EN UN CLIENTE
            for (size_t j = 1; j < ruta.size() - 1; ++j) {
                int nodo = ruta[j];
                if (problema->isCustomer(nodo)) {
                    clienteARemover = nodo;
                    posCliente = j;
                    break; // Encontramos un cliente, salimos para removerlo.
                }
            }

            if (clienteARemover != -1) {
                ruta.erase(ruta.begin() + posCliente);
                clientesParaReubicar.push_back(clienteARemover);
                // Recalculamos la carga para la siguiente iteración del while.
                cargaActual = calcularCarga(ruta, problema);
            }
            else {
                
                break;
            }
        }
    }

    // --- PASO 3: Reubicar clientes (faltantes + extraídos) con lógica de "Primer Ajuste" ---
    std::vector<int> todosLosClientesPendientes = clientesFaltantes;
    todosLosClientesPendientes.insert(todosLosClientesPendientes.end(), clientesParaReubicar.begin(), clientesParaReubicar.end());

    for (int cliente : todosLosClientesPendientes) {
        bool clienteReubicado = false;

        // 1. Intentar insertar en la primera ruta existente donde quepa.
        for (size_t k = 0; k < rutas.size(); ++k) {
            int cargaActualRuta = calcularCarga(rutas[k], problema);
            if (cargaActualRuta + problema->getCustomerDemand()[cliente] <= problema->getMaxCapacity()) {
                // Hay espacio. Insertamos el cliente al final de la ruta (antes del último depósito).
                rutas[k].insert(rutas[k].end() - 1, cliente);
                clienteReubicado = true;
                break; // Cliente reubicado, pasamos al siguiente cliente pendiente.
            }
        }

        // 2. Si no cupo en ninguna ruta, intentar crear una nueva.
        if (!clienteReubicado) {
            if (rutas.size() < (size_t)maxVehiculos && problema->getCustomerDemand()[cliente] <= problema->getMaxCapacity()) {
                rutas.push_back({ 0, cliente, 0 }); // Crear nueva ruta {depósito, cliente, depósito}.
            }
            // Nota: Si el cliente no cabe aquí, se quedará sin asignar,
            // pero la solución resultante será (potencialmente) factible con los clientes que sí se pudieron asignar.
        }
    }

    // --- PASO 4: Reconstruir la solución a partir de las rutas reparadas (simplificado) ---
    std::vector<int> solucionFinal;
    for (const auto& ruta : rutas) {
        // Solo añadir rutas que efectivamente visitan al menos un cliente.
        if (ruta.size() > 2) { 
            // Añade los nodos de la ruta, desde el primer cliente hasta el 0 final.
            // Ejemplo: de {0, 5, 8, 0} -> añade {5, 8, 0}
            solucionFinal.insert(solucionFinal.end(), ruta.begin() + 1, ruta.end());
        }
    }

    // Actualizar el objeto 'sol' directamente con la nueva secuencia de variables.
    int n = sol.getNumVariables();
    for (int i = 0; i < n-1; ++i) {
        if (i < (int)solucionFinal.size()) {
            sol.setVariableValue(i, solucionFinal[i]);
        }
        else {
            sol.setVariableValue(i, -1); // Marcar el resto de variables como no utilizadas.
        }
    
    }
    sol.setVariableValue(sol.getNumVariables(), -1);
 
}
void imprimirSolucionC(Solution sol) {

    for (int j = 0; j < sol.getNumVariables(); j++)
    {
        cout << sol.getVariableValue(j);
        if (j < sol.getNumVariables() - 1) {
            cout << ", ";
        }

    }
    cout << " " << endl;


}
 
void CVRP_Repair::execute(Solution sol) {
    sol.getProblem()->evaluate(&sol);
    sol.getProblem()->evaluateConstraints(&sol);

    if (sol.getNumberOfViolatedConstraints() > 0) {
        repararSolucion(sol); 
     /*   cout << "imprimir" << endl;*/
       /*imprimirSolucionC(sol);*/
        // Re-evaluar la solución después de la reparación.
        sol.getProblem()->evaluate(&sol);
        sol.getProblem()->evaluateConstraints(&sol);
    }
}


void CVRP_Repair::initialize(Requirements* config) {
    this->param = *(config->load());
}