#pragma once


#include "../WindowsRequirements.h"
#include "./tools/operators/interval/ImprovementOperator.h"
#include "problems/CVRP.h"

class  LocalSearch : public ImprovementOperator {

private:
	

public:
	LocalSearch();
	CVRP* problem_;
	SolutionSet seto;



	void execute(Solution solucion);

	void initialize(Requirements* config) override;
};

