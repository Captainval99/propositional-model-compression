#ifndef SRC_PROPAGATION_HEURISTICS_H
#define SRC_PROPAGATION_HEURISTICS_H

#include <deque>
#include <algorithm>
#include <math.h>
#include <set>
#include <queue>
#include <map>

#include "SATTypes.h"
#include "Heap.h"

//abstract base class
class Heuristic {

    protected:
        struct VarComparator {
            bool operator()(Var x, Var y) const {
                if (heuristicValues[x.id] == heuristicValues[y.id]) {
                    return x.id < y.id;
                }

                return heuristicValues[x.id] > heuristicValues[y.id]; 
            }
        };

        std::vector<Var> variables;
        bool dynamicHeuristic;
        Minisat::Heap<Var, VarComparator> variablesHeap = Minisat::Heap<Var, VarComparator>(VarComparator());
        std::map<unsigned int, bool> activeVariables;
        std::vector<Assignment> predictedAssignments;

    public:
        static std::map<unsigned int, double> heuristicValues;

        explicit Heuristic(std::vector<Var> variables, bool dynamicHeuristic) : variables(variables), dynamicHeuristic(dynamicHeuristic) {
        }

        virtual ~Heuristic() {}

        virtual void updateVariables(Cl* clause) = 0;

        Var getNextVar() {
            if (variablesHeap.empty()) {
                throw std::runtime_error("Error, the model is not satisfying!");
            }

            Var nextVar = variablesHeap.removeMin();

            activeVariables[nextVar.id] = false;

            return nextVar;
        }

        Assignment getPredictedAssignment(Var var) {
            return predictedAssignments[var.id - 1];
        }
};

class ParsingOrder: public Heuristic {
    public:
        ParsingOrder(std::vector<Var> variables) : Heuristic(variables, false) {
            for (Var var: variables) {
                //invert the id so that the smallest id gets assigned first because of max heap
                heuristicValues[var.id] = var.id * -1.0;
                variablesHeap.insert(var);

                //predict the assignment based on the occurence counts
                if (var.nrPosOcc >= var.nrNegOcc) {
                    predictedAssignments.push_back(Assignment::TRUE);
                } else {
                    predictedAssignments.push_back(Assignment::FALSE);
                }
            }
        }

        void updateVariables(Cl* clause) {}
};

class JeroslowWang: public Heuristic {
    private:
        std::vector<double> negHeuristicValues;
        std::vector<double> posHeuristicValues;

    public:
        explicit JeroslowWang(std::vector<Var> variables_, bool dynamic) : Heuristic(variables_, dynamic) {
            for (Var var: variables_) {
                double negHeuristicValue = 0;
                double posHeuristicValue = 0;

                //std::cout << "neg size: " << var.negOccList.size() << ", pos size: " << var.posOccList.size() << std::endl;

                for (Cl* clause: var.negOccList) {
                    if (clause->literals.size() > 0) {
                        double clauseSize = static_cast<double>(clause->literals.size());
                        negHeuristicValue += pow(2, -clauseSize);
                    }
                }

                negHeuristicValues.push_back(negHeuristicValue);

                for (Cl* clause: var.posOccList) {
                    if (clause->literals.size() > 0) {
                        double clauseSize = static_cast<double>(clause->literals.size());
                        posHeuristicValue += pow(2, -clauseSize);
                    }
                }

                posHeuristicValues.push_back(posHeuristicValue);

                //heuristicValues[var.id - 1] = heuristicValue;
                heuristicValues[var.id] = negHeuristicValue + posHeuristicValue;
                activeVariables[var.id] = true;
                variablesHeap.insert(var);

                //std::cout << "Var: " << var.id << ", heuristic value: " << heuristicValue << std::endl;

                //use the positive and negative heuristic values to predict the assignment
                if (posHeuristicValue >= negHeuristicValue) {
                    predictedAssignments.push_back(Assignment::TRUE);
                } else {
                    predictedAssignments.push_back(Assignment::FALSE);
                }

            }
        }

        void updateVariables(Cl* clause) {
            if (!dynamicHeuristic) {
                return;
            }

            //update all variables in the clause
            for (Lit lit: clause->literals) {
                Var& var = variables.at(lit.id - 1);

                //std::cout << "Update variable: " << var.id << std::endl;
                //remove the variable from the set and reinsert it to update the position
                if (activeVariables[var.id]) {
                    double clauseSize = static_cast<double>(clause->literals.size());
                    double updateValue = pow(2, -clauseSize);
                    
                    heuristicValues[var.id] -= updateValue;

                    variablesHeap.increase(var);

                    //update assignment prediction
                    if (lit.negative) {
                        negHeuristicValues[var.id - 1] -= updateValue;
                    } else {
                        posHeuristicValues[var.id - 1] -= updateValue;
                    }

                    if (posHeuristicValues[var.id - 1] >= negHeuristicValues[var.id - 1]) {
                        predictedAssignments[var.id - 1] = Assignment::TRUE;
                    } else {
                        predictedAssignments[var.id - 1] = Assignment::FALSE;
                    }
                }
            }
        }
};

