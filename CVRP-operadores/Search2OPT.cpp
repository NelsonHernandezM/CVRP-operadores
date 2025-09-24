#include "Search2OPT.h"
#include "problems/CVRP.h"
#include <vector>
#include <algorithm> // Para std::reverse

// --- FUNCIONES AUXILIARES (Decodificación y Reconstrucción) ---
// Estas funciones son esenciales para traducir entre la representación de la solución y las rutas.

/**
 * @brief Convierte la representación plana de la solución (un array de enteros)
 * en una estructura más manejable de vector de vectores, donde cada vector interno es una ruta.
 */
void decodeRoutes(Solution& sol, std::vector<std::vector<int>>& rutas) {
    rutas.clear();
    std::vector<int> ruta_actual;
    for (int i = 0; i < sol.getNumVariables(); ++i) {
        int value = sol.getVariableValue(i).L;
        if (value == -1) break; // Fin de la solución
        if (value == 0) { // El 0 (depósito) separa las rutas
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
}

/**
 * @brief Realiza el proceso inverso a decodificarRutas. Toma el vector de vectores
 * de rutas y lo vuelve a codificar en el array plano de la solución.
 */
void reSolfromRoutes(Solution& sol, const std::vector<std::vector<int>>& rutas) {
    std::vector<int> valores_planos;
    for (size_t i = 0; i < rutas.size(); ++i) {
        if (rutas[i].empty()) continue;
        valores_planos.insert(valores_planos.end(), rutas[i].begin(), rutas[i].end());
        // Añade un 0 como separador si no es la última ruta
        if (i < rutas.size() - 1) {
            valores_planos.push_back(0);
        }
    }
    // Rellena el resto de la solución con -1
    for (size_t i = valores_planos.size(); i < sol.getNumVariables(); ++i) {
        valores_planos.push_back(-1);
    }
    // Actualiza el objeto Solution
    for (size_t i = 0; i < sol.getNumVariables(); ++i) {
        sol.setVariableValue(i, valores_planos[i]);
    }
}

// --- CLASE PRINCIPAL: LocalSearch simplificada a solo 2-Opt ---


Search2OPT::Search2OPT() {}

void Search2OPT::initialize(Requirements* config) {}

void Search2OPT::execute(Solution y) {

	//cout << " 1 "<<y.getObjective(0) << endl;


    CVRP* problema = dynamic_cast<CVRP*>(y.getProblem());
    if (!problema) return;

    int** cost_matrix = problema->getCost_Matrix();

    std::vector<std::vector<int>> rutas;
    decodeRoutes(y, rutas);

    bool mejora_global_encontrada = true;
    while (mejora_global_encontrada) {
        mejora_global_encontrada = false;

        double mejor_delta = 0;
        int mejor_ruta_idx = -1, mejor_i = -1, mejor_j = -1;

        for (size_t r = 0; r < rutas.size(); ++r) {
            if (rutas[r].size() < 2) continue; // Un 2-Opt real necesita al menos 2 aristas (3 nodos + depósito)

            std::vector<int> ruta_con_deposito = { 0 };
            ruta_con_deposito.insert(ruta_con_deposito.end(), rutas[r].begin(), rutas[r].end());
            ruta_con_deposito.push_back(0);

            // Se necesitan al menos 4 aristas en la ruta con depósito para un 2-opt válido no adyacente
            if (ruta_con_deposito.size() < 5) continue;

            for (size_t i = 0; i < ruta_con_deposito.size() - 3; ++i) {
                for (size_t j = i + 2; j < ruta_con_deposito.size() - 1; ++j) {
                    int A = ruta_con_deposito[i];
                    int B = ruta_con_deposito[i + 1];
                    int C = ruta_con_deposito[j];
                    int D = ruta_con_deposito[j + 1];

                    double costo_actual = cost_matrix[A][B] + cost_matrix[C][D];
                    double costo_nuevo = cost_matrix[A][C] + cost_matrix[B][D];
                    double delta = costo_nuevo - costo_actual;

                    if (delta < mejor_delta) {
                        mejor_delta = delta;
                        mejor_ruta_idx = r;
                        // Guardamos los índices 'i' y 'j' tal cual los da el bucle
                        mejor_i = i;
                        mejor_j = j;
                    }
                }
            }
        }

        if (mejor_delta < -1e-9) {
            // --- CORRECCIÓN ---
            // Traducimos los índices 'i' y 'j' de 'ruta_con_deposito' a la 'rutas[r]' original.
            // El segmento a invertir en 'rutas[r]' empieza en el índice 'mejor_i' y termina en 'mejor_j - 1'.
            // std::reverse(first, last) no incluye 'last', por lo que el segundo iterador
            // debe apuntar a la posición (mejor_j - 1) + 1, que es simplemente 'mejor_j'.
            std::reverse(rutas[mejor_ruta_idx].begin() + mejor_i, rutas[mejor_ruta_idx].begin() + mejor_j);

            mejora_global_encontrada = true;
        }
    }

    reSolfromRoutes(y, rutas);
    problema->evaluate(&y);
    problema->evaluateConstraints(&y);
    //cout << " 2 " << y.getObjective(0) << endl;

}