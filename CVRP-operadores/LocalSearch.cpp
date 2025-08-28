#include "LocalSearch.h"
#include "problems/CVRP.h"
#include "tools/RandomNumber.h"
#include "solutions/SolutionSet.h"
#include "problems/Problem.h"
#include "tools/Requirements.h"
#include <vector>
#include <cmath>
#include <utility>
#include <numeric>
#include <algorithm>
#include <cassert>
#include <iostream>

// --- Estructuras de Datos para Mejoras ---
// Estructura para almacenar la información de un movimiento candidato
struct Move {
    double delta_cost = 0.0;
    int type = -1; // 1: swap, 2: 2-opt
    int pos1_route = -1, pos1_idx = -1;
    int pos2_route = -1, pos2_idx = -1;
};

// --- Funciones Auxiliares para Manipulación de Rutas ---

/**
 * @brief Decodifica la solución plana (un solo arreglo) en una estructura de rutas (vector de vectores).
 * La representación de rutas es esencial para la evaluación incremental.
 * @param sol La solución del problema.
 * @param problema Puntero al objeto CVRP para usar el método isDepot.
 * @return Un vector donde cada vector interno representa una ruta de clientes.
 */
std::vector<std::vector<int>> decodificarRutas(Solution& sol, CVRP* problema) {
    std::vector<std::vector<int>> rutas;
    std::vector<int> ruta_actual;
    // La solución se representa como una secuencia de clientes y depósitos (0).
    // Ej: [1, 5, 2, 0, 3, 4, -1] -> Ruta1: [1, 5, 2], Ruta2: [3, 4]
    for (int i = 0; i < sol.getNumVariables(); ++i) {
        int value = sol.getVariableValue(i).L;
        if (value == -1) break; // Fin de la secuencia

        if (problema->isDepot(value)) {
            if (!ruta_actual.empty()) {
                rutas.push_back(ruta_actual);
                ruta_actual.clear();
            }
        }
        else {
            ruta_actual.push_back(value);
        }
    }
    if (!ruta_actual.empty()) {
        rutas.push_back(ruta_actual);
    }
    return rutas;
}

/**
 * @brief Reconstruye la solución plana original a partir de la estructura de rutas optimizada.
 * Esta función es el inverso de decodificarRutas.
 * @param sol Objeto Solution a modificar.
 * @param rutas La estructura de rutas optimizada.
 * @param indicesMutables Los índices originales de la solución que corresponden a los nodos de clientes.
 */
void reconstruirSolucionDesdeRutas(Solution& sol, const std::vector<std::vector<int>>& rutas, const std::vector<int>& indicesMutables) {
    std::vector<int> valores_planos;
    for (size_t i = 0; i < rutas.size(); ++i) {
        for (int nodo : rutas[i]) {
            valores_planos.push_back(nodo);
        }
        // Añadir un separador de depósito (0) entre rutas, excepto para la última.
        if (i < rutas.size() - 1) {
            valores_planos.push_back(0);
        }
    }

    // Rellenar el resto de la solución con -1 (no usado)
    while (valores_planos.size() < sol.getNumVariables()) {
        valores_planos.push_back(-1);
    }

    // Actualizar el objeto Solution
    for (size_t i = 0; i < sol.getNumVariables(); ++i) {
        sol.setVariableValue(i, valores_planos[i]);
    }
}


// --- Funciones de Búsqueda Local con Evaluación Incremental (Best-Improvement) ---

/**
 * @brief TAREA 1, 2 y 4: Explora la vecindad de intercambio (swap) usando best-improvement.
 * Calcula el delta de costo de forma incremental (O(1)) y verifica la factibilidad de capacidad.
 * @return true si se encontró y aplicó una mejora, false en caso contrario.
 */
