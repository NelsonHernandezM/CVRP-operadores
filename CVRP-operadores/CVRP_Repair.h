#pragma once

#include "tools/builders/RepairBuilder.h"
#include "../WindowsRequirements.h"
#include <problems/CVRP.h>

class CVRP_Repair : public RepairOperator
{


public:
	virtual void execute(Solution sol);
	virtual void initialize(Requirements* config);


};

