#ifndef SRC_PROPAGATION_PROPAGATION_H_
#define RC_PROPAGATION_PROPAGATION_H_

#include <deque>


#include "SATTypes.h"

namespace Propagation {
    
    int propagate(std::vector<Cl>& clauses, std::vector<Var> variables, Var propagationVar) {
        //queue for variables that are propagated
        std::deque<Var> propagationQueue = {propagationVar};
        //variable to count the number of assignments that are made
        int nrAssign = 0;

        while (!propagationQueue.empty()) {
            Var var = propagationQueue.front();
            propagationQueue.pop_front();

            //choose the right occurence list
            std::vector<Cl*> occList;

            if(var.state == TRUE) {
                occList = var.negOccList;
            } else if (var.state == FALSE) {
                occList = var.posOccList;
            }
            //TODO: Error handling when state == OPEN

            //iterate over occurence list and update counters
            for (Cl* clause: occList) {
                if (var.state == TRUE) {
                    clause->nrNeg -= 1;
                } else {
                    clause->nrPos -= 1;
                }

                //check for unit clause
                if ((clause->nrNeg == 1 && clause->nrPos == 0) || (clause->nrNeg == 0 && clause->nrPos == 1)) {
                    nrAssign += 1;
                    for (Lit lit: clause->literals) {
                        Var& unitVar = variables.at(lit.id -1);
                        if (unitVar.state == OPEN) {
                            if(clause->nrNeg == 1) {
                                unitVar.state = Assignment::FALSE;
                            } else {
                                unitVar.state = Assignment::TRUE;
                            }

                            propagationQueue.push_back(unitVar);
                            break;
                        }
                    }
                }
            }
        }
        return nrAssign;
    }

}

#endif