bool explorarVecindadSwap(std::vector<std::vector<int>>& rutas, CVRP* problema, std::vector<double>& demandas_rutas) {
    Move mejor_movimiento;
    mejor_movimiento.delta_cost = 0;
    int** cost_matrix = problema->getCost_Matrix();
    int* demands = problema->getCustomerDemand();
    int capacity = problema->getMaxCapacity();

    for (size_t r1 = 0; r1 < rutas.size(); ++r1) {
        for (size_t i = 0; i < rutas[r1].size(); ++i) {
            for (size_t r2 = r1; r2 < rutas.size(); ++r2) {
                size_t start_j = (r1 == r2) ? i + 1 : 0;
                for (size_t j = start_j; j < rutas[r2].size(); ++j) {

                    int u = rutas[r1][i];
                    int v = rutas[r2][j];

                    // TAREA 4: Comprobación de factibilidad incremental de capacidad
                    if (r1 != r2) {
                        double nueva_demanda_r1 = demandas_rutas[r1] - demands[u] + demands[v];
                        double nueva_demanda_r2 = demandas_rutas[r2] - demands[v] + demands[u];
                        if (nueva_demanda_r1 > capacity || nueva_demanda_r2 > capacity) {
                            continue; // Movimiento infactible
                        }
                    }

                    // TAREA 1: Cálculo del delta de costo (O(1))
                    double costo_actual = 0;
                    double costo_nuevo = 0;

                    int pred_u = (i > 0) ? rutas[r1][i - 1] : 0; // 0 es el depot
                    int succ_u = (i < rutas[r1].size() - 1) ? rutas[r1][i + 1] : 0;
                    int pred_v = (j > 0) ? rutas[r2][j - 1] : 0;
                    int succ_v = (j < rutas[r2].size() - 1) ? rutas[r2][j + 1] : 0;

                    costo_actual += cost_matrix[pred_u][u] + cost_matrix[u][succ_u];
                    costo_actual += cost_matrix[pred_v][v] + cost_matrix[v][succ_v];

                    if (r1 == r2) { // Swap dentro de la misma ruta
                        if (i + 1 == j) { // Nodos adyacentes
                            costo_nuevo = cost_matrix[pred_u][v] + cost_matrix[v][u] + cost_matrix[u][succ_v];
                            // El arco (u,v) se convierte en (v,u). Restamos el original para no contarlo doble.
                            costo_actual -= cost_matrix[u][v];
                        }
                        else {
                            costo_nuevo = cost_matrix[pred_u][v] + cost_matrix[v][succ_u] + cost_matrix[pred_v][u] + cost_matrix[u][succ_v];
                        }
                    }
                    else { // Swap entre rutas diferentes
                        costo_nuevo = cost_matrix[pred_u][v] + cost_matrix[v][succ_u] + cost_matrix[pred_v][u] + cost_matrix[u][succ_v];
                    }

                    double delta = costo_nuevo - costo_actual;

                    if (delta < mejor_movimiento.delta_cost) {
                        mejor_movimiento.delta_cost = delta;
                        mejor_movimiento.type = 1;
                        mejor_movimiento.pos1_route = r1;
                        mejor_movimiento.pos1_idx = i;
                        mejor_movimiento.pos2_route = r2;
                        mejor_movimiento.pos2_idx = j;
                    }
                }
            }
        }
    }

    // Aplicar el mejor movimiento encontrado (Best-Improvement)
    if (mejor_movimiento.delta_cost < -1e-9) { // Comparar con una pequeña tolerancia para evitar errores de punto flotante
        int r1 = mejor_movimiento.pos1_route;
        int i = mejor_movimiento.pos1_idx;
        int r2 = mejor_movimiento.pos2_route;
        int j = mejor_movimiento.pos2_idx;

        if (r1 != r2) {
            int nodo_u = rutas[r1][i];
            int nodo_v = rutas[r2][j];
            demandas_rutas[r1] += demands[nodo_v] - demands[nodo_u];
            demandas_rutas[r2] += demands[nodo_u] - demands[nodo_v];
        }

        std::swap(rutas[r1][i], rutas[r2][j]);
        return true;
    }
    return false;
}

/**
 * @brief TAREA 2: Implementación de 2-Opt (intra-ruta) con best-improvement.
 * Revisa todas las posibles inversiones de sub-segmentos dentro de cada ruta.
 * @return true si se encontró y aplicó una mejora, false en caso contrario.
 */
bool explorarVecindad2Opt(std::vector<std::vector<int>>& rutas, CVRP* problema) {
    Move mejor_movimiento;
    mejor_movimiento.delta_cost = 0;
    int** cost_matrix = problema->getCost_Matrix();

    for (size_t r = 0; r < rutas.size(); ++r) {
        if (rutas[r].size() < 2) continue;
        for (size_t i = 0; i < rutas[r].size() - 1; ++i) {
            for (size_t j = i + 1; j < rutas[r].size(); ++j) {
                int u1 = rutas[r][i];
                int v1 = (i > 0) ? rutas[r][i - 1] : 0; // 0 es el depot
                int u2 = rutas[r][j];
                int v2 = (j < rutas[r].size() - 1) ? rutas[r][j + 1] : 0;

                double costo_actual = cost_matrix[v1][u1] + cost_matrix[u2][v2];
                double costo_nuevo = cost_matrix[v1][u2] + cost_matrix[u1][v2];

                if (costo_nuevo - costo_actual < mejor_movimiento.delta_cost) {
                    mejor_movimiento.delta_cost = costo_nuevo - costo_actual;
                    mejor_movimiento.type = 2;
                    mejor_movimiento.pos1_route = r;
                    mejor_movimiento.pos1_idx = i;
                    mejor_movimiento.pos2_idx = j;
                }
            }
        }
    }

    if (mejor_movimiento.delta_cost < -1e-9) {
        int r = mejor_movimiento.pos1_route;
        int i = mejor_movimiento.pos1_idx;
        int j = mejor_movimiento.pos2_idx;
        std::reverse(rutas[r].begin() + i, rutas[r].begin() + j + 1);
        return true;
    }
    return false;
}


/**
 * @brief Búsqueda Local principal usando una estructura VND (Variable Neighborhood Descent).
 * Aplica secuencialmente diferentes tipos de movimientos hasta que no se pueda encontrar más mejora.
 */
