#include "miCVRP_Repair.h"
#include <problems/CVRP.h>

#include <algorithm>
#include <vector>
#include <set>
#include <iostream>
#include <limits> // Necesario para std::numeric_limits
#include <chrono>

// --- FUNCIONES AUXILIARES (sin cambios) ---
int calcularCarga(const std::vector<int>& ruta, CVRP* problema) {
    int carga = 0;
    for (int nodo : ruta) {
        if (problema->isCustomer(nodo)) {
            carga += problema->getCustomerDemand()[nodo];
        }
    }
    return carga;
}

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

    if (rutaActual.size() > 1) {
        rutaActual.push_back(0);
        rutas.push_back(rutaActual);
    }
    return rutas;
}


// --- FUNCIÓN DE REPARACIÓN (MODIFICADA) ---
void repararSolucion(Solution sol) {
    CVRP* problema = dynamic_cast<CVRP*>(sol.getProblem());
    if (!problema) return;

    // Obtener la estructura de rutas actual de la solución.
    std::vector<std::vector<int>> rutas = separarSolucionPorRutas(&sol);
    int numClientes = problema->getNumberCustomers();
    int maxVehiculos = problema->getNumVehicles();

    // Lista para clientes que necesitan ser reubicados (por exceso de capacidad, rutas eliminadas, etc.)
    std::vector<int> clientesParaReubicar;

    // --- PASO 0: Eliminar clientes duplicados ---
    std::set<int> vistos;
    for (auto& ruta : rutas) {
        for (size_t j = 1; j < ruta.size() - 1; ) { // empezamos en 1 para no tocar depósito inicial
            int nodo = ruta[j];
            if (problema->isCustomer(nodo)) {
                if (vistos.find(nodo) != vistos.end()) {
                    // Nodo duplicado, eliminarlo
                    ruta.erase(ruta.begin() + j);
                    continue; // no incrementamos j porque la ruta se desplazó
                }
                else {
                    vistos.insert(nodo);
                }
            }
            ++j;
        }
    }

    // --- (NUEVO) PASO 0.5: Reducir el número de rutas si excede maxVehiculos ---
    // Si hay más rutas que vehículos, se eliminan las más cortas y sus clientes se reubican.
    while (rutas.size() > (size_t)maxVehiculos) {
        // Encontrar la ruta más corta (con menos clientes) para eliminarla.
        // Es una heurística simple: es más fácil reubicar a pocos clientes.
        auto it_ruta_a_eliminar = std::min_element(rutas.begin(), rutas.end(),
            [](const std::vector<int>& a, const std::vector<int>& b) {
                // Comparamos el número de clientes (tamaño total menos los dos depósitos)
                return a.size() < b.size();
            });

        if (it_ruta_a_eliminar != rutas.end() && it_ruta_a_eliminar->size() > 2) {
            // Añadir sus clientes (excepto los depósitos 0) a la lista de reubicación.
            for (size_t i = 1; i < it_ruta_a_eliminar->size() - 1; ++i) {
                clientesParaReubicar.push_back((*it_ruta_a_eliminar)[i]);
            }
            // Eliminar la ruta de la lista de rutas.
            rutas.erase(it_ruta_a_eliminar);
        }
        else {
            // Si no se encuentra una ruta válida para eliminar, salir del bucle para evitar un ciclo infinito.
            break;
        }
    }

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
    for (auto& ruta : rutas) {
        int cargaActual = calcularCarga(ruta, problema);
        while (cargaActual > problema->getMaxCapacity()) {
            int clienteARemover = -1;
            int posCliente = -1;

            // Empezamos en 1 para no remover el depósito inicial.
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
                cargaActual = calcularCarga(ruta, problema); // Recalcular carga
            }
            else {
                break; // No hay más clientes que remover en esta ruta
            }
        }
    }

    // --- PASO 3: Reubicar clientes (faltantes + extraídos) con lógica de "Primer Ajuste" ---
    std::vector<int> todosLosClientesPendientes = clientesFaltantes;
    todosLosClientesPendientes.insert(todosLosClientesPendientes.end(), clientesParaReubicar.begin(), clientesParaReubicar.end());

    for (int cliente : todosLosClientesPendientes) {
        bool clienteReubicado = false;

        // 1. Intentar insertar en la primera ruta existente donde quepa.
        for (auto& ruta : rutas) {
            int cargaActualRuta = calcularCarga(ruta, problema);
            if (cargaActualRuta + problema->getCustomerDemand()[cliente] <= problema->getMaxCapacity()) {
                ruta.insert(ruta.end() - 1, cliente); // Insertar antes del último '0'
                clienteReubicado = true;
                break;
            }
        }

        // 2. Si no cupo en ninguna ruta, intentar crear una nueva si aún hay vehículos disponibles.
        if (!clienteReubicado) {
            if (rutas.size() < (size_t)maxVehiculos && problema->getCustomerDemand()[cliente] <= problema->getMaxCapacity()) {
                rutas.push_back({ 0, cliente, 0 }); // Crear nueva ruta {depósito, cliente, depósito}.
            }
        }
    }

    // --- PASO 4: Reconstruir la solución a partir de las rutas reparadas ---
    std::vector<int> solucionFinal;
    for (const auto& ruta : rutas) {
        if (ruta.size() > 2) { // Solo añadir rutas que visitan al menos un cliente.
            // Añade {cliente1, cliente2, ..., 0}
            solucionFinal.insert(solucionFinal.end(), ruta.begin() + 1, ruta.end());
        }
    }

    // Actualizar el objeto 'sol' directamente con la nueva secuencia de variables.
    int n = sol.getNumVariables();
    for (int i = 0; i < solucionFinal.size(); ++i) {
        
            sol.setVariableValue(i, solucionFinal[i]);
         
      
    }
    sol.setVariableValue(sol.getNumVariables(), -1);
}


// --- Resto del archivo (sin cambios) ---
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

void miCVRP_Repair::execute(Solution sol) {
    sol.getProblem()->evaluate(&sol);
    sol.getProblem()->evaluateConstraints(&sol);

    if (sol.getNumberOfViolatedConstraints() > 0) {
        //// Tomamos el tiempo de inicio
        //auto start = std::chrono::high_resolution_clock::now();

        repararSolucion(sol);
        // Re-evaluar la solución después de la reparación.
        sol.getProblem()->evaluate(&sol);
        sol.getProblem()->evaluateConstraints(&sol);

        //// Tomamos el tiempo de finalización
        //auto end = std::chrono::high_resolution_clock::now();

        //// --- Alternativa ---
        //// Define una duración en segundos usando un tipo 'double'
        //using floating_seconds = std::chrono::duration<double>;

        //// Calcula la duración y conviértela directamente a segundos decimales
        //double seconds = std::chrono::duration_cast<floating_seconds>(end - start).count();

        ////// Imprimimos el resultado en segundos
        ////std::cout << "--------------------------------------------------" << std::endl;
        ////std::cout << "Tiempo de ejecución de repair: " << seconds << " s" << std::endl;
        ////std::cout << "--------------------------------------------------" << std::endl;

    }



}

void miCVRP_Repair::initialize(Requirements* config) {
    this->param = *(config->load());
}