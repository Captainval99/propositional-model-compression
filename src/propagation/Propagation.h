#ifndef SRC_PROPAGATION_PROPAGATION_H_
#define SRC_PROPAGATION_PROPAGATION_H_

#include <deque>
#include <unordered_set>


#include "SATTypes.h"
#include "Heuristics.h"

namespace Propagation {
    
    void propagate(std::vector<Cl>& clauses, std::vector<Var>& variables, std::vector<unsigned int>& trail, int& head, Heuristic* heuristic, std::vector<Assignment>& values) {
        while (head < trail.size()) {
            unsigned int varId = trail[head];
            head++;

            //choose the right occurence list
            std::vector<Cl*> occList;
            std::vector<Cl*> satOccList;

            Var var = variables[varId - 1];

            if(values[varId - 1] == TRUE) {
                occList = var.negOccList;
                satOccList = var.posOccList;
            } else if (values[varId - 1] == FALSE) {
                occList = var.posOccList;
                satOccList = var.negOccList;
            } else {
                throw std::runtime_error("Variable that is propagated has on value assigned.");
            }

            //std::cout << "Propagated variable: " << var.id << ", state: " << var.state << std::endl;

            //iterate over the clauses that are satisfied and clear them and update the counters and the heuristic
            for (Cl* clause: satOccList) {
                for (Lit lit: clause->literals) {
                    Var& satVar = variables.at(lit.id - 1);

                    if(lit.negative) {
                        satVar.nrNegOcc -= 1;
                    } else {
                        satVar.nrPosOcc -= 1;
                    }
                }

                heuristic->updateVariables(clause);

                clause->literals.clear();
            }

            //iterate over occurence list and update counters
            for (Cl* clause: occList) {
                //check if the clause is already satisfied
                if (clause->literals.size() == 0) {
                    continue;
                }

                clause->nrUnasignedVars -= 1;

                //check for unit clause
                if (clause->nrUnasignedVars == 1) {
                    for (Lit lit: clause->literals) {
                        if (values[lit.id - 1] == Assignment::OPEN) {
                            if(lit.negative) {
                                values[lit.id - 1] = Assignment::FALSE;
                            } else {
                                values[lit.id - 1] = Assignment::TRUE;
                            }

                            trail.push_back(lit.id);
                            break;
                        }
                    }
                }
            }
        }
    }

}

#endif