void busquedaLocalVND(Solution& sol, CVRP* problema) {
    auto rutas = decodificarRutas(sol, problema);
    int* demands = problema->getCustomerDemand();

    std::vector<double> demandas_rutas(rutas.size(), 0.0);
    for (size_t i = 0; i < rutas.size(); ++i) {
        for (int nodo : rutas[i]) {
            demandas_rutas[i] += demands[nodo];
        }
    }

    bool mejora_encontrada = true;
    while (mejora_encontrada) {
        mejora_encontrada = false;

        if (explorarVecindadSwap(rutas, problema, demandas_rutas)) {
            mejora_encontrada = true;
            continue; // Si mejora, vuelve a la primera vecindad (estrategia first-improvement entre vecindades)
        }
        if (explorarVecindad2Opt(rutas, problema)) {
            mejora_encontrada = true;
            continue;
        }
        // Aquí podrías añadir más vecindades, como Relocate.
    }

    // Reconstruir la solución final, evaluar y actualizar
    reconstruirSolucionDesdeRutas(sol, rutas, {}); // No se necesita indicesMutables aquí
    problema->evaluate(&sol);
    problema->evaluateConstraints(&sol);
}

/**
 * @brief Perturba la solución para escapar de óptimos locales.
 * Realiza varios intercambios de nodos entre rutas aleatorias.
 */
void perturbarSolucion(std::vector<std::vector<int>>& rutas, int num_clientes_total) {
    if (rutas.empty() || num_clientes_total < 4) return;
    RandomNumber* rnd = RandomNumber::getInstance();

    // La "fuerza" de la perturbación es proporcional al tamaño del problema
    int num_perturbaciones = std::max(2, static_cast<int>(num_clientes_total * 0.15));

    for (int k = 0; k < num_perturbaciones; ++k) {
        int r1 = rnd->nextInt(rutas.size() - 1);
        int r2 = rnd->nextInt(rutas.size() - 1);

        if (rutas[r1].empty() || rutas[r2].empty()) continue;

        int idx1 = rnd->nextInt(rutas[r1].size() - 1);
        int idx2 = rnd->nextInt(rutas[r2].size() - 1);

        std::swap(rutas[r1][idx1], rutas[r2][idx2]);
    }
}


// --- Clase Principal Modificada ---

LocalSearch::LocalSearch() {}

void LocalSearch::initialize(Requirements* config) {
    config->addValue("#Iteraciones-sin-mejora", Constantes::INT);
    this->param = *config->load();
}

/**
 * @brief TAREA 3: Método principal que ejecuta la Búsqueda Local Iterada (ILS).
 * Incorpora un criterio de parada adaptativo.
 */
void LocalSearch::execute(Solution y) {
    const int MAX_ITER_SIN_MEJORA = this->param.get("#Iteraciones-sin-mejora").getInt();
    CVRP* problema = dynamic_cast<CVRP*>(y.getProblem());
    const bool maximization = y.getProblem()->getObjectivesType()[0] == Constantes::MAXIMIZATION;

    // 1. Búsqueda Local Inicial para encontrar el primer óptimo local
    busquedaLocalVND(y, problema);

    // Guardamos la mejor solución encontrada hasta ahora
    Solution mejor_solucion = y;

    // --- Bucle principal de ILS con Criterio de Parada Adaptativo ---
    int iteraciones_sin_mejora = 0;
    while (iteraciones_sin_mejora < MAX_ITER_SIN_MEJORA) {

        // Copiamos la mejor solución actual para perturbarla
        Solution candidata = mejor_solucion;
        auto rutas_candidatas = decodificarRutas(candidata, problema);

        // 2. Perturbación (el "kick" para salir del óptimo local)
        perturbarSolucion(rutas_candidatas, problema->getNumberCustomers());
        reconstruirSolucionDesdeRutas(candidata, rutas_candidatas, {});

        // 3. Búsqueda Local sobre la solución perturbada
        busquedaLocalVND(candidata, problema);

        // 4. Criterio de Aceptación: se acepta si es estrictamente mejor
        bool es_mejor = false;
        if (candidata.getNumberOfViolatedConstraints() == 0) {
            Interval objetivo_candidato = candidata.getObjective(0);
            Interval objetivo_mejor = mejor_solucion.getObjective(0);

            es_mejor = (!maximization && objetivo_candidato < objetivo_mejor);
        }

        if (es_mejor) {
            mejor_solucion = candidata; // Aceptar la nueva mejor solución
            iteraciones_sin_mejora = 0; // Reiniciar contador
        }
        else {
            iteraciones_sin_mejora++; // Incrementar contador
        }
    }

    // Al final, nos aseguramos de que el objeto 'y' que se recibió
    // (posiblemente una copia) contenga la mejor solución encontrada.
    // NOTA: Dependiendo de cómo se llame a execute(), este cambio podría
    // no persistir fuera de la función si 'y' se pasa por valor.
    y = mejor_solucion;
}