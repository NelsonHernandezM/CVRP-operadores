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

 
void repararSolucion(Solution& sol) {
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
    // Los clientes eliminados se añaden a una lista para ser reubicados.
    std::vector<int> clientesParaReubicar;
    for (auto& ruta : rutas) {
        while (calcularCarga(ruta, problema) > problema->getMaxCapacity()) {
            int clienteARemover = -1, posCliente = -1;
            double maxDemanda = -1.0;

            // Heurística: remover el cliente con mayor demanda para aliviar la carga rápidamente.
            for (size_t j = 1; j < ruta.size() - 1; ++j) {
                int nodo = ruta[j];
                if (problema->isCustomer(nodo) && problema->getCustomerDemand()[nodo] > maxDemanda) {
                    maxDemanda = problema->getCustomerDemand()[nodo];
                    clienteARemover = nodo;
                    posCliente = j;
                }
            }

            if (clienteARemover != -1) {
                ruta.erase(ruta.begin() + posCliente);
                clientesParaReubicar.push_back(clienteARemover);
            }
            else {
                break; // No hay más clientes que remover, salir del bucle.
            }
        }
    }

    // --- PASO 3: Reubicar clientes (faltantes + extraídos) ---
 
    std::vector<int> todosLosClientesPendientes = clientesFaltantes;
    todosLosClientesPendientes.insert(todosLosClientesPendientes.end(), clientesParaReubicar.begin(), clientesParaReubicar.end());

    for (int cliente : todosLosClientesPendientes) {
        double minCosteDelta = std::numeric_limits<double>::max();
        int mejorRutaIdx = -1;
        int mejorPosicion = -1;

        // Iterar sobre todas las rutas existentes para encontrar la mejor inserción.
        for (size_t k = 0; k < rutas.size(); ++k) {
            // Comprobación rápida de capacidad: si no cabe, no analizar inserciones.
            if (calcularCarga(rutas[k], problema) + problema->getCustomerDemand()[cliente] > problema->getMaxCapacity()) {
                continue;
            }

            // Evaluar el coste de insertar en cada posición posible de la ruta.
            for (size_t pos = 1; pos < rutas[k].size(); ++pos) {
                int nodoPrevio = rutas[k][pos - 1];
                int nodoSiguiente = rutas[k][pos];
 
                double costeOriginal = problema->getCost_Matrix()[nodoPrevio][nodoSiguiente];
                double costeNuevo = problema->getCost_Matrix()[nodoPrevio][cliente] + problema->getCost_Matrix()[cliente][nodoSiguiente];
                double costeDelta = costeNuevo - costeOriginal;

                if (costeDelta < minCosteDelta) {
                    minCosteDelta = costeDelta;
                    mejorRutaIdx = k;
                    mejorPosicion = pos;
                }
            }
        }

        if (mejorRutaIdx != -1) {
            // Se encontró un lugar válido, insertar el cliente.
            rutas[mejorRutaIdx].insert(rutas[mejorRutaIdx].begin() + mejorPosicion, cliente);
        }
        else {
            // No se pudo insertar en ninguna ruta existente. Intentar crear una nueva ruta.
            if (rutas.size() < (size_t)maxVehiculos && problema->getCustomerDemand()[cliente] <= problema->getMaxCapacity()) {
                rutas.push_back({ 0, cliente, 0 }); // Crear nueva ruta {depósito, cliente, depósito}.
            }
 .
        }
    }

    // --- PASO 4: Reconstruir la solución a partir de las rutas reparadas ---
    std::vector<int> nuevaSolucion;
    for (const auto& ruta : rutas) {
        // Solo añadir rutas válidas (con al menos un cliente).
        if (ruta.size() > 2) {
            // Añadir los nodos de la ruta, desde el primer cliente hasta el 0 final.
            nuevaSolucion.insert(nuevaSolucion.end(), ruta.begin() + 1, ruta.end());
        }
    }

  
    std::vector<int> solucionFinal;
    for (const auto& ruta : rutas) {
        if (ruta.size() > 2) {
            // Añade [cliente1, cliente2, ..., 0]
            solucionFinal.insert(solucionFinal.end(), ruta.begin() + 1, ruta.end());
        }
    }


    // Actualizar el objeto 'sol' directamente con la nueva secuencia de variables.
    int n = sol.getNumVariables();
    for (int i = 0; i < n; ++i) {
        if (i < (int)solucionFinal.size()) {
            sol.setVariableValue(i, solucionFinal[i]);
        }
        else {
            sol.setVariableValue(i, -1); // Marcar el resto como no utilizado.
        }
    }
}


 
 
void CVRP_Repair::execute(Solution sol) {
    sol.getProblem()->evaluate(&sol);
    sol.getProblem()->evaluateConstraints(&sol);

    if (sol.getNumberOfViolatedConstraints() > 0) {
        repararSolucion(sol); 

        // Re-evaluar la solución después de la reparación.
        sol.getProblem()->evaluate(&sol);
        sol.getProblem()->evaluateConstraints(&sol);
    }
}

void CVRP_Repair::initialize(Requirements* config) {
    this->param = *(config->load());
}