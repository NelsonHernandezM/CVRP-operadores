#include "CombinationCrossover.h"

#include "tools/operators/interval/MOSADOperators/BRBAXCrossover.h"
#include "tools/operators/interval/MOSADOperators/OXCrossover.h"
#include "tools/operators/interval/MOSADOperators/PereiraCrossover.h"

#include "problems/CVRP.h"
#include "solutions/Solution.h"
#include "solutions/SolutionSet.h"
#include "tools/RandomNumber.h"
#include "miCVRP_Repair.h"
#include <vector>
#include <numeric>
#include <algorithm>
#include <utility>
#include <random>

void imprimirSolucionComb(Solution s) {
	Interval* vars = s.getDecisionVariables();
	for (int i = 0; i < s.getNumVariables(); i++) {
		std::cout << vars[i].L << " ";
	}
	std::cout << std::endl;
}


void CombinationCrossover::execute(SolutionSet parents, SolutionSet children) {

	RandomNumber* rnd = RandomNumber::getInstance();

	//cout << "Padre 1: ";
	//imprimirSolucionComb(parents.get(0));
	//cout << "Padre 2: ";
	//imprimirSolucionComb(parents.get(1));

	int aleatorio = rnd->nextInt(2);
	switch (aleatorio)
	{
	case 0: {
		//cout << "BRBAX" << endl;
		BRBAXCrossover brbax;
		brbax.execute(parents, children);
		break;
	}
	case 1: {
	/*	cout << "OXCrossover" << endl;*/
		OXCrossover ox;
		ox.execute(parents, children);
		break;
	}
	case 2: {
	/*	cout << "OXCrossover" << endl;*/
		PereiraCrossover pc;
		pc.execute(parents, children);
		break;
	}
	}
	//cout << "hijo 1: ";
	//imprimirSolucionComb(children.get(0));
	//cout << "hijo 2: ";
	//imprimirSolucionComb(children.get(1));


}

void CombinationCrossover::initialize(Requirements* config) {





}