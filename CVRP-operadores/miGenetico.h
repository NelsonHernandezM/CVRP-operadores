#pragma once
#pragma once

#include "metaheuristics/Algorithm.h"
#include "tools/Requirements.h"
#include "tools/builders/MutationBuilder.h"
#include "tools/builders/SelectionBuilder.h"
#include "tools/builders/CrossoverBuilder.h"
#include "tools/builders/RepairBuilder.h"
#include "tools/builders/ProblemBuilder.h"
#include "tools/builders/ImprovementBuilder.h"
#include "solutions/SolutionSet.h"


class miGenetico : public Algorithm
{
	int MAX_GENERATIONS;
	int N;
	Interval OPTIMO;


	SolutionSet* best;
	SolutionSet* pob;

	MutationOperator* mo;
	CrossoverOperator* co;
	SelectionOperator* so;
	ImprovementOperator* improvement;
	RepairOperator* reparacion;

public:
	miGenetico();
	//~miGenetico();
	void execute() override;
	void initialize() override;
	void initialize(Requirements* req) override;

};

