#ifndef SRC_PROPAGATION_PROPAGATION_H_
#define SRC_PROPAGATION_PROPAGATION_H_

#include <deque>
#include <unordered_set>


#include "SATTypes.h"
#include "Heuristics.h"

namespace Propagation {
    
    void propagate(std::vector<Cl>& clauses, std::vector<Var>& variables, std::vector<Var>& trail, int& head, Heuristic* heuristic) {
        while (head < trail.size()) {
            Var var = trail[head];
            head++;

            //choose the right occurence list
            std::vector<Cl*> occList;
            std::vector<Cl*> satOccList;

            if(var.state == TRUE) {
                occList = var.negOccList;
                satOccList = var.posOccList;
            } else if (var.state == FALSE) {
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

                if (var.state == TRUE) {
                    clause->nrNeg -= 1;
                } else {
                    clause->nrPos -= 1;
                }

                //check for unit clause
                if ((clause->nrNeg == 1 && clause->nrPos == 0) || (clause->nrNeg == 0 && clause->nrPos == 1)) {
                    for (Lit lit: clause->literals) {
                        Var& unitVar = variables.at(lit.id -1);

                        if (unitVar.state == Assignment::OPEN) {
                            if(clause->nrNeg == 1) {
                                unitVar.state = Assignment::FALSE;
                            } else {
                                unitVar.state = Assignment::TRUE;
                            }

                            trail.push_back(unitVar);
                            break;
                        }
                    }
                }
            }
        }
    }

}

#endif