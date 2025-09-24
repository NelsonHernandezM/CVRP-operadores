#pragma once


#include "../WindowsRequirements.h"
#include "./tools/operators/interval/ImprovementOperator.h"
#include "problems/CVRP.h"

class  Search2OPT : public ImprovementOperator {

private:


public:
	Search2OPT();
	CVRP* problem_;
	SolutionSet seto;



	void execute(Solution solucion);

	void initialize(Requirements* config) override;
};

