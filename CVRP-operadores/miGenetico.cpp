#include "miGenetico.h"
#include <string>
#include <algorithm>
#include "utility"
#include <vector>
#include <string>
#include <sstream> // Necesario para std::stringstream


miGenetico::miGenetico() :Algorithm(NULL) {

}



void miGenetico::initialize() {

}


void miGenetico::initialize(Requirements* req) {

	req->addValue("#N", Constantes::INT);
	req->addValue("#MAX_GENERATIONS", Constantes::INT);
	req->addValue("#Problem-Instance", Constantes::STRING);
	//req->addValue("#OPTIMO", Constantes::INT);

	this->param_ = *(req->load());

	this->MAX_GENERATIONS = this->param_.get("#MAX_GENERATIONS").getInt();
	this->N = this->param_.get("#N").getInt();
	/*this->OPTIMO = this->param_.get("#OPTIMO").getInt();*/


	this->problem_ = ProblemBuilder::execute(this->param_.get("#Problem-Instance").getString());

	this->mo = (DEMutationOperator*)DEMutationBuilder::execute(req);
	this->co = (CrossoverOperator*)CrossoverBuilder::execute(req);
	this->so = (SelectionOperator*)SelectionBuilder::execute(req);
	this->improvement = (ImprovementOperator*)ImprovementBuilder::execute(req);
	this->reparacion = (RepairOperator*)RepairBuilder::execute(req);

	pob = new SolutionSet((2 * N), this->problem_);

}

void imprimirConjuntoSoluciones(SolutionSet* soluciones) {


	cout << "Soluciones" << endl;
	for (int i = 0; i < soluciones->size(); i++)
	{
		Solution sol = soluciones->get(i);
		cout << "[";
		for (int j = 0; j < sol.getNumVariables(); j++)
		{
			cout << sol.getVariableValue(j);
			if (j < sol.getNumVariables() - 1) {
				cout << ", ";
			}

		}
		cout << "]";

		cout << " [" << sol.getObjective(0) << "]";
		cout << " [" << sol.getResourceValue(0) << "]" << endl;

	}
	cout << " " << endl;


}

void imprimirSolucion(Solution sol) {



	for (int j = 0; j < sol.getNumVariables(); j++)
	{
		cout << sol.getVariableValue(j);
		if (j < sol.getNumVariables() - 1) {
			cout << ", ";
		}
	}
	cout << " " <<endl;


}

// La función ahora devuelve un std::string
std::string obtenerSolucionComoCadena(Solution sol) {
	std::stringstream ss; // Se crea un stringstream para construir la cadena
	ss << "[";
	for (int j = 0; j < sol.getNumVariables(); j++) {
		ss << sol.getVariableValue(j); // Se añade el valor al stream
		if (j < sol.getNumVariables() - 1) {
			ss << " "; // Se añade el separador al stream
		}
	}
	// ss << " "; // Descomenta esta línea si necesitas un espacio al final
	ss << "]\n"; // Cierra el corchete
	return ss.str(); // Se convierte el contenido del stream a una cadena y se devuelve
}


