// 
//#include "tools/builders/CrossoverBuilder.h"
//#include "tools/RandomNumber.h"
//#include "solutions/SolutionSet.h"
//#include "problems/Problem.h"
//#include "tools/Requirements.h"
//#include "../WindowsRequirements.h"
//#include "utility"
//#include <vector>
//#include <cmath>
//#include "../WindowsRequirements.h"
//
//class   HeuristicCrossover : public CrossoverOperator {
//public:
//  
//    enum HeuristicMode { GREEDY, RANDOM, PROBABILISTIC, RANDOM_CHOICE};
//    HeuristicCrossover(HeuristicMode mode = GREEDY) : mode_(mode) {}
//
//    void setMode(HeuristicMode new_mode);
//    void initialize(HeuristicMode mode);
//
//    void execute(SolutionSet parents, SolutionSet children) override;
//    void initialize(Requirements* config) override;
//private:
//    HeuristicMode mode_;
//
//
//
//};