//#include "LocalSearch.h"
//#include "problems/CVRP.h"
//#include "tools/RandomNumber.h"
//#include "tools/operators/interval/MOSADOperators/CombinationMutation.h"
//#include "solutions/SolutionSet.h"
//#include "problems/Problem.h"
//#include "tools/Requirements.h"
//#include <vector>
//#include <cmath>
//#include <utility>
//#include <numeric>
//#include <algorithm>
//#include <iostream>
//#include <chrono>
//#include <random> // Necesario para std::mt19937 y std::random_device
//
//// --- ESTRUCTURAS Y ENUMS ---
//enum class EstrategiaMejora { FIRST = 0, BEST = 1 };
//enum class EstrategiaRuta { INTRA = 0, INTER = 1, AMBAS = 2 };
//
//// Estructura para registrar un movimiento potencial y su impacto en el costo.
//struct Move {
//	double delta_cost = 0.0;
//	int type = -1;
//	size_t pos1_route = -1, pos1_idx = -1;
//	size_t pos2_route = -1, pos2_idx = -1;
//};
//
//// Estructura para tener acceso O(1) a la ubicación de cualquier cliente.
//struct NodeLocation {
//	size_t route_idx;
//	size_t pos_idx;
//};
//
//
//void imprimirLS(Solution s) {
//	Interval* vars = s.getDecisionVariables();
//	for (int i = 0; i < s.getNumVariables(); i++) {
//		std::cout << vars[i].L << " ";
//	}
//	std::cout << std::endl;
//}
//
//
//
//std::vector<std::vector<int>> decodificarRutas(Solution& sol, CVRP* problema) {
//	std::vector<std::vector<int>> rutas;
//	std::vector<int> ruta_actual;
//	ruta_actual.reserve(problema->getNumberCustomers());
//	for (int i = 0; i < sol.getNumVariables(); ++i) {
//		int value = sol.getVariableValue(i).L;
//		if (value == -1) break;
//		if (problema->isDepot(value)) {
//			if (!ruta_actual.empty()) {
//				rutas.push_back(ruta_actual);
//				ruta_actual.clear();
//			}
//		}
//		else {
//			ruta_actual.push_back(value);
//		}
//	}
//	if (!ruta_actual.empty()) {
//		rutas.push_back(ruta_actual);
//	}
//	return rutas;
//}
//
///**
// * @brief Actualiza una solución a partir de una estructura de rutas modificada.
// * @param sol La solución a actualizar (se modifica por referencia).
// * @param rutas La estructura de rutas con la nueva configuración.
// */
//void reconstruirSolucionDesdeRutas(Solution& sol, const std::vector<std::vector<int>>& rutas) {
//	std::vector<int> valores_planos;
//	valores_planos.reserve(sol.getNumVariables());
//	for (size_t i = 0; i < rutas.size(); ++i) {
//		if (rutas[i].empty()) continue;
//		valores_planos.insert(valores_planos.end(), rutas[i].begin(), rutas[i].end());
//		// Añade un separador de depósito si no es la última ruta con clientes.
//		if (i < rutas.size() - 1) {
//			bool more_routes = false;
//			for (size_t j = i + 1; j < rutas.size(); ++j) {
//				if (!rutas[j].empty()) {
//					more_routes = true;
//					break;
//				}
//			}
//			if (more_routes) valores_planos.push_back(0);
//		}
//	}
//	// Rellena el resto de la solución con -1.
//	size_t current_size = valores_planos.size();
//	for (size_t i = 0; i < sol.getNumVariables() - current_size; ++i) {
//		valores_planos.push_back(-1);
//	}
//	for (size_t i = 0; i < sol.getNumVariables(); ++i) {
//		sol.setVariableValue(i, valores_planos[i]);
//	}
//}
//
//// --- Generación de Lista de Candidatos ---
///**
// * @brief Genera una lista de los K clientes más cercanos para cada cliente.
// * @param num_clientes Número total de clientes.
// * @param K Tamaño de la lista de candidatos.
// * @param cost_matrix Matriz de costos para calcular distancias.
// * @return Una lista de adyacencia donde cada índice i contiene los K vecinos más cercanos del cliente i.
// */
//std::vector<std::vector<int>> generarListaCandidatos(int num_clientes, int K, int** cost_matrix) {
//	std::vector<std::vector<int>> listaCandidatos(num_clientes + 1);
//	if (K >= num_clientes) K = num_clientes - 1;
//
//	for (int i = 1; i <= num_clientes; ++i) {
//		std::vector<std::pair<int, int>> distancias;
//		distancias.reserve(num_clientes);
//		for (int j = 1; j <= num_clientes; ++j) {
//			if (i == j) continue;
//			distancias.push_back({ cost_matrix[i][j], j });
//		}
//		std::sort(distancias.begin(), distancias.end());
//		listaCandidatos[i].reserve(K);
//		for (int k = 0; k < K; ++k) {
//			listaCandidatos[i].push_back(distancias[k].second);
//		}
//	}
//	return listaCandidatos;
//}
//
//// --- BÚSQUEDA DE VECINDAD (OPERADORES) ---
//
///**
// * @brief Operador Swap (Intra-ruta): Intercambia dos clientes dentro de la misma ruta.
// */
//bool explorarVecindadSwapIntra(std::vector<std::vector<int>>& rutas, CVRP* problema, int bestImprovement, std::vector<NodeLocation>& nodeLocations) {
//	Move mejor_movimiento;
//	mejor_movimiento.delta_cost = 0; // Inicialización crucial
//	int** cost_matrix = problema->getCost_Matrix();
//
//	for (size_t r = 0; r < rutas.size(); ++r) {
//		if (rutas[r].size() < 2) continue;
//		for (size_t i = 0; i < rutas[r].size(); ++i) {
//			const int u = rutas[r][i];
//			const int pred_u = (i > 0) ? rutas[r][i - 1] : 0;
//			const int succ_u = (i < rutas[r].size() - 1) ? rutas[r][i + 1] : 0;
//			const double costo_removido_u = cost_matrix[pred_u][u] + cost_matrix[u][succ_u];
//
//			for (size_t j = i + 1; j < rutas[r].size(); ++j) {
//				const int v = rutas[r][j];
//				const int pred_v = rutas[r][j - 1];
//				const int succ_v = (j < rutas[r].size() - 1) ? rutas[r][j + 1] : 0;
//				double costo_actual = costo_removido_u + cost_matrix[pred_v][v] + cost_matrix[v][succ_v];
//				double costo_nuevo;
//
//				if (i + 1 == j) { // Caso adyacente
//					costo_actual -= cost_matrix[u][v];
//					costo_nuevo = cost_matrix[pred_u][v] + cost_matrix[v][u] + cost_matrix[u][succ_v];
//				}
//				else { // Caso no adyacente
//					costo_nuevo = cost_matrix[pred_u][v] + cost_matrix[v][succ_u] + cost_matrix[pred_v][u] + cost_matrix[u][succ_v];
//				}
//
//				const double delta = costo_nuevo - costo_actual;
//				if (delta < -1e-9) {
//					if (bestImprovement == 0) { // First Improvement
//						std::swap(rutas[r][i], rutas[r][j]);
//						std::swap(nodeLocations[u].pos_idx, nodeLocations[v].pos_idx);
//						return true;
//					}
//					if (delta < mejor_movimiento.delta_cost) { // Best Improvement
//						mejor_movimiento = { delta, 1, r, i, r, j };
//					}
//				}
//			}
//		}
//	}
//
//	if (bestImprovement == 1 && mejor_movimiento.delta_cost < -1e-9) {
//		size_t r = mejor_movimiento.pos1_route;
//		size_t i = mejor_movimiento.pos1_idx;
//		size_t j = mejor_movimiento.pos2_idx;
//		const int u = rutas[r][i];
//		const int v = rutas[r][j];
//
//		std::swap(rutas[r][i], rutas[r][j]);
//		std::swap(nodeLocations[u].pos_idx, nodeLocations[v].pos_idx);
//		return true;
//	}
//	return false;
//}
//
///**
// * @brief Operador 2-Opt (Intra-ruta): Invierte un segmento de una ruta.
// */
//bool explorarVecindad2OptIntra(std::vector<std::vector<int>>& rutas, CVRP* problema, int bestImprovement, std::vector<NodeLocation>& nodeLocations) {
//	Move mejor_movimiento;
//	mejor_movimiento.delta_cost = 0; // Inicialización crucial
//	int** cost_matrix = problema->getCost_Matrix();
//
//	for (size_t r = 0; r < rutas.size(); ++r) {
//		const size_t m = rutas[r].size();
//		if (m < 2) continue;
//		for (size_t i = 0; i < m - 1; ++i) {
//			const int u1 = rutas[r][i];
//			const int v1_pred = (i > 0) ? rutas[r][i - 1] : 0;
//
//			for (size_t j = i + 1; j < m; ++j) {
//				const int u2 = rutas[r][j];
//				const int v2_succ = (j < m - 1) ? rutas[r][j + 1] : 0;
//				const double costo_removido = cost_matrix[v1_pred][u1] + cost_matrix[u2][v2_succ];
//				const double costo_agregado = cost_matrix[v1_pred][u2] + cost_matrix[u1][v2_succ];
//				const double delta = costo_agregado - costo_removido;
//
//				if (delta < -1e-9) {
//					if (bestImprovement == 0) { // First Improvement
//						std::reverse(rutas[r].begin() + i, rutas[r].begin() + j + 1);
//						for (size_t k = i; k <= j; ++k) nodeLocations[rutas[r][k]].pos_idx = k;
//						return true;
//					}
//					if (delta < mejor_movimiento.delta_cost) { // Best Improvement
//						mejor_movimiento = { delta, 2, r, i, (size_t)-1, j };
//					}
//				}
//			}
//		}
//	}
//
//	if (bestImprovement == 1 && mejor_movimiento.delta_cost < -1e-9) {
//		size_t r = mejor_movimiento.pos1_route;
//		size_t i = mejor_movimiento.pos1_idx;
//		size_t j = mejor_movimiento.pos2_idx;
//		std::reverse(rutas[r].begin() + i, rutas[r].begin() + j + 1);
//		for (size_t k = i; k <= j; ++k) nodeLocations[rutas[r][k]].pos_idx = k;
//		return true;
//	}
//	return false;
//}
//
///**
// * @brief Operador Swap (Inter-ruta): Intercambia dos clientes entre dos rutas diferentes.
// */
//bool explorarVecindadSwapInter(std::vector<std::vector<int>>& rutas, CVRP* problema, std::vector<double>& demandas_rutas, int bestImprovement, const std::vector<std::vector<int>>& listaCandidatos, std::vector<NodeLocation>& nodeLocations) {
//	Move mejor_movimiento;
//	mejor_movimiento.delta_cost = 0; // Inicialización crucial
//	int** cost_matrix = problema->getCost_Matrix();
//	int* demands = problema->getCustomerDemand();
//	const int capacity = problema->getMaxCapacity();
//
//	for (size_t r1 = 0; r1 < rutas.size(); ++r1) {
//		for (size_t i = 0; i < rutas[r1].size(); ++i) {
//			const int u = rutas[r1][i];
//
//			for (int v : listaCandidatos[u]) {
//				const NodeLocation& v_loc = nodeLocations[v];
//				const size_t r2 = v_loc.route_idx;
//				if (r1 >= r2) continue; // Evita pares duplicados y swaps en la misma ruta
//				const size_t j = v_loc.pos_idx;
//
//				// Salvaguarda: verifica consistencia antes de usar los datos del mapa.
//				if (r2 >= rutas.size() || j >= rutas[r2].size() || rutas[r2][j] != v) continue;
//
//				const int demanda_u = demands[u], demanda_v = demands[v];
//				if (demandas_rutas[r1] - demanda_u + demanda_v > capacity || demandas_rutas[r2] - demanda_v + demanda_u > capacity) continue;
//
//				const int pred_u = (i > 0) ? rutas[r1][i - 1] : 0;
//				const int succ_u = (i < rutas[r1].size() - 1) ? rutas[r1][i + 1] : 0;
//				const int pred_v = (j > 0) ? rutas[r2][j - 1] : 0;
//				const int succ_v = (j < rutas[r2].size() - 1) ? rutas[r2][j + 1] : 0;
//
//				const double costo_actual = cost_matrix[pred_u][u] + cost_matrix[u][succ_u] + cost_matrix[pred_v][v] + cost_matrix[v][succ_v];
//				const double costo_nuevo = cost_matrix[pred_u][v] + cost_matrix[v][succ_u] + cost_matrix[pred_v][u] + cost_matrix[u][succ_v];
//				const double delta = costo_nuevo - costo_actual;
//
//				if (delta < -1e-9) {
//					if (bestImprovement == 0) {
//						demandas_rutas[r1] += demanda_v - demanda_u;
//						demandas_rutas[r2] += demanda_u - demanda_v;
//						std::swap(rutas[r1][i], rutas[r2][j]);
//						std::swap(nodeLocations[u], nodeLocations[v]);
//						return true;
//					}
//					if (delta < mejor_movimiento.delta_cost) {
//						mejor_movimiento = { delta, 1, r1, i, r2, j };
//					}
//				}
//			}
//		}
//	}
//
//	if (bestImprovement == 1 && mejor_movimiento.delta_cost < -1e-9) {
//		const size_t r1 = mejor_movimiento.pos1_route, i = mejor_movimiento.pos1_idx;
//		const size_t r2 = mejor_movimiento.pos2_route, j = mejor_movimiento.pos2_idx;
//		const int u = rutas[r1][i], v = rutas[r2][j];
//		demandas_rutas[r1] += demands[v] - demands[u];
//		demandas_rutas[r2] += demands[u] - demands[v];
//		std::swap(rutas[r1][i], rutas[r2][j]);
//		std::swap(nodeLocations[u], nodeLocations[v]);
//		return true;
//	}
//	return false;
//}
//
///**
// * @brief Operador 2-Opt* (Inter-ruta): Intercambia las colas de dos rutas.
// */
//bool explorarVecindad2OptInter(std::vector<std::vector<int>>& rutas, CVRP* problema, std::vector<double>& demandas_rutas, int bestImprovement, const std::vector<std::vector<int>>& listaCandidatos, std::vector<NodeLocation>& nodeLocations) {
//	Move mejor_movimiento;
//	mejor_movimiento.delta_cost = 0; // Inicialización crucial
//	int** cost_matrix = problema->getCost_Matrix();
//	int* demands = problema->getCustomerDemand();
//	const int capacity = problema->getMaxCapacity();
//
//	std::vector<std::vector<double>> suffixDemand(rutas.size());
//	for (size_t r = 0; r < rutas.size(); ++r) {
//		suffixDemand[r].resize(rutas[r].size() + 1, 0.0);
//		for (int k = (int)rutas[r].size() - 1; k >= 0; --k) {
//			suffixDemand[r][k] = demands[rutas[r][k]] + suffixDemand[r][k + 1];
//		}
//	}
//
//	for (size_t r1 = 0; r1 < rutas.size(); ++r1) {
//		if (rutas[r1].empty()) continue;
//		for (size_t i = 0; i < rutas[r1].size(); ++i) {
//			const int u1 = rutas[r1][i];
//			for (int u2 : listaCandidatos[u1]) {
//				const NodeLocation& u2_loc = nodeLocations[u2];
//				const size_t r2 = u2_loc.route_idx;
//				if (r1 >= r2) continue;
//				const size_t j = u2_loc.pos_idx;
//
//				// Salvaguarda: verifica consistencia antes de usar los datos del mapa.
//				if (r2 >= rutas.size() || j >= rutas[r2].size() || rutas[r2][j] != u2) continue;
//
//				double demanda_cola1 = suffixDemand[r1][i + 1];
//				double demanda_cola2 = suffixDemand[r2][j + 1];
//
//				if (demandas_rutas[r1] - demanda_cola1 + demanda_cola2 > capacity ||
//					demandas_rutas[r2] - demanda_cola2 + demanda_cola1 > capacity) {
//					continue;
//				}
//
//				const int v1 = (i < rutas[r1].size() - 1) ? rutas[r1][i + 1] : 0;
//				const int v2 = (j < rutas[r2].size() - 1) ? rutas[r2][j + 1] : 0;
//				const double delta = (cost_matrix[u1][v2] + cost_matrix[u2][v1]) - (cost_matrix[u1][v1] + cost_matrix[u2][v2]);
//
//				if (delta < -1e-9) {
//					if (bestImprovement == 0) { // First Improvement
//						demandas_rutas[r1] += demanda_cola2 - demanda_cola1;
//						demandas_rutas[r2] += demanda_cola1 - demanda_cola2;
//
//						std::vector<int> temp_cola(rutas[r1].begin() + i + 1, rutas[r1].end());
//						rutas[r1].erase(rutas[r1].begin() + i + 1, rutas[r1].end());
//						rutas[r1].insert(rutas[r1].end(), rutas[r2].begin() + j + 1, rutas[r2].end());
//						rutas[r2].erase(rutas[r2].begin() + j + 1, rutas[r2].end());
//						rutas[r2].insert(rutas[r2].end(), temp_cola.begin(), temp_cola.end());
//
//						for (size_t p = i + 1; p < rutas[r1].size(); ++p) nodeLocations[rutas[r1][p]] = { r1, p };
//						for (size_t p = j + 1; p < rutas[r2].size(); ++p) nodeLocations[rutas[r2][p]] = { r2, p };
//
//						return true;
//					}
//					if (delta < mejor_movimiento.delta_cost) { // Best Improvement
//						mejor_movimiento = { delta, 3, r1, i, r2, j };
//					}
//				}
//			}
//		}
//	}
//
//	if (bestImprovement == 1 && mejor_movimiento.delta_cost < -1e-9) { // Best Improvement
//		const size_t r1 = mejor_movimiento.pos1_route, i = mejor_movimiento.pos1_idx;
//		const size_t r2 = mejor_movimiento.pos2_route, j = mejor_movimiento.pos2_idx;
//
//		double demanda_cola1 = suffixDemand[r1][i + 1];
//		double demanda_cola2 = suffixDemand[r2][j + 1];
//		demandas_rutas[r1] += demanda_cola2 - demanda_cola1;
//		demandas_rutas[r2] += demanda_cola1 - demanda_cola2;
//
//		std::vector<int> temp_cola(rutas[r1].begin() + i + 1, rutas[r1].end());
//		rutas[r1].erase(rutas[r1].begin() + i + 1, rutas[r1].end());
//		rutas[r1].insert(rutas[r1].end(), rutas[r2].begin() + j + 1, rutas[r2].end());
//		rutas[r2].erase(rutas[r2].begin() + j + 1, rutas[r2].end());
//		rutas[r2].insert(rutas[r2].end(), temp_cola.begin(), temp_cola.end());
//
//		for (size_t p = i + 1; p < rutas[r1].size(); ++p) nodeLocations[rutas[r1][p]] = { r1, p };
//		for (size_t p = j + 1; p < rutas[r2].size(); ++p) nodeLocations[rutas[r2][p]] = { r2, p };
//
//		return true;
//	}
//	return false;
//}
//
//// --- ORQUESTADOR VND ---
///**
// * @brief Variable Neighborhood Descent (VND).
// * Aplica secuencialmente los operadores de búsqueda local hasta que no se encuentran más mejoras.
// */
//void busquedaLocalVND(std::vector<std::vector<int>>& rutas, Solution& sol, CVRP* problema, int bestImprovement, EstrategiaRuta est_ruta, const std::vector<std::vector<int>>& listaCandidatos, std::vector<NodeLocation>& nodeLocations) {
//	if (rutas.empty()) return;
//
//	std::vector<double> demandas_rutas(rutas.size());
//	for (size_t i = 0; i < rutas.size(); ++i) {
//		demandas_rutas[i] = 0;
//		for (int nodo : rutas[i]) demandas_rutas[i] += problema->getCustomerDemand()[nodo];
//	}
//
//	bool mejora_encontrada = true;
//	while (mejora_encontrada) {
//		mejora_encontrada = false;
//		if (est_ruta == EstrategiaRuta::INTRA || est_ruta == EstrategiaRuta::AMBAS) {
//			if (explorarVecindadSwapIntra(rutas, problema, bestImprovement, nodeLocations)) { mejora_encontrada = true; continue; }
//			if (explorarVecindad2OptIntra(rutas, problema, bestImprovement, nodeLocations)) { mejora_encontrada = true; continue; }
//		}
//		if (est_ruta == EstrategiaRuta::INTER || est_ruta == EstrategiaRuta::AMBAS) {
//			if (explorarVecindadSwapInter(rutas, problema, demandas_rutas, bestImprovement, listaCandidatos, nodeLocations)) { mejora_encontrada = true; continue; }
//			if (explorarVecindad2OptInter(rutas, problema, demandas_rutas, bestImprovement, listaCandidatos, nodeLocations)) { mejora_encontrada = true; continue; }
//		}
//	}
//
//	reconstruirSolucionDesdeRutas(sol, rutas);
//	problema->evaluate(&sol);
//	problema->evaluateConstraints(&sol);
//}
//
//// --- FUNCIONES DE PERTURBACIÓN ---
//
///**
// * @brief Perturbación mediante swaps aleatorios entre rutas.
// */
//void perturbarSolucionSwap(std::vector<std::vector<int>>& rutas, std::vector<NodeLocation>& nodeLocations, int num_clientes_total, CVRP* problema) {
//	if (rutas.empty() || num_clientes_total < 4) return;
//	RandomNumber* rnd = RandomNumber::getInstance();
//	int num_perturbaciones = std::min(50, static_cast<int>(num_clientes_total * 0.15));
//
//	for (int k = 0; k < num_perturbaciones; ++k) {
//		if (rutas.size() < 2) continue;
//		int r1 = rnd->nextInt(rutas.size() - 1);
//		int r2 = rnd->nextInt(rutas.size() - 1);
//		if (r1 == r2 || rutas[r1].empty() || rutas[r2].empty()) continue;
//		int idx1 = rnd->nextInt(rutas[r1].size() - 1);
//		int idx2 = rnd->nextInt(rutas[r2].size() - 1);
//
//		const int u = rutas[r1][idx1];
//		const int v = rutas[r2][idx2];
//
//		std::swap(rutas[r1][idx1], rutas[r2][idx2]);
//		std::swap(nodeLocations[u], nodeLocations[v]);
//	}
//}
//
///**
// * @brief Perturbación de tipo Scramble (barajado de un subsegmento de todos los clientes).
// */
//void perturbarSolucionScramble(std::vector<std::vector<int>>& rutas, std::vector<NodeLocation>& nodeLocations, int num_clientes_total, CVRP* problema) {
//	if (rutas.empty() || num_clientes_total < 4) return;
//
//	std::vector<int> todos_los_clientes;
//	todos_los_clientes.reserve(num_clientes_total);
//	std::vector<size_t> tamanos_rutas;
//	tamanos_rutas.reserve(rutas.size());
//
//	for (const auto& ruta : rutas) {
//		if (!ruta.empty()) {
//			todos_los_clientes.insert(todos_los_clientes.end(), ruta.begin(), ruta.end());
//			tamanos_rutas.push_back(ruta.size());
//		}
//	}
//
//	if (todos_los_clientes.size() < 2) return;
//
//	RandomNumber* rnd = RandomNumber::getInstance();
//	int pos1, pos2;
//	do {
//		pos1 = rnd->nextInt(todos_los_clientes.size() - 1);
//		pos2 = rnd->nextInt(todos_los_clientes.size() - 1);
//	} while (pos1 == pos2);
//
//	if (pos1 > pos2) std::swap(pos1, pos2);
//
//	// Baraja el subsegmento usando el motor de números aleatorios estándar de C++.
//	std::shuffle(todos_los_clientes.begin() + pos1, todos_los_clientes.begin() + pos2 + 1, std::mt19937{ std::random_device{}() });
//
//	auto cliente_iterator = todos_los_clientes.begin();
//	size_t ruta_idx_no_vacia = 0;
//	for (size_t i = 0; i < rutas.size(); ++i) {
//		if (!rutas[i].empty()) {
//			rutas[i].assign(cliente_iterator, cliente_iterator + tamanos_rutas[ruta_idx_no_vacia]);
//			cliente_iterator += tamanos_rutas[ruta_idx_no_vacia];
//			ruta_idx_no_vacia++;
//		}
//	}
//
//	// Resincronización completa obligatoria después de una modificación tan drástica.
//	for (size_t r = 0; r < rutas.size(); ++r) {
//		for (size_t p = 0; p < rutas[r].size(); ++p) {
//			nodeLocations[rutas[r][p]] = { r, p };
//		}
//	}
//}
//
// 
//void perturbarSolucionCombination(
//	Solution& sol,                              // <-- Recibe por referencia para modificar la original
//	std::vector<std::vector<int>>& rutas,       // <-- Recibe por referencia para actualizarla
//	std::vector<NodeLocation>& nodeLocations,   // <-- Recibe por referencia para actualizarla
//	CVRP* problema)                             // <-- Necesario para decodificar
//{
//	//cout << endl;
//	//cout << endl;
//	// 1. Ejecuta la mutación sobre el objeto Solution
//	CombinationMutation mutator;
//	SolutionSet* seto= new SolutionSet(1,1,problema);
//	//cout << "ANTES DE LA MUTACION: ";
//	//imprimirLS(sol);
//	//cout << "------------------------------";
//
//	seto->set(0, sol);
//
//	mutator.execute(*seto, sol,1.0); // 'sol' es modificada aquí
//
//	//cout << "despues MUTACION: ";
//	//imprimirLS(sol);
//	//cout << "*******************************";
//
//	//cout << endl; cout << endl;
//	// 2. Resincroniza las estructuras de datos locales
//	// 'rutas' ahora está obsoleta, la reconstruimos desde la 'sol' modificada.
//	rutas = decodificarRutas(sol, problema);
//
//	// 'nodeLocations' también está obsoleta, la reconstruimos desde las nuevas 'rutas'.
//	nodeLocations.assign(problema->getNumberCustomers() + 1, {}); // Limpia el mapa
//	for (size_t r = 0; r < rutas.size(); ++r) {
//		for (size_t p = 0; p < rutas[r].size(); ++p) {
//			nodeLocations[rutas[r][p]] = { r, p };
//		}
//	}
//	delete seto;
//}
//
//
//// --- CLASE PRINCIPAL ---
//LocalSearch::LocalSearch() {}
//
//void LocalSearch::initialize(Requirements* config) {
//	config->addValue("#Iteraciones-sin-mejora", Constantes::INT);
//	config->addValue("#BestImprovement", Constantes::INT);
//	config->addValue("#EstrategiaRuta", Constantes::INT);
//	this->param = *config->load();
//}
//
///**
// * @brief Ejecuta el algoritmo de Búsqueda Local Iterada (ILS).
// */
//void LocalSearch::execute(Solution y) {
//	const int MAX_ITER_SIN_MEJORA = this->param.get("#Iteraciones-sin-mejora").getInt();
//	const int BEST_IMPROVEMENT = this->param.get("#BestImprovement").getInt();
//	const auto ESTRATEGIA_RUTA = static_cast<EstrategiaRuta>(this->param.get("#EstrategiaRuta").getInt());
//
//	CVRP* problema = dynamic_cast<CVRP*>(y.getProblem());
//	const bool maximization = y.getProblem()->getObjectivesType()[0] == Constantes::MAXIMIZATION;
//	const int K = 15;
//	std::vector<std::vector<int>> listaCandidatos_ = generarListaCandidatos(problema->getNumberCustomers(), K, problema->getCost_Matrix());
//
//	auto rutas_candidatas = decodificarRutas(y, problema);
//	std::vector<NodeLocation> nodeLocations(problema->getNumberCustomers() + 1);
//
//	// Lambda para sincronizar el mapa de localizaciones con la estructura de rutas.
//	auto sincronizar_locaciones = [&](const std::vector<std::vector<int>>& rutas) {
//		// Limpia el mapa para evitar datos residuales.
//		nodeLocations.assign(problema->getNumberCustomers() + 1, {});
//		for (size_t r = 0; r < rutas.size(); ++r) {
//			for (size_t p = 0; p < rutas[r].size(); ++p) {
//				nodeLocations[rutas[r][p]] = { r, p };
//			}
//		}
//		};
//
//	sincronizar_locaciones(rutas_candidatas);
//
//	// Búsqueda local inicial para mejorar la solución de partida.
//	busquedaLocalVND(rutas_candidatas, y, problema, BEST_IMPROVEMENT, ESTRATEGIA_RUTA, listaCandidatos_, nodeLocations);
//
//	Solution mejor_solucion = y;
//	int iteraciones_sin_mejora = 0;
//
//	// Bucle principal de Búsqueda Local Iterada (ILS).
//	while (iteraciones_sin_mejora < MAX_ITER_SIN_MEJORA) {
//		// INICIO DE ITERACIÓN: Siempre parte de un estado limpio y sincronizado desde la mejor solución conocida.
//		rutas_candidatas = decodificarRutas(mejor_solucion, problema);
//		sincronizar_locaciones(rutas_candidatas);
//
//		Solution candidata_actual = mejor_solucion;
//		RandomNumber* rnd = RandomNumber::getInstance();
//
//		// FASE DE PERTURBACIÓN: Modifica aleatoriamente el estado para escapar de óptimos locales.
//		if (rnd->nextDouble() < 0.3) {
//			perturbarSolucionCombination(candidata_actual, rutas_candidatas, nodeLocations, problema);
//		}
//		else {
//			perturbarSolucionCombination(candidata_actual, rutas_candidatas, nodeLocations, problema);
//		}
//
//		// FASE DE BÚSQUEDA LOCAL: Aplica VND a la solución perturbada para encontrar un nuevo óptimo local.
//		busquedaLocalVND(rutas_candidatas, candidata_actual, problema, BEST_IMPROVEMENT, ESTRATEGIA_RUTA, listaCandidatos_, nodeLocations);
//
//		// CRITERIO DE ACEPTACIÓN: Decide si la nueva solución reemplaza a la mejor encontrada.
//		bool es_mejor = false;
//		if (candidata_actual.getNumberOfViolatedConstraints() == 0 && mejor_solucion.getNumberOfViolatedConstraints() == 0) {
//			es_mejor = (!maximization && candidata_actual.getObjective(0).L < mejor_solucion.getObjective(0).L);
//		}
//		else if (candidata_actual.getNumberOfViolatedConstraints() < mejor_solucion.getNumberOfViolatedConstraints()) {
//			es_mejor = true;
//		}
//
//		if (es_mejor) {
//			mejor_solucion = candidata_actual;
//			iteraciones_sin_mejora = 0;
//		}
//		else {
//			iteraciones_sin_mejora++;
//		}
//	}
//
//	y = mejor_solucion; // Asigna la mejor solución global encontrada.
//}

