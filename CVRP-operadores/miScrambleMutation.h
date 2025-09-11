#pragma once
#include "tools/builders/MutationBuilder.h"


#include "../WindowsRequirements.h"

class miScrambleMutation : public MutationOperator {
 
private:
    double mutationProbability_;    //Input [PolynomialMutation-probability] probability of mutating

public:

    void execute(Solution ind) override;
    void initialize(Requirements* config) override;
};



 