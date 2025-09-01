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

#include <tools/operators/interval/PolynomialMutation.h>

#include "miGenetico.h"

#include "SwapMutation.h"
#include "ScrambleMutation.h"


#include <iostream>
#include <tools/AlgorithmBuilder.h>
 
#include<tools/builders/ProblemBuilder.h>
#include <tools/builders/MutationBuilder.h>
#include <tools/builders/ImprovementBuilder.h>
#include <tools/builders/RepairBuilder.h>
 
#include "OxCrossover.h"
#include "CVRP_Repair.h"
#include "LocalSearch.h"
#include <chrono>
#include <iostream>
#include <string>
#include <random>
using namespace std;
#include <cstdlib>  // rand, srand

#include <fstream>
#include <tools/Builders/HyperheuristicBuilder.h>


int main()
{
    std::mt19937 rng(310);  // Semilla fija
    srand(310);  // Semilla fija
    RandomNumber* rnd = RandomNumber::getInstance();
    rnd->setSeed(310);


    for (int i = 0; i < 1; i++)
    {


        auto start = std::chrono::high_resolution_clock::now(); // Inicio del tiempo

        cout << i << endl;
        ImprovementBuilder::add("LocalSearch", new LocalSearch());
        CrossoverBuilder::add("OxCrossover", new OxCrossover());
        MutationBuilder::add("SwapMutation", new SwapMutation());
        MutationBuilder::add("ScrambleMutation", new ScrambleMutation());


        AlgorithmBuilder::add("miGenetico", new miGenetico());
        RepairBuilder::add("CVRP_Repair", new CVRP_Repair());
        Algorithm* alg = AlgorithmBuilder::execute("_INPUT-CVRP/config_GA.txt");

        alg->execute();
        std::string nombreArchivo = "Salida-CVRP22" + std::to_string(i) + ".txt";
        ofstream out(nombreArchivo);
        SolutionSet res = alg->getSolutionSet();

        out << res;
        out.close();

        cout << *alg->getLastB() << endl;
        cout << alg->getSolutionSet() << endl;

        // Liberar memoria manualmente
       /* delete alg;*/


        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        std::cout << "Iteración " << i << " tiempo: " << duration.count() << " segundos" << std::endl;
    }



 /*   Hyperheuristic* a = HyperheuristicBuilder::execute("HH_CVRP-N01-I01-CVRP_Simple.txt");
    a->execute();

    ofstream out("Salida-HH_CVRP.txt");
    SolutionSet res = a->getSolutionSet();

    out << res;
    out.close();*/


}

 