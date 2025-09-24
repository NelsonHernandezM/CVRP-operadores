// CVRP-operadores.cpp : Este archivo contiene la función "main". La ejecución del programa comienza y termina ahí.
//

#include <iostream>
#include <problems/CVRP.h>
using namespace std;
#include <cstdlib>  // rand, srand
#include <random>
#include <tools/AlgorithmBuilder.h>
#include <tools/builders/MutationBuilder.h>

#include <tools/builders/CrossoverBuilder.h>
#include <chrono>
#include <iostream>
#include <tools/operators/interval/SBXCrossover.h>
#include "Search2OPT.h"
#include <tools/operators/interval/PolynomialMutation.h>

#include "miGenetico.h"
#include "EdgeRecombinationCrossover.h"
#include "miSwapMutation.h"
#include "CycleCrossover.h"

#include "AlternatingEdgesCrossover.h"

#include "miScrambleMutation.h"
#include "CombinationCrossover.h"
#include "PartiallyMappedCrossover.h"
#include <iostream>
#include <tools/AlgorithmBuilder.h>

#include<tools/builders/ProblemBuilder.h>
#include <tools/builders/MutationBuilder.h>
#include <tools/builders/ImprovementBuilder.h>
#include <tools/builders/RepairBuilder.h>

#include "OxCrossover.h"
#include "HeuristicCrossover.h"
//#include "miCVRP_Repair.h"
#include "LocalSearch.h"
#include <chrono>
#include <iostream>
#include <string>
#include <random>
using namespace std;
#include <cstdlib>  // rand, srand

#include <fstream>
#include <tools/Builders/HyperheuristicBuilder.h>
#include "miBRBAX.h"

int main()
{



	RandomNumber* rnd2 = RandomNumber::getInstance();

	
		//9089
	for (int i = 0; i < 100; i++)
	{
		int semilla = rnd2->nextInt(10000000);
		if (i > 0) { semilla = rnd2->nextInt(100000); }
		else {
			semilla = 17548;
		}cout << "semilla: " << semilla << endl;
		std::mt19937 rng(semilla);  // Semilla fija
		srand(semilla);  // Semilla fija
		RandomNumber* rnd = RandomNumber::getInstance();
		rnd->setSeed(semilla);

		auto start = std::chrono::high_resolution_clock::now(); // Inicio del tiempo

		cout << i << endl;
		ImprovementBuilder::add("LocalSearch", new LocalSearch());
		ImprovementBuilder::add("Search2OPT", new Search2OPT());
		
		CrossoverBuilder::add("miBRBAX", new miBRBAX());
		//CrossoverBuilder::add("CombinationCrossover", new CombinationCrossover());
		//CrossoverBuilder::add("OxCrossover", new OxCrossover());
		//CrossoverBuilder::add("PartiallyMappedCrossover", new PartiallyMappedCrossover());
		//CrossoverBuilder::add("EdgeRecombinationCrossover", new EdgeRecombinationCrossover());
		//CrossoverBuilder::add("CycleCrossover", new CycleCrossover());
		//CrossoverBuilder::add("AlternatingEdgesCrossover", new AlternatingEdgesCrossover());
		//CrossoverBuilder::add("HeuristicCrossover", new HeuristicCrossover());
		
		MutationBuilder::add("miSwapMutation", new miSwapMutation());
		MutationBuilder::add("miScrambleMutation", new miScrambleMutation());



		AlgorithmBuilder::add("miGenetico", new miGenetico());
		/*      RepairBuilder::add("miCVRP_Repair", new miCVRP_Repair());*/
		Algorithm* alg = AlgorithmBuilder::execute("_INPUT-CVRP/config_GA.txt");

		alg->execute();
		std::string nombreArchivo = "Salida-CVRP .txt";
		ofstream out(nombreArchivo);
		SolutionSet res = alg->getSolutionSet();

		out << res;
		out.close();

		
		cout << alg->getSolutionSet() << endl;
		cout << *alg->getLastB() << endl;

		// Liberar memoria manualmente
	   /* delete alg;*/


		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> duration = end - start;
		std::cout << "Iteración " << i << " tiempo: " << duration.count() << " segundos" << " minutos: " << duration.count() / 60 << std::endl;
	}



	/*   Hyperheuristic* a = HyperheuristicBuilder::execute("HH_CVRP-N01-I01-CVRP_Simple.txt");
	   a->execute();

	   ofstream out("Salida-HH_CVRP.txt");
	   SolutionSet res = a->getSolutionSet();

	   out << res;
	   out.close();*/


}

