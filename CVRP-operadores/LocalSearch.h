#pragma once


#include "../WindowsRequirements.h"
#include "./tools/operators/interval/ImprovementOperator.h"

class  LocalSearch : public ImprovementOperator {




public:
	LocalSearch();



	void execute(Solution solucion);

	void initialize(Requirements* config) override;
};

