#include "miGenetico.h"
#include <string>
#include <algorithm>
#include "utility"
#include <vector>
 

 
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

	this->mo = (MutationOperator*)MutationBuilder::execute(req);
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
	cout << " " << endl;


}
  

void miGenetico::execute() {
	int generacionesSinMejora = 0;
	int topeSinMejora = 100;

	int pobSize = 2 * this->N;
	int generation = 0;
	bool first = true;
	Solution nueva(this->problem_);

	best = new SolutionSet(1, 1, this->problem_);
	SolutionSet parents(2, 2, this->problem_);
	SolutionSet children(2, 2, this->problem_);
 ////
	int best_fitness_anterior = 0;

	// Generar población inicial
	for (int i = 0; i < pobSize; i++) {
		nueva = this->problem_->generateRandomSolution();
		this->problem_->evaluate(&nueva);
		this->problem_->evaluateConstraints(&nueva);

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

	// Bucle principal
	while (generation < this->MAX_GENERATIONS) {
		cout << generation<<"\r";
		for (int i = 0; i < this->N; i++) {
			// Selección
			parents.set(0, this->so->execute(*pob));
			parents.set(1, this->so->execute(*pob));

			// Cruza
			this->co->execute(parents, children);

			// Mutación
			this->mo->execute(children.get(0));
			this->mo->execute(children.get(1));



			////// Reparación
			this->reparacion->execute(children.get(0));
			this->reparacion->execute(children.get(1));

			// Evaluación
			this->problem_->evaluate(children.getptr(0));
			this->problem_->evaluateConstraints(children.getptr(0));
			this->problem_->evaluate(children.getptr(1));
			this->problem_->evaluateConstraints(children.getptr(1));

			RandomNumber* rnd = RandomNumber::getInstance();

			
			 
				// Si el número es menor a 30 (30% de probabilidad), aplica la mejora.
				this->improvement->execute(children.get(0));
				this->improvement->execute(children.get(1));

		 
			 



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
					  best_fitness_anterior = 0.0;
				}
				else if (!maximization && children.get(h).getObjective(0) < best->get(0).getObjective(0) && children.get(h).getNumberOfViolatedConstraints() == 0) {
					best->set(0, children.get(h));
					std::wstring m2 = L"MEJORSO: \n";
					OutputDebugStringW(m2.c_str());
					  best_fitness_anterior = 0.0;
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
						std::wstring m = L"MEJORO: \n";
						OutputDebugStringW(m.c_str());
						break;
					}
				}
			}
		}

		// >>> 3. COMPRUEBA EL ESTANCAMIENTO AL FINAL DE LA GENERACIÓN <<<
		if (best->get(0).getObjective(0).L == best_fitness_anterior) {
			generacionesSinMejora++; // Si no cambió, incrementa el contador
		}
		else {
			generacionesSinMejora = 0; // Si mejoró, reinicia el contador
		}
		// Si se alcanza el tope, rompe el bucle y termina.
		if (generacionesSinMejora >= topeSinMejora) {
			break;
		}

		/*cout << best->get(0).getObjective(0) << endl;*/

	/*	if (best->get(0).getObjective(0).L == 784) {
			cout << "Óptimo encontrado en generación " << generation << endl;
			break;
			}*/

		++generation;

 

	}

	this->lastB_ = this->best;
	this->last_ = *pob;

	

}
