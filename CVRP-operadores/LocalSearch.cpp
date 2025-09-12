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
#include <chrono>


// --- ESTRUCTURAS Y ENUMS --- 
enum class EstrategiaMejora { FIRST = 0, BEST = 1 };
enum class EstrategiaRuta { INTRA = 0, INTER = 1, AMBAS = 2 };

struct Move {
	double delta_cost = 0.0;
	int type = -1;
	size_t pos1_route = -1, pos1_idx = -1;
	size_t pos2_route = -1, pos2_idx = -1;
};

// Estructura para localizar un nodo rápidamente 
struct NodeLocation {
	size_t route_idx;
	size_t pos_idx;
};

void print_vector(const std::string& label, const std::vector<int>& vec) {
	std::cout << label;
	for (int val : vec) {
		std::cout << val << " ";
	}
	std::cout << std::endl;
}


// --- FUNCIONES DE UTILIDAD --- 
std::vector<std::vector<int>> decodificarRutas(Solution& sol, CVRP* problema) {
	std::vector<std::vector<int>> rutas;
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
	return rutas;
}

void reconstruirSolucionDesdeRutas(Solution& sol, const std::vector<std::vector<int>>& rutas) {
	std::vector<int> valores_planos;
	valores_planos.reserve(sol.getNumVariables());
	for (size_t i = 0; i < rutas.size(); ++i) {
		if (rutas[i].empty()) continue;
		valores_planos.insert(valores_planos.end(), rutas[i].begin(), rutas[i].end());
		if (i < rutas.size() - 1) {
			// Solo añade depot si no es la última ruta y hay más rutas no vacías después 
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

	for (int i = 1; i <= num_clientes; ++i) {
		std::vector<std::pair<int, int>> distancias;
		distancias.reserve(num_clientes);
		for (int j = 1; j <= num_clientes; ++j) {
			if (i == j) continue;
			distancias.push_back({ cost_matrix[i][j], j });
		}

		std::sort(distancias.begin(), distancias.end());

		listaCandidatos[i].reserve(K);
		for (int k = 0; k < K; ++k) {
			listaCandidatos[i].push_back(distancias[k].second);
		}
	}
	return listaCandidatos;
}


// --- BÚSQUEDA DE VECINDAD INTRA-RUTA --- 
bool explorarVecindadSwapIntra(std::vector<std::vector<int>>& rutas, CVRP* problema, int bestImprovement,std::vector<NodeLocation>& nodeLocations){
	Move mejor_movimiento;
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
						// ACTUALIZACIÓN: Intercambia solo las posiciones
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
		const int u = rutas[mejor_movimiento.pos1_route][mejor_movimiento.pos1_idx];
		const int v = rutas[mejor_movimiento.pos2_route][mejor_movimiento.pos2_idx];
		std::swap(rutas[mejor_movimiento.pos1_route][mejor_movimiento.pos1_idx], rutas[mejor_movimiento.pos2_route][mejor_movimiento.pos2_idx]);
	 
		std::swap(nodeLocations[u].pos_idx, nodeLocations[v].pos_idx);
		return true;
	}

	return false;
}

bool explorarVecindad2OptIntra(std::vector<std::vector<int>>& rutas,
	CVRP* problema,
	int bestImprovement, std::vector<NodeLocation>& nodeLocations) {



	Move mejor_movimiento;
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

				const double costo_removido =
					cost_matrix[v1_pred][u1] + cost_matrix[u2][v2_succ];
				const double costo_agregado =
					cost_matrix[v1_pred][u2] + cost_matrix[u1][v2_succ];
				const double delta = costo_agregado - costo_removido;

				if (delta < -1e-9) {
					if (bestImprovement == 0) {
						std::reverse(rutas[r].begin() + i,
							rutas[r].begin() + j + 1);
						for (size_t k = i; k <= j; ++k) {
							nodeLocations[rutas[r][k]].pos_idx = k;
						}
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
		// ACTUALIZACIÓN: Recorre el segmento invertido y actualiza sus posiciones
		for (size_t k = i; k <= j; ++k) {
			nodeLocations[rutas[r][k]].pos_idx = k;
		}
		return true;
	}
	return false;
}



 
bool explorarVecindadSwapInter(std::vector<std::vector<int>>& rutas, CVRP* problema, std::vector<double>& demandas_rutas, int bestImprovement, const std::vector<std::vector<int>>& listaCandidatos, std::vector<NodeLocation>& nodeLocations) {
	Move mejor_movimiento;
	int** cost_matrix = problema->getCost_Matrix();
	int* demands = problema->getCustomerDemand();
	const int capacity = problema->getMaxCapacity();

	cout << "************ANTES DE ACTUALIZAR******SWAPINTER*************" << endl;

	// 3. Recorremos el vector e imprimimos los miembros de cada objeto
	for (const auto& location : nodeLocations) {
		std::cout << "Ruta: " << location.route_idx
			<< ", Posicion: " << location.pos_idx << std::endl;
	}
	cout << "**********************************************" << endl;

	//////////////// DEPURAR ACTUALIZACION DE NODE LOCATIONS ///////////////////////

	//// Función auxiliar para mantener 'nodeLocations' sincronizado con 'rutas'
	//auto sincronizar_locaciones = [&](const std::vector<std::vector<int>>& rutas) {
	//	for (size_t r = 0; r < rutas.size(); ++r) {
	//		for (size_t p = 0; p < rutas[r].size(); ++p) {
	//			nodeLocations[rutas[r][p]] = { r, p };
	//		}
	//	}
	//	};
	//sincronizar_locaciones(rutas);


	for (size_t r1 = 0; r1 < rutas.size(); ++r1) {
		for (size_t i = 0; i < rutas[r1].size(); ++i) {
			const int u = rutas[r1][i];

			for (int v : listaCandidatos[u]) {
				const NodeLocation& v_loc = nodeLocations[v];
				const size_t r2 = v_loc.route_idx;
				if (r1 >= r2) continue; 
				const size_t j = v_loc.pos_idx;

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
						std::swap(nodeLocations[u], nodeLocations[v]); // Actualización crítica 
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
		std::swap(nodeLocations[u], nodeLocations[v]); // Actualización crítica 
		return true;
	}
	return false;
}

bool explorarVecindad2OptInter(std::vector<std::vector<int>>& rutas,
	CVRP* problema,
	std::vector<double>& demandas_rutas,
	int bestImprovement,
	const std::vector<std::vector<int>>& listaCandidatos,
	std::vector<NodeLocation>& nodeLocations) {

	

	Move mejor_movimiento;
	int** cost_matrix = problema->getCost_Matrix();
	int* demands = problema->getCustomerDemand();
	const int capacity = problema->getMaxCapacity();

	// --- Precomputar suffix sums de demandas por ruta --- 
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

				// --- Usar suffix sums en O(1) --- 
				double demanda_cola1 = suffixDemand[r1][i + 1];
				double demanda_cola2 = suffixDemand[r2][j + 1];

				if (demandas_rutas[r1] - demanda_cola1 + demanda_cola2 > capacity ||
					demandas_rutas[r2] - demanda_cola2 + demanda_cola1 > capacity) {
					continue;
				}

				const int v1 = (i < rutas[r1].size() - 1) ? rutas[r1][i + 1] : 0;
				const int v2 = (j < rutas[r2].size() - 1) ? rutas[r2][j + 1] : 0;
				const double delta = (cost_matrix[u1][v2] + cost_matrix[u2][v1]) -
					(cost_matrix[u1][v1] + cost_matrix[u2][v2]);

				if (delta < -1e-9) {
					if (bestImprovement == 0) {
						// --- aplicar movimiento --- 
						std::vector<int> cola1(rutas[r1].begin() + i + 1, rutas[r1].end());
						std::vector<int> cola2(rutas[r2].begin() + j + 1, rutas[r2].end());
						rutas[r1].resize(i + 1);
						rutas[r1].insert(rutas[r1].end(), cola2.begin(), cola2.end());
						rutas[r2].resize(j + 1);
						rutas[r2].insert(rutas[r2].end(), cola1.begin(), cola1.end());
						demandas_rutas[r1] += demanda_cola2 - demanda_cola1;
						demandas_rutas[r2] += demanda_cola1 - demanda_cola2;

						// --- actualizar suffix sums sólo para r1 y r2 --- 
						suffixDemand[r1].resize(rutas[r1].size() + 1, 0.0);
						for (int k = (int)rutas[r1].size() - 1; k >= 0; --k)
							suffixDemand[r1][k] = demands[rutas[r1][k]] + suffixDemand[r1][k + 1];
						suffixDemand[r2].resize(rutas[r2].size() + 1, 0.0);
						for (int k = (int)rutas[r2].size() - 1; k >= 0; --k)
							suffixDemand[r2][k] = demands[rutas[r2][k]] + suffixDemand[r2][k + 1];


						for (size_t k = 0; k < cola2.size(); ++k) {
							nodeLocations[cola2[k]] = { r1, i + 1 + k };
						}
						for (size_t k = 0; k < cola1.size(); ++k) {
							nodeLocations[cola1[k]] = { r2, j + 1 + k };
						}


						return true;
					}
					if (delta < mejor_movimiento.delta_cost) {
						mejor_movimiento = { delta, 3, r1, i, r2, j };
					}
				}
			}
		}
	}

	// --- aplicar best improvement si existe --- 
	if (bestImprovement == 1 && mejor_movimiento.delta_cost < -1e-9) {
		const size_t r1 = mejor_movimiento.pos1_route, i = mejor_movimiento.pos1_idx;
		const size_t r2 = mejor_movimiento.pos2_route, j = mejor_movimiento.pos2_idx;
		double demanda_cola1 = suffixDemand[r1][i + 1];
		double demanda_cola2 = suffixDemand[r2][j + 1];

		std::vector<int> cola1(rutas[r1].begin() + i + 1, rutas[r1].end());
		std::vector<int> cola2(rutas[r2].begin() + j + 1, rutas[r2].end());
		rutas[r1].resize(i + 1);
		rutas[r1].insert(rutas[r1].end(), cola2.begin(), cola2.end());
		rutas[r2].resize(j + 1);
		rutas[r2].insert(rutas[r2].end(), cola1.begin(), cola1.end());
		demandas_rutas[r1] += demanda_cola2 - demanda_cola1;
		demandas_rutas[r2] += demanda_cola1 - demanda_cola2;

		// --- actualizar suffix sums sólo para r1 y r2 --- 
		suffixDemand[r1].resize(rutas[r1].size() + 1, 0.0);
		for (int k = (int)rutas[r1].size() - 1; k >= 0; --k)
			suffixDemand[r1][k] = demands[rutas[r1][k]] + suffixDemand[r1][k + 1];
		suffixDemand[r2].resize(rutas[r2].size() + 1, 0.0);
		for (int k = (int)rutas[r2].size() - 1; k >= 0; --k)
			suffixDemand[r2][k] = demands[rutas[r2][k]] + suffixDemand[r2][k + 1];

		// ACTUALIZACIÓN: Recorre los segmentos movidos y actualiza su ruta y posición
		for (size_t k = 0; k < cola2.size(); ++k) {
			nodeLocations[cola2[k]] = { r1, i + 1 + k };
		}
		for (size_t k = 0; k < cola1.size(); ++k) {
			nodeLocations[cola1[k]] = { r2, j + 1 + k };
		}

		return true;
	}
	return false;
}



// --- ORQUESTADOR VND (Variable Neighborhood Descent) --- 
void busquedaLocalVND(
	std::vector<std::vector<int>>& rutas, // Recibe las rutas directamente
	Solution& sol,                         // Recibe la solución para actualizarla al final
	CVRP* problema,
	int bestImprovement,
	EstrategiaRuta est_ruta,
	const std::vector<std::vector<int>>& listaCandidatos,
	std::vector<NodeLocation>& nodeLocations // ¡Recibe nodeLocations por referencia!
) {
	// Ya no decodificamos las rutas, las recibimos como parámetro
	if (rutas.empty()) return;

	const int* demands = problema->getCustomerDemand();
	std::vector<double> demandas_rutas(rutas.size());
	for (size_t i = 0; i < rutas.size(); ++i) {
		demandas_rutas[i] = 0;
		for (int nodo : rutas[i]) demandas_rutas[i] += demands[nodo];
	}

 
	bool mejora_encontrada = true;
	while (mejora_encontrada) {
		mejora_encontrada = false;
		//////////////// DEPURAR ACTUALIZACION DE NODE LOCATIONS ///////////////////////
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



// Cambia la firma para aceptar nodeLocations
void perturbarSolucionSwap(std::vector<std::vector<int>>& rutas, std::vector<NodeLocation>& nodeLocations, int num_clientes_total) {
	if (rutas.empty() || num_clientes_total < 4) return;
	RandomNumber* rnd = RandomNumber::getInstance();
	int num_perturbaciones = std::min(100, static_cast<int>(num_clientes_total * 0.15));

	for (int k = 0; k < num_perturbaciones; ++k) {
		if (rutas.size() < 2) continue;
		int r1 = rnd->nextInt(rutas.size() - 1), r2 = rnd->nextInt(rutas.size() - 1);
		if (r1 == r2 || rutas[r1].empty() || rutas[r2].empty()) continue;
		int idx1 = rnd->nextInt(rutas[r1].size() - 1), idx2 = rnd->nextInt(rutas[r2].size() - 1);

		// Obtenemos los IDs de los nodos ANTES del swap
		const int u = rutas[r1][idx1];
		const int v = rutas[r2][idx2];

		std::swap(rutas[r1][idx1], rutas[r2][idx2]);

		// AÑADIR ESTA LÍNEA: Actualización atómica
		std::swap(nodeLocations[u], nodeLocations[v]);
	}
}


void perturbarSolucionScramble(std::vector<std::vector<int>>& rutas, std::vector<NodeLocation>& nodeLocations, int num_clientes_total) {
	if (rutas.empty() || num_clientes_total < 4) return;

	// 1. Aplanar todas las rutas en una sola lista de clientes
	std::vector<int> todos_los_clientes;
	todos_los_clientes.reserve(num_clientes_total);
	std::vector<size_t> tamanos_rutas; // Guardar el tamaño original de cada ruta
	tamanos_rutas.reserve(rutas.size());

	for (const auto& ruta : rutas) {
		if (!ruta.empty()) {
			todos_los_clientes.insert(todos_los_clientes.end(), ruta.begin(), ruta.end());
			tamanos_rutas.push_back(ruta.size());
		}
	}

	if (todos_los_clientes.size() < 2) return;

	RandomNumber* rnd = RandomNumber::getInstance();

	// 2. Seleccionar un subconjunto aleatorio para barajar (lógica de Scramble)
	int pos1 = rnd->nextInt(todos_los_clientes.size() - 1);
	int pos2;
	do {
		pos2 = rnd->nextInt(todos_los_clientes.size() - 1);
	} while (pos1 == pos2);

	if (pos1 > pos2) {
		std::swap(pos1, pos2);
	}

	// 3. Barajar (scramble) el subconjunto usando el algoritmo de Fisher-Yates
	for (int i = pos2; i > pos1; --i) {
		// Elige un índice aleatorio 'j' en el rango no barajado [pos1, i]
		int j = pos1 + rnd->nextInt(i - pos1); //  
		std::swap(todos_los_clientes[i], todos_los_clientes[j]);
	}

	// 4. Reconstruir las rutas con la lista de clientes ya barajada
	auto cliente_iterator = todos_los_clientes.begin();
	size_t ruta_idx_no_vacia = 0; // Un contador para tamanos_rutas
	for (size_t i = 0; i < rutas.size(); ++i) {
		if (!rutas[i].empty()) {
			rutas[i].assign(cliente_iterator, cliente_iterator + tamanos_rutas[ruta_idx_no_vacia]);
			cliente_iterator += tamanos_rutas[ruta_idx_no_vacia];
			ruta_idx_no_vacia++;
		}
	}

	// AÑADIR ESTO: Reconstruir nodeLocations desde las rutas modificadas
	for (size_t r = 0; r < rutas.size(); ++r) {
		for (size_t p = 0; p < rutas[r].size(); ++p) {
			nodeLocations[rutas[r][p]] = { r, p };
		}
	}
}






LocalSearch::LocalSearch() {}

void LocalSearch::initialize(Requirements* config) {
	config->addValue("#Iteraciones-sin-mejora", Constantes::INT);
	config->addValue("#BestImprovement", Constantes::INT);
	config->addValue("#EstrategiaRuta", Constantes::INT);
	this->param = *config->load();
}



void LocalSearch::execute(Solution y) {
	// 1. Obtener parámetros y problema
	const int MAX_ITER_SIN_MEJORA = this->param.get("#Iteraciones-sin-mejora").getInt();
	const int BEST_IMPROVEMENT = this->param.get("#BestImprovement").getInt();
	const auto ESTRATEGIA_RUTA = static_cast<EstrategiaRuta>(this->param.get("#EstrategiaRuta").getInt());

	CVRP* problema = dynamic_cast<CVRP*>(y.getProblem());
	const bool maximization = y.getProblem()->getObjectivesType()[0] == Constantes::MAXIMIZATION;

	const int K = 15; // Tamaño de la lista de candidatos
	std::vector<std::vector<int>> listaCandidatos_ = generarListaCandidatos(problema->getNumberCustomers(), K, problema->getCost_Matrix());

	// ======================= INICIO DE CAMBIOS PRINCIPALES =======================

	// 2. Decodificar la solución inicial y preparar las estructuras de datos
	auto rutas_candidatas = decodificarRutas(y, problema);
	std::vector<NodeLocation> nodeLocations(problema->getNumberCustomers() + 1);

	// Función auxiliar para mantener 'nodeLocations' sincronizado con 'rutas'
	auto sincronizar_locaciones = [&](const std::vector<std::vector<int>>& rutas) {
		for (size_t r = 0; r < rutas.size(); ++r) {
			for (size_t p = 0; p < rutas[r].size(); ++p) {
				nodeLocations[rutas[r][p]] = { r, p };
			}
		}
		};

	sincronizar_locaciones(rutas_candidatas); // Sincronización inicial

	// 3. Búsqueda local inicial (ahora usando las nuevas estructuras)
	// Esta llamada modificará 'rutas_candidatas' y actualizará la solución 'y'
	busquedaLocalVND(rutas_candidatas, y, problema, BEST_IMPROVEMENT, ESTRATEGIA_RUTA, listaCandidatos_, nodeLocations);

	// 4. Establecer la solución mejorada como punto de partida para ILS
	Solution mejor_solucion = y;
	int iteraciones_sin_mejora = 0;

	// 5. Bucle principal de Búsqueda Local Iterada (ILS)
	while (iteraciones_sin_mejora < MAX_ITER_SIN_MEJORA) {
		Solution candidata_actual = mejor_solucion;
		RandomNumber* rnd = RandomNumber::getInstance();

		// Perturbación (ahora recibe y actualiza 'nodeLocations')
		if (rnd->nextDouble() < 0.3) {
			perturbarSolucionScramble(rutas_candidatas, nodeLocations, problema->getNumberCustomers());
		}
		else {
			perturbarSolucionSwap(rutas_candidatas, nodeLocations, problema->getNumberCustomers());
		}

		// Búsqueda local sobre la solución perturbada
		busquedaLocalVND(rutas_candidatas, candidata_actual, problema, BEST_IMPROVEMENT, ESTRATEGIA_RUTA, listaCandidatos_, nodeLocations);

		// Criterio de aceptación
		bool es_mejor = false;
		if (candidata_actual.getNumberOfViolatedConstraints() == 0 && mejor_solucion.getNumberOfViolatedConstraints() == 0) {
			es_mejor = (!maximization && candidata_actual.getObjective(0).L < mejor_solucion.getObjective(0).L);
		}
		else if (candidata_actual.getNumberOfViolatedConstraints() < mejor_solucion.getNumberOfViolatedConstraints()) {
			es_mejor = true;
		}

		if (es_mejor) {
			mejor_solucion = candidata_actual;
			// Las 'rutas_candidatas' ya son las correctas para la siguiente iteración
			// 'nodeLocations' ya fue actualizado por la búsqueda local. No se necesita hacer nada.
			iteraciones_sin_mejora = 0;
		}
		else {
			// Si no hay mejora, revertimos las rutas a las de la última mejor solución
			rutas_candidatas = decodificarRutas(mejor_solucion, problema);
			sincronizar_locaciones(rutas_candidatas); // Revertir también las locaciones es crucial
			iteraciones_sin_mejora++;
		}
	}

	// ======================= FIN DE CAMBIOS PRINCIPALES =======================

	y = mejor_solucion; // Asignar la mejor solución encontrada
}