void miGenetico::execute() {
	int generacionesSinMejora = 0;
	int topeSinMejora = 250;

	int pobSize = 2 * this->N;
	int generation = 0;
	bool first = true;
	Solution nueva(this->problem_);

	best = new SolutionSet(1, 1, this->problem_);
	SolutionSet parents(2, 2, this->problem_);
	SolutionSet children(2, 2, this->problem_);
	////
	   // Declarar la variable aquí
	long double best_fitness_anterior = 0.0; // Usar long double si .L es long double

	// Generar población inicial
	for (int i = 0; i < pobSize; i++) {
		nueva = this->problem_->generateRandomSolution();

		//std::vector<int> digitos2 = { 28,12,80,68,29,24,54,55,25,4,26,53,0,92,37,98,100,91,16,86,44,38,14,42,43,15,57,2,58,0,27,69,1,70,30,20,66,32,90,63,10,62,88,31,0,6,96,99,59,93,85,61,17,45,84,5,60,89,0,52,7,82,48,19,11,64,49,36,47,46,8,83,18,0,73,74,22,41,23,67,39,56,75,72,21,40,0,13,87,97,95,94,0,50,33,81,51,9,71,65,35,34,78,79,3,77,76,-1 };
		//std::vector<int> digitos = { 20, 24, 25, 27, 29, 30, 28, 26, 23, 22, 21, 0, 32, 33, 31, 35, 37, 38, 39, 36, 34, 0, 43, 42, 41, 40, 44, 45, 46, 48, 51, 50, 52, 49, 47, 0, 69, 68, 64, 61, 72, 80, 79, 77, 73, 70, 71, 76, 78, 81, 0, 99, 100, 97, 93, 92, 94, 95, 96, 98, 0, 65, 63, 74, 62, 66, 67, 0, 57, 55, 54, 53, 56, 58, 60, 59, 0, 75, 1, 2, 4, 6, 9, 11, 8, 7, 3, 5, 0, 10, 12, 14, 16, 15, 19, 18, 17, 13, 0, 90, 87, 86, 83, 82, 84, 85, 88, 89, 91, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

		//int prueba =nueva.getNumVariables();
		//int prueba2 = digitos.size();

		//for (size_t hh = 0; hh < nueva.getNumVariables(); hh++)
		//{
		//	nueva.setVariableValue(hh, digitos[hh]);
		//}
		//imprimirSolucion(nueva);

		//this->problem_->evaluate(&nueva);
		//this->problem_->evaluateConstraints(&nueva);

		//cout << endl;
		//cout << "numero de restricciones violadas" << endl;
		//cout<<nueva.getNumberOfViolatedConstraints() << endl;
		//cout << "COSTO TOTAL de restricciones violadas"<< endl;
		//cout << nueva.getOverallConstraintViolation()<< endl;

		this->pob->add(nueva);

		if (first) {
			best->set(0, nueva);
			first = false;
		}
		else {
			bool maximization = this->problem_->getObjectivesType()[0] == Constantes::MAXIMIZATION;
			if (maximization && nueva.getObjective(0) > best->get(0).getObjective(0) && nueva.getNumberOfViolatedConstraints() == 0) {
				best->set(0, nueva);
			}
			else if (!maximization && nueva.getObjective(0) < best->get(0).getObjective(0) && nueva.getNumberOfViolatedConstraints() == 0) {
				best->set(0, nueva);
			}
		}
	}

	// <<< CAMBIO 1: Inicializar best_fitness_anterior DESPUÉS de la población inicial >>>
	best_fitness_anterior = best->get(0).getObjective(0).L;

	// Bucle principal
	while (generation < this->MAX_GENERATIONS) {

		for (int i = 0; i < this->N; i++) {
			// Selección
			//parents.set(0, this->so->execute(*pob));
			//parents.set(1, this->so->execute(*pob));

			// Cruza
			this->co->execute(*pob, children);

			// Mutación
			this->mo->execute(*pob, children.get(0));
			this->mo->execute(*pob, children.get(1));



			////// Reparación
			// 
			
	/*		CVRP_Repair rep;
			rep.execute(children.getptr(0));
			rep.execute(children.getptr(1));*/

			this->reparacion->execute(children.get(0));
			this->reparacion->execute(children.get(1));

			// Evaluación
			this->problem_->evaluate(children.getptr(0));
			this->problem_->evaluateConstraints(children.getptr(0));
			this->problem_->evaluate(children.getptr(1));
			this->problem_->evaluateConstraints(children.getptr(1));

			RandomNumber* rnd = RandomNumber::getInstance();

			/*
			cout << endl;*/
			// Si el número es menor a 1 (10% de probabilidad), aplica la mejora.
			if (rnd->nextDouble() < 0.1) {
				/*cout << "pase\r" ;*/
				this->improvement->execute(children.get(0));

			}
			if (rnd->nextDouble() < 0.1) {
				/*cout << "pase\r" ;*/

				this->improvement->execute(children.get(1));
			}



			children.get(0).getVariableValue(0).L;
			// Evaluación
			this->problem_->evaluate(children.getptr(0));
			this->problem_->evaluateConstraints(children.getptr(0));
			this->problem_->evaluate(children.getptr(1));
			this->problem_->evaluateConstraints(children.getptr(1));

			// Comparar cada hijo con el mejor global
			for (int h = 0; h < 2; h++) {


				bool maximization = this->problem_->getObjectivesType()[0] == Constantes::MAXIMIZATION;

				if (maximization && children.get(h).getObjective(0) > best->get(0).getObjective(0) && children.get(h).getNumberOfViolatedConstraints() == 0) {
					best->set(0, children.get(h));
					// <<< CAMBIO 2: Línea eliminada >>>
				}
				else if (!maximization && children.get(h).getObjective(0) < best->get(0).getObjective(0) && children.get(h).getNumberOfViolatedConstraints() == 0) {
					best->set(0, children.get(h));
				/*	std::wstring m2 = L"MEJORSO: \n";
					OutputDebugStringW(m2.c_str());*/
					// <<< CAMBIO 2: Línea eliminada >>>
				}
			}

			// Intentar reemplazar hijos en la población
			for (int h = 0; h < 2; h++) {


				for (int k = 0; k < pobSize; k++) {
					bool maximization = this->problem_->getObjectivesType()[0] == Constantes::MAXIMIZATION;

					if (maximization && children.get(h).getObjective(0) > pob->get(k).getObjective(0) && children.get(h).getNumberOfViolatedConstraints() == 0) {
						pob->set(k, children.get(h));
						break;
					}
					else if (!maximization && children.get(h).getObjective(0) < pob->get(k).getObjective(0) && children.get(h).getNumberOfViolatedConstraints() == 0) {
						pob->set(k, children.get(h));
						/*std::wstring m = L"MEJORO: \n";
						OutputDebugStringW(m.c_str());*/
						break;
					}
				}
			}
		}

		// <<< CAMBIO 3: Lógica de estancamiento corregida >>>
		// Obtenemos el fitness actual
		auto current_best_fitness = best->get(0).getObjective(0).L;

		if (current_best_fitness == best_fitness_anterior) {
			generacionesSinMejora++; // No hubo mejora
		}
		else {
			generacionesSinMejora = 0; // Hubo mejora, reiniciar contador
			best_fitness_anterior = current_best_fitness; // Actualizar el valor anterior
		}

		// Si se alcanza el tope, rompe el bucle y termina.
		/*if (generacionesSinMejora >= topeSinMejora) {
			break;
		}*/

		/*cout << best->get(0).getObjective(0) << endl;*/

		if (best->get(0).getObjective(0).L == 815 || best->get(0).getObjective(0).L < 815) {
			cout << "Óptimo encontrado en generación " << generation << endl;
			break;
			}

		
		//std::string ss = obtenerSolucionComoCadena(best->get(0));
		//OutputDebugStringA(ss.c_str());
		 
		 
		 
		cout << generation << " best: " << best->get(0).getObjective(0) << " sin mejora: " << generacionesSinMejora    << "        \r" << flush;
		
		++generation;



	}

	this->lastB_ = this->best;
	this->last_ = *pob;



}