#include "LocalSearch.h"
#include "problems/CVRP.h"
#include "tools/RandomNumber.h"
#include "tools/operators/interval/MOSADOperators/CombinationMutation.h"
#include "solutions/SolutionSet.h"
#include "problems/Problem.h"
#include "tools/Requirements.h"
#include <vector>
#include <cmath>
#include <utility>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <random>

// --- ESTRUCTURAS Y ENUMS ---
enum class EstrategiaMejora { FIRST = 0, BEST = 1 };
enum class EstrategiaRuta { INTRA = 0, INTER = 1, AMBAS = 2 };

struct Move {
	double delta_cost = 0.0;
	int type = -1;
	size_t pos1_route = -1, pos1_idx = -1;
	size_t pos2_route = -1, pos2_idx = -1;
};

struct NodeLocation {
	size_t route_idx;
	size_t pos_idx;
};


void imprimirLS(Solution s) {
	Interval* vars = s.getDecisionVariables();
	for (int i = 0; i < s.getNumVariables(); i++) {
		std::cout << vars[i].L << " ";
	}
	std::cout << std::endl;
}

// *** CAMBIO: La función "rellena" un vector pasado por referencia para evitar crear y devolver una copia. ***
void decodificarRutas(Solution& sol, CVRP* problema, std::vector<std::vector<int>>& rutas) {
	rutas.clear(); // Limpiamos el vector para reutilizarlo
	std::vector<int> ruta_actual;
	ruta_actual.reserve(problema->getNumberCustomers());
	for (int i = 0; i < sol.getNumVariables(); ++i) {
		int value = sol.getVariableValue(i).L;
		if (value == -1) break;
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
}



void reconstruirSolucionDesdeRutas(Solution& sol, const std::vector<std::vector<int>>& rutas) {
	std::vector<int> valores_planos;
	valores_planos.reserve(sol.getNumVariables());
	for (size_t i = 0; i < rutas.size(); ++i) {
		if (rutas[i].empty()) continue;
		valores_planos.insert(valores_planos.end(), rutas[i].begin(), rutas[i].end());
		if (i < rutas.size() - 1) {
			bool more_routes = false;
			for (size_t j = i + 1; j < rutas.size(); ++j) {
				if (!rutas[j].empty()) {
					more_routes = true;
					break;
				}
			}
			if (more_routes) valores_planos.push_back(0);
		}
	}
	size_t current_size = valores_planos.size();
	for (size_t i = 0; i < sol.getNumVariables() - current_size; ++i) {
		valores_planos.push_back(-1);
	}
	for (size_t i = 0; i < sol.getNumVariables(); ++i) {
		sol.setVariableValue(i, valores_planos[i]);
	}
}

// --- Generación de Lista de Candidatos ---
std::vector<std::vector<int>> generarListaCandidatos(int num_clientes, int K, int** cost_matrix) {
	std::vector<std::vector<int>> listaCandidatos(num_clientes + 1);
	if (K >= num_clientes) K = num_clientes - 1;
	if (K <= 0) return listaCandidatos;

	for (int i = 1; i <= num_clientes; ++i) {
		std::vector<std::pair<int, int>> distancias;
		distancias.reserve(num_clientes - 1);
		for (int j = 1; j <= num_clientes; ++j) {
			if (i == j) continue;
			distancias.push_back({ cost_matrix[i][j], j });
		}

		// *** CAMBIO: Usamos partial_sort en lugar de sort. Es mucho más rápido para encontrar los "K mejores". ***
		std::partial_sort(distancias.begin(), distancias.begin() + K, distancias.end());

		listaCandidatos[i].reserve(K);
		for (int k = 0; k < K; ++k) {
			listaCandidatos[i].push_back(distancias[k].second);
		}
	}
	return listaCandidatos;
}

// --- BÚSQUEDA DE VECINDAD (OPERADORES) ---
bool explorarVecindadSwapIntra(std::vector<std::vector<int>>& rutas, CVRP* problema, int bestImprovement, std::vector<NodeLocation>& nodeLocations) {
	Move mejor_movimiento;
	mejor_movimiento.delta_cost = 0;
	int** cost_matrix = problema->getCost_Matrix();

	for (size_t r = 0; r < rutas.size(); ++r) {
		if (rutas[r].size() < 2) continue;
		for (size_t i = 0; i < rutas[r].size(); ++i) {
			const int u = rutas[r][i];
			const int pred_u = (i > 0) ? rutas[r][i - 1] : 0;
			const int succ_u = (i < rutas[r].size() - 1) ? rutas[r][i + 1] : 0;
			const double costo_removido_u = cost_matrix[pred_u][u] + cost_matrix[u][succ_u];

			for (size_t j = i + 1; j < rutas[r].size(); ++j) {
				const int v = rutas[r][j];
				const int pred_v = rutas[r][j - 1];
				const int succ_v = (j < rutas[r].size() - 1) ? rutas[r][j + 1] : 0;
				double costo_actual = costo_removido_u + cost_matrix[pred_v][v] + cost_matrix[v][succ_v];
				double costo_nuevo;

				if (i + 1 == j) {
					costo_actual -= cost_matrix[u][v];
					costo_nuevo = cost_matrix[pred_u][v] + cost_matrix[v][u] + cost_matrix[u][succ_v];
				}
				else {
					costo_nuevo = cost_matrix[pred_u][v] + cost_matrix[v][succ_u] + cost_matrix[pred_v][u] + cost_matrix[u][succ_v];
				}

				const double delta = costo_nuevo - costo_actual;
				if (delta < -1e-9) {
					if (bestImprovement == 0) {
						std::swap(rutas[r][i], rutas[r][j]);
						std::swap(nodeLocations[u].pos_idx, nodeLocations[v].pos_idx);
						return true;
					}
					if (delta < mejor_movimiento.delta_cost) {
						mejor_movimiento = { delta, 1, r, i, r, j };
					}
				}
			}
		}
	}

	if (bestImprovement == 1 && mejor_movimiento.delta_cost < -1e-9) {
		size_t r = mejor_movimiento.pos1_route;
		size_t i = mejor_movimiento.pos1_idx;
		size_t j = mejor_movimiento.pos2_idx;
		const int u = rutas[r][i];
		const int v = rutas[r][j];

		std::swap(rutas[r][i], rutas[r][j]);
		std::swap(nodeLocations[u].pos_idx, nodeLocations[v].pos_idx);
		return true;
	}
	return false;
}

bool explorarVecindad2OptIntra(std::vector<std::vector<int>>& rutas, CVRP* problema, int bestImprovement, std::vector<NodeLocation>& nodeLocations) {
	Move mejor_movimiento;
	mejor_movimiento.delta_cost = 0;
	int** cost_matrix = problema->getCost_Matrix();

	for (size_t r = 0; r < rutas.size(); ++r) {
		const size_t m = rutas[r].size();
		if (m < 2) continue;
		for (size_t i = 0; i < m - 1; ++i) {
			const int u1 = rutas[r][i];
			const int v1_pred = (i > 0) ? rutas[r][i - 1] : 0;

			for (size_t j = i + 1; j < m; ++j) {
				const int u2 = rutas[r][j];
				const int v2_succ = (j < m - 1) ? rutas[r][j + 1] : 0;
				const double costo_removido = cost_matrix[v1_pred][u1] + cost_matrix[u2][v2_succ];
				const double costo_agregado = cost_matrix[v1_pred][u2] + cost_matrix[u1][v2_succ];
				const double delta = costo_agregado - costo_removido;

				if (delta < -1e-9) {
					if (bestImprovement == 0) {
						std::reverse(rutas[r].begin() + i, rutas[r].begin() + j + 1);
						for (size_t k = i; k <= j; ++k) nodeLocations[rutas[r][k]].pos_idx = k;
						return true;
					}
					if (delta < mejor_movimiento.delta_cost) {
						mejor_movimiento = { delta, 2, r, i, (size_t)-1, j };
					}
				}
			}
		}
	}

	if (bestImprovement == 1 && mejor_movimiento.delta_cost < -1e-9) {
		size_t r = mejor_movimiento.pos1_route;
		size_t i = mejor_movimiento.pos1_idx;
		size_t j = mejor_movimiento.pos2_idx;
		std::reverse(rutas[r].begin() + i, rutas[r].begin() + j + 1);
		for (size_t k = i; k <= j; ++k) nodeLocations[rutas[r][k]].pos_idx = k;
		return true;
	}
	return false;
}

bool explorarVecindadSwapInter(std::vector<std::vector<int>>& rutas, CVRP* problema, std::vector<double>& demandas_rutas, int bestImprovement, const std::vector<std::vector<int>>& listaCandidatos, std::vector<NodeLocation>& nodeLocations) {
	Move mejor_movimiento;
	mejor_movimiento.delta_cost = 0;
	int** cost_matrix = problema->getCost_Matrix();
	int* demands = problema->getCustomerDemand();
	const int capacity = problema->getMaxCapacity();

	for (size_t r1 = 0; r1 < rutas.size(); ++r1) {
		for (size_t i = 0; i < rutas[r1].size(); ++i) {
			const int u = rutas[r1][i];

			for (int v : listaCandidatos[u]) {
				const NodeLocation& v_loc = nodeLocations[v];
				const size_t r2 = v_loc.route_idx;
				if (r1 >= r2) continue;
				const size_t j = v_loc.pos_idx;

				if (r2 >= rutas.size() || j >= rutas[r2].size() || rutas[r2][j] != v) continue;

				const int demanda_u = demands[u], demanda_v = demands[v];
				if (demandas_rutas[r1] - demanda_u + demanda_v > capacity || demandas_rutas[r2] - demanda_v + demanda_u > capacity) continue;

				const int pred_u = (i > 0) ? rutas[r1][i - 1] : 0;
				const int succ_u = (i < rutas[r1].size() - 1) ? rutas[r1][i + 1] : 0;
				const int pred_v = (j > 0) ? rutas[r2][j - 1] : 0;
				const int succ_v = (j < rutas[r2].size() - 1) ? rutas[r2][j + 1] : 0;

				const double costo_actual = cost_matrix[pred_u][u] + cost_matrix[u][succ_u] + cost_matrix[pred_v][v] + cost_matrix[v][succ_v];
				const double costo_nuevo = cost_matrix[pred_u][v] + cost_matrix[v][succ_u] + cost_matrix[pred_v][u] + cost_matrix[u][succ_v];
				const double delta = costo_nuevo - costo_actual;

				if (delta < -1e-9) {
					if (bestImprovement == 0) {
						demandas_rutas[r1] += demanda_v - demanda_u;
						demandas_rutas[r2] += demanda_u - demanda_v;
						std::swap(rutas[r1][i], rutas[r2][j]);
						std::swap(nodeLocations[u], nodeLocations[v]);
						return true;
					}
					if (delta < mejor_movimiento.delta_cost) {
						mejor_movimiento = { delta, 1, r1, i, r2, j };
					}
				}
			}
		}
	}

	if (bestImprovement == 1 && mejor_movimiento.delta_cost < -1e-9) {
		const size_t r1 = mejor_movimiento.pos1_route, i = mejor_movimiento.pos1_idx;
		const size_t r2 = mejor_movimiento.pos2_route, j = mejor_movimiento.pos2_idx;
		const int u = rutas[r1][i], v = rutas[r2][j];
		demandas_rutas[r1] += demands[v] - demands[u];
		demandas_rutas[r2] += demands[u] - demands[v];
		std::swap(rutas[r1][i], rutas[r2][j]);
		std::swap(nodeLocations[u], nodeLocations[v]);
		return true;
	}
	return false;
}

bool explorarVecindad2OptInter(std::vector<std::vector<int>>& rutas, CVRP* problema, std::vector<double>& demandas_rutas, int bestImprovement, const std::vector<std::vector<int>>& listaCandidatos, std::vector<NodeLocation>& nodeLocations) {
	Move mejor_movimiento;
	mejor_movimiento.delta_cost = 0;
	int** cost_matrix = problema->getCost_Matrix();
	int* demands = problema->getCustomerDemand();
	const int capacity = problema->getMaxCapacity();

	std::vector<std::vector<double>> suffixDemand(rutas.size());
	for (size_t r = 0; r < rutas.size(); ++r) {
		suffixDemand[r].resize(rutas[r].size() + 1, 0.0);
		for (int k = (int)rutas[r].size() - 1; k >= 0; --k) {
			suffixDemand[r][k] = demands[rutas[r][k]] + suffixDemand[r][k + 1];
		}
	}

	for (size_t r1 = 0; r1 < rutas.size(); ++r1) {
		if (rutas[r1].empty()) continue;
		for (size_t i = 0; i < rutas[r1].size(); ++i) {
			const int u1 = rutas[r1][i];
			for (int u2 : listaCandidatos[u1]) {
				const NodeLocation& u2_loc = nodeLocations[u2];
				const size_t r2 = u2_loc.route_idx;
				if (r1 >= r2) continue;
				const size_t j = u2_loc.pos_idx;

				if (r2 >= rutas.size() || j >= rutas[r2].size() || rutas[r2][j] != u2) continue;

				double demanda_cola1 = suffixDemand[r1][i + 1];
				double demanda_cola2 = suffixDemand[r2][j + 1];

				if (demandas_rutas[r1] - demanda_cola1 + demanda_cola2 > capacity ||
					demandas_rutas[r2] - demanda_cola2 + demanda_cola1 > capacity) {
					continue;
				}

				const int v1 = (i < rutas[r1].size() - 1) ? rutas[r1][i + 1] : 0;
				const int v2 = (j < rutas[r2].size() - 1) ? rutas[r2][j + 1] : 0;
				const double delta = (cost_matrix[u1][v2] + cost_matrix[u2][v1]) - (cost_matrix[u1][v1] + cost_matrix[u2][v2]);

				if (delta < -1e-9) {
					if (bestImprovement == 0) {
						demandas_rutas[r1] += demanda_cola2 - demanda_cola1;
						demandas_rutas[r2] += demanda_cola1 - demanda_cola2;

						std::vector<int> temp_cola(rutas[r1].begin() + i + 1, rutas[r1].end());
						rutas[r1].erase(rutas[r1].begin() + i + 1, rutas[r1].end());
						rutas[r1].insert(rutas[r1].end(), rutas[r2].begin() + j + 1, rutas[r2].end());
						rutas[r2].erase(rutas[r2].begin() + j + 1, rutas[r2].end());
						rutas[r2].insert(rutas[r2].end(), temp_cola.begin(), temp_cola.end());

						for (size_t p = i + 1; p < rutas[r1].size(); ++p) nodeLocations[rutas[r1][p]] = { r1, p };
						for (size_t p = j + 1; p < rutas[r2].size(); ++p) nodeLocations[rutas[r2][p]] = { r2, p };

						return true;
					}
					if (delta < mejor_movimiento.delta_cost) {
						mejor_movimiento = { delta, 3, r1, i, r2, j };
					}
				}
			}
		}
	}

	if (bestImprovement == 1 && mejor_movimiento.delta_cost < -1e-9) {
		const size_t r1 = mejor_movimiento.pos1_route, i = mejor_movimiento.pos1_idx;
		const size_t r2 = mejor_movimiento.pos2_route, j = mejor_movimiento.pos2_idx;

		double demanda_cola1 = suffixDemand[r1][i + 1];
		double demanda_cola2 = suffixDemand[r2][j + 1];
		demandas_rutas[r1] += demanda_cola2 - demanda_cola1;
		demandas_rutas[r2] += demanda_cola1 - demanda_cola2;

		std::vector<int> temp_cola(rutas[r1].begin() + i + 1, rutas[r1].end());
		rutas[r1].erase(rutas[r1].begin() + i + 1, rutas[r1].end());
		rutas[r1].insert(rutas[r1].end(), rutas[r2].begin() + j + 1, rutas[r2].end());
		rutas[r2].erase(rutas[r2].begin() + j + 1, rutas[r2].end());
		rutas[r2].insert(rutas[r2].end(), temp_cola.begin(), temp_cola.end());

		for (size_t p = i + 1; p < rutas[r1].size(); ++p) nodeLocations[rutas[r1][p]] = { r1, p };
		for (size_t p = j + 1; p < rutas[r2].size(); ++p) nodeLocations[rutas[r2][p]] = { r2, p };

		return true;
	}
	return false;
}


void busquedaLocalVND(std::vector<std::vector<int>>& rutas, Solution& sol, CVRP* problema, int bestImprovement, EstrategiaRuta est_ruta, const std::vector<std::vector<int>>& listaCandidatos, std::vector<NodeLocation>& nodeLocations) {
	if (rutas.empty()) return;

	std::vector<double> demandas_rutas(rutas.size());
	for (size_t i = 0; i < rutas.size(); ++i) {
		demandas_rutas[i] = 0;
		for (int nodo : rutas[i]) demandas_rutas[i] += problema->getCustomerDemand()[nodo];
	}

	bool mejora_encontrada = true;
	while (mejora_encontrada) {
		mejora_encontrada = false;
		if (est_ruta == EstrategiaRuta::INTRA || est_ruta == EstrategiaRuta::AMBAS) {
			if (explorarVecindadSwapIntra(rutas, problema, bestImprovement, nodeLocations)) { mejora_encontrada = true; continue; }
			if (explorarVecindad2OptIntra(rutas, problema, bestImprovement, nodeLocations)) { mejora_encontrada = true; continue; }
		}
		if (est_ruta == EstrategiaRuta::INTER || est_ruta == EstrategiaRuta::AMBAS) {
			if (explorarVecindadSwapInter(rutas, problema, demandas_rutas, bestImprovement, listaCandidatos, nodeLocations)) { mejora_encontrada = true; continue; }
			if (explorarVecindad2OptInter(rutas, problema, demandas_rutas, bestImprovement, listaCandidatos, nodeLocations)) { mejora_encontrada = true; continue; }
		}
	}

	reconstruirSolucionDesdeRutas(sol, rutas);
	problema->evaluate(&sol);
	problema->evaluateConstraints(&sol);
}

// --- FUNCIONES DE PERTURBACIÓN ---
void perturbarSolucionSwap(std::vector<std::vector<int>>& rutas, std::vector<NodeLocation>& nodeLocations, int num_clientes_total, CVRP* problema) {
	if (rutas.empty() || num_clientes_total < 4) return;
	RandomNumber* rnd = RandomNumber::getInstance();
	int num_perturbaciones = std::min(50, static_cast<int>(num_clientes_total * 0.15));

	for (int k = 0; k < num_perturbaciones; ++k) {
		if (rutas.size() < 2) continue;
		int r1 = rnd->nextInt(rutas.size() - 1);
		int r2 = rnd->nextInt(rutas.size() - 1);
		if (r1 == r2 || rutas[r1].empty() || rutas[r2].empty()) continue;
		int idx1 = rnd->nextInt(rutas[r1].size() - 1);
		int idx2 = rnd->nextInt(rutas[r2].size() - 1);

		const int u = rutas[r1][idx1];
		const int v = rutas[r2][idx2];

		std::swap(rutas[r1][idx1], rutas[r2][idx2]);
		std::swap(nodeLocations[u], nodeLocations[v]);
	}
}

void perturbarSolucionScramble(std::vector<std::vector<int>>& rutas, std::vector<NodeLocation>& nodeLocations, int num_clientes_total, CVRP* problema) {
	if (rutas.empty() || num_clientes_total < 4) return;

	std::vector<int> todos_los_clientes;
	todos_los_clientes.reserve(num_clientes_total);
	std::vector<size_t> tamanos_rutas;
	tamanos_rutas.reserve(rutas.size());

	for (const auto& ruta : rutas) {
		if (!ruta.empty()) {
			todos_los_clientes.insert(todos_los_clientes.end(), ruta.begin(), ruta.end());
			tamanos_rutas.push_back(ruta.size());
		}
	}

	if (todos_los_clientes.size() < 2) return;

	RandomNumber* rnd = RandomNumber::getInstance();
	int pos1, pos2;
	do {
		pos1 = rnd->nextInt(todos_los_clientes.size() - 1);
		pos2 = rnd->nextInt(todos_los_clientes.size() - 1);
	} while (pos1 == pos2);

	if (pos1 > pos2) std::swap(pos1, pos2);

	std::shuffle(todos_los_clientes.begin() + pos1, todos_los_clientes.begin() + pos2 + 1, std::mt19937{ std::random_device{}() });

	auto cliente_iterator = todos_los_clientes.begin();
	size_t ruta_idx_no_vacia = 0;
	for (size_t i = 0; i < rutas.size(); ++i) {
		if (!rutas[i].empty()) {
			rutas[i].assign(cliente_iterator, cliente_iterator + tamanos_rutas[ruta_idx_no_vacia]);
			cliente_iterator += tamanos_rutas[ruta_idx_no_vacia];
			ruta_idx_no_vacia++;
		}
	}

	for (size_t r = 0; r < rutas.size(); ++r) {
		for (size_t p = 0; p < rutas[r].size(); ++p) {
			nodeLocations[rutas[r][p]] = { r, p };
		}
	}
}

// *** CAMBIO: Se elimina la fuga de memoria de 'new SolutionSet' creándolo en la pila (stack). ***
void perturbarSolucionCombination(
	SolutionSet& seto,
	Solution& sol,
	std::vector<std::vector<int>>& rutas,
	std::vector<NodeLocation>& nodeLocations,
	CVRP* problema)
{
	CombinationMutation mutator;

	// Se crea el objeto en la pila para que se destruya automáticamente. ¡No más fugas!
	
	seto.set(0, sol);
	mutator.execute(seto, sol, 1.0); // 'sol' es modificada aquí

	// Resincroniza las estructuras de datos locales
	decodificarRutas(sol, problema, rutas);

	nodeLocations.assign(problema->getNumberCustomers() + 1, {});
	for (size_t r = 0; r < rutas.size(); ++r) {
		for (size_t p = 0; p < rutas[r].size(); ++p) {
			nodeLocations[rutas[r][p]] = { r, p };
		}
	}
}


// --- CLASE PRINCIPAL ---
LocalSearch::LocalSearch() {}

void LocalSearch::initialize(Requirements* config) {
	config->addValue("#Iteraciones-sin-mejora", Constantes::INT);
	config->addValue("#BestImprovement", Constantes::INT);
	config->addValue("#EstrategiaRuta", Constantes::INT);
	this->param = *config->load();
}

// *** CAMBIO: El cuerpo de la función se adapta para trabajar de forma segura con el parámetro `y` pasado por valor. ***
void LocalSearch::execute(Solution y) {
	SolutionSet seto(1, 1, y.getProblem());
	const int MAX_ITER_SIN_MEJORA = this->param.get("#Iteraciones-sin-mejora").getInt();
	const int BEST_IMPROVEMENT = this->param.get("#BestImprovement").getInt();
	const auto ESTRATEGIA_RUTA = static_cast<EstrategiaRuta>(this->param.get("#EstrategiaRuta").getInt());

	CVRP* problema = dynamic_cast<CVRP*>(y.getProblem());
	const bool maximization = y.getProblem()->getObjectivesType()[0] == Constantes::MAXIMIZATION;
	const int K = 15;
	std::vector<std::vector<int>> listaCandidatos_ = generarListaCandidatos(problema->getNumberCustomers(), K, problema->getCost_Matrix());

	// --- Pool de Soluciones Internas ---
	// Se crean los objetos Solution UNA SOLA VEZ para minimizar fugas y evitar copias peligrosas en el bucle.
	Solution mejor_solucion(problema);
	Solution candidata_actual(problema);

	// Se copia el estado del parámetro 'y' a nuestra solución interna 'mejor_solucion'.
	mejor_solucion = y;

	// --- Estructuras de datos reutilizables ---
	std::vector<std::vector<int>> rutas_candidatas;
	std::vector<NodeLocation> nodeLocations(problema->getNumberCustomers() + 1);

	auto sincronizar_locaciones = [&](const std::vector<std::vector<int>>& rutas) {
		nodeLocations.assign(problema->getNumberCustomers() + 1, {});
		for (size_t r = 0; r < rutas.size(); ++r) {
			for (size_t p = 0; p < rutas[r].size(); ++p) {
				nodeLocations[rutas[r][p]] = { r, p };
			}
		}
		};

	// Búsqueda local inicial sobre nuestra copia interna segura.
	decodificarRutas(mejor_solucion, problema, rutas_candidatas);
	sincronizar_locaciones(rutas_candidatas);
	busquedaLocalVND(rutas_candidatas, mejor_solucion, problema, BEST_IMPROVEMENT, ESTRATEGIA_RUTA, listaCandidatos_, nodeLocations);

	int iteraciones_sin_mejora = 0;

	while (iteraciones_sin_mejora < MAX_ITER_SIN_MEJORA) {
		// Se reutiliza la 'candidata_actual' copiando el estado de 'mejor_solucion'
		candidata_actual = mejor_solucion;
		decodificarRutas(candidata_actual, problema, rutas_candidatas);
		sincronizar_locaciones(rutas_candidatas);

		RandomNumber* rnd = RandomNumber::getInstance();
		if (rnd->nextDouble() < 0.3) {
			perturbarSolucionCombination(seto,candidata_actual, rutas_candidatas, nodeLocations, problema);
		}
		else {
			perturbarSolucionCombination(seto,candidata_actual, rutas_candidatas, nodeLocations, problema);
		}

		busquedaLocalVND(rutas_candidatas, candidata_actual, problema, BEST_IMPROVEMENT, ESTRATEGIA_RUTA, listaCandidatos_, nodeLocations);

		bool es_mejor = false;
		if (candidata_actual.getNumberOfViolatedConstraints() == 0 && mejor_solucion.getNumberOfViolatedConstraints() == 0) {
			es_mejor = (!maximization && candidata_actual.getObjective(0).L < mejor_solucion.getObjective(0).L);
		}
		else if (candidata_actual.getNumberOfViolatedConstraints() < mejor_solucion.getNumberOfViolatedConstraints()) {
			es_mejor = true;
		}

		if (es_mejor) {
			mejor_solucion = candidata_actual;
			iteraciones_sin_mejora = 0;
		}
		else {
			iteraciones_sin_mejora++;
		}
	}

	// *** CAMBIO: Antes de terminar, se copia el estado de la mejor solución encontrada de vuelta al parámetro 'y'. ***
	y = mejor_solucion;

}