#ifndef SRC_PROPAGATION_PROPAGATION_H_
#define SRC_PROPAGATION_PROPAGATION_H_

#include <deque>
#include <unordered_set>


#include "SATTypes.h"

namespace Propagation {
    
    void propagate(std::vector<Cl>& clauses, std::vector<Var>& variables, Var propagationVar) {
        //queue for variables that are propagated
        std::deque<Var> propagationQueue = {propagationVar};
        std::unordered_set<unsigned int> propagationQueueIds = {propagationVar.id}; 

        while (!propagationQueue.empty()) {
            Var var = propagationQueue.front();
            propagationQueue.pop_front();
            propagationQueueIds.erase(var.id);

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

            //iterate over the clauses that are satisfied and clear them
            for (Cl* clause: satOccList) {
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
                            //check if variable is already in queue
                            if (propagationQueueIds.count(unitVar.id)) {
                                break;
                            }

                            if(clause->nrNeg == 1) {
                                unitVar.state = Assignment::FALSE;
                            } else {
                                unitVar.state = Assignment::TRUE;
                            }

                            propagationQueue.push_back(unitVar);
                            propagationQueueIds.insert(unitVar.id);
                            break;
                        }
                    }
                }
            }
        }
    }

}

#endif