class MomsFreeman: public Heuristic {
    private:
        double momsParameter;
        unsigned int minClauseLength;
        std::vector<Cl>& clauses;
        unsigned int nrMinClauses = 0;
        std::vector<unsigned int> posCounts;
        std::vector<unsigned int> negCounts;

        void calculateHeuristicValue(Var variable) {
            unsigned int posCount = 0;
            unsigned int negCount = 0;

            for (Cl* clause: variable.negOccList) {
                if (clause->literals.size() == minClauseLength) {
                    negCount += 1;
                }
            }

            negCounts.push_back(negCount);

            for (Cl* clause: variable.posOccList) {
                if (clause->literals.size() == minClauseLength) {
                    posCount += 1;
                }
            }

            posCounts.push_back(posCount);

            unsigned int heuristicValue = (posCount + negCount) * momsParameter + posCount * negCount;

            heuristicValues[variable.id] = heuristicValue;
        }

        void findMinClauseLength() {
            //determine the length of the shortest clause
            minClauseLength = clauses[0].literals.size();

            unsigned int i = 1;

            //find the first clause that is not satisfied
            while (minClauseLength == 0 && i < clauses.size()) {
                minClauseLength = clauses[i].literals.size();
                i +=1 ;
            }

            nrMinClauses = 1;

            while (i < clauses.size()) {
                unsigned int currentSize = clauses[i].literals.size();

                if (currentSize != 0 && currentSize < minClauseLength) {
                    minClauseLength = currentSize;
                    nrMinClauses = 1;
                } else if (currentSize == minClauseLength) {
                    nrMinClauses += 1;
                }

                i += 1;
            }
        }

        void setPredictedAssignment(Var var) {
            if (posCounts[var.id - 1] >= negCounts[var.id - 1]) {
                predictedAssignments[var.id - 1] = Assignment::TRUE;
            } else {
                predictedAssignments[var.id - 1] = Assignment::FALSE;
            }
        }

        public:
            explicit MomsFreeman(std::vector<Var> variables, std::vector<Cl>& clauses, bool dynamic, double parameter) : Heuristic(variables, dynamic), clauses(clauses) {
                momsParameter = std::pow(2, parameter);
                
                findMinClauseLength();
                
                for (Var var: variables) {
                    calculateHeuristicValue(var);
                    setPredictedAssignment(var);
                    activeVariables[var.id] = true;
                    variablesHeap.insert(var);
                    //std::cout << "Id: " << var.id << ", heuristics value: " << heuristicValues[var.id] << std::endl;
                }
            }

            void updateVariables(Cl* clause) {
                //is only executed if the heuristic is dynamic
                if (!dynamicHeuristic) {
                    return;
                }

                if ((clause->literals.size() == minClauseLength)) {
                    nrMinClauses -= 1;

                    for (Lit lit: clause->literals) {
                        Var& var = variables.at(lit.id - 1);

                        //update the heuristic value of all variables in the clause that are still in the heap
                        if (activeVariables[var.id]) {
                            std::vector<Cl*> occList;

                            if (lit.negative) {
                                occList = var.posOccList;
                                //update the counter for the assginment prediction
                                negCounts[var.id - 1] -= 1;
                            } else {
                                occList = var.negOccList;
                                posCounts[var.id - 1] -= 1;
                            }

                            unsigned int count = 0;

                            for (Cl* clause: occList) {
                                if (clause->literals.size() == minClauseLength) {
                                    count += 1;
                                }
                            }

                            heuristicValues[var.id] -= momsParameter - count; 

                            variablesHeap.increase(var);

                            //update the assignment prediction
                            setPredictedAssignment(var);
                        }


                    }
                }


                //std::cout << "minClauseOccs: " << nrMinClauses << std::endl;

                //if all clauses of minimum length are satisfied, all heuristic values have to be recalculated
                if (nrMinClauses == 0) {
                    //clear the clause so that it won't count as minimum clause as it is already satisfied
                    clause->literals.clear();

                    findMinClauseLength();

                    variablesHeap.clear();
                    predictedAssignments.clear();
                    posCounts.clear();
                    negCounts.clear();

                    for (Var var: variables) {
                        if (activeVariables[var.id]) {
                            calculateHeuristicValue(var);
                            variablesHeap.insert(var);
                        }   
                    }
                }
            }
};

#endif