#include "miScrambleMutation.h"
#include "tools/RandomNumber.h"
#include "solutions/SolutionSet.h"
#include "problems/Problem.h"
#include "tools/Requirements.h"
#include "problems/CVRP.h"
#include <vector>
#include <utility>
#include <algorithm>
#include <random>

void miScrambleMutation::initialize(Requirements* config) {
    config->addValue("#miScrambleMutation-probability", Constantes::DOUBLE);
    this->param = *(config->load());
    mutationProbability_ = this->param.get("#miScrambleMutation-probability").getDouble();
}

void miScrambleMutation::execute(Solution y) {
    RandomNumber* rnd = RandomNumber::getInstance();

    if (rnd->nextDouble() > mutationProbability_) {
        return;
    }

    std::vector<int> clientes;
    std::vector<int> indices_clientes;
    clientes.reserve(y.getNumVariables());
    indices_clientes.reserve(y.getNumVariables());

    auto* problema = dynamic_cast<CVRP*>(y.getProblem());

    for (int i = 0; i < y.getNumVariables(); ++i) {
        int value = y.getVariableValue(i).L;
 
        if (value != -1 && problema && !problema->isDepot(value)) {
            clientes.push_back(value);
            indices_clientes.push_back(i);
        }
    }

    if (clientes.size() < 2) {
        return;
    }

    int numClientes = clientes.size()-1;

    int pos1 = rnd->nextInt(numClientes);
    int pos2 = rnd->nextInt(numClientes);

    if (pos1 > pos2) {
        std::swap(pos1, pos2);
    }

    if (pos1 < pos2) {
        std::mt19937 g(rnd->nextInt(1000000));
        std::shuffle(clientes.begin() + pos1, clientes.begin() + pos2 + 1, g);
    }

    for (size_t i = 0; i < clientes.size(); ++i) {
        y.setVariableValue(indices_clientes[i], clientes[i]);
    }
}