#ifndef SRC_PROPAGATION_HEURISTICS_H
#define SRC_PROPAGATION_HEURISTICS_H

#include <deque>
#include <algorithm>
#include <math.h>
#include <set>

#include "SATTypes.h"

//abstract base class
class Heuristic {

    protected:
        std::deque<Var> variables;
        bool dynamicHeuristic;
        virtual void sortVariables() = 0;

    public:
        explicit Heuristic(std::deque<Var> variables, bool staticHeuristic) : variables(variables), dynamicHeuristic(staticHeuristic) {
        }

        virtual ~Heuristic() {}

        Var getNextVar() {
            if (variables.size() == 0) {
                throw std::runtime_error("Error, the model is not satisfying!");
            }

            Var nextVar = variables.front();
            variables.pop_front();
            return nextVar;
        }

        void updateHeuristic() {
            //if heuristic is static it is not necessary to recalculate the heuristic
            if (dynamicHeuristic) {
                sortVariables();
            }
        }
};

class ParsingOrder: public Heuristic {
    public:
        ParsingOrder(std::deque<Var> variables) : Heuristic(variables, false) {}

        //the list does not get sorted so the method doesn't have to be implemented
        void sortVariables() {} 
};

class JeroslowWang: public Heuristic {
    private:
        static bool compare (const Var variable1, const Var variable2) {
            //std::cout << "variable1: " << variable1.id << ", variable2: " << variable2.id << std::endl;

            double j1 = 0;
            double j2 = 0;

            for (Cl* clause: variable1.negOccList) {
                double clauseSize = static_cast<double>(clause->literals.size());
                j1 += pow(2, -clauseSize);
            }

            for (Cl* clause: variable1.posOccList) {
                double clauseSize = static_cast<double>(clause->literals.size());
                j1 += pow(2, -clauseSize);
            }

            for (Cl* clause: variable2.negOccList) {
                double clauseSize = static_cast<double>(clause->literals.size());
                j2 += pow(2, -clauseSize);
            }

            for (Cl* clause: variable2.posOccList) {
                double clauseSize = static_cast<double>(clause->literals.size());
                j2 += pow(2, -clauseSize);
            }

            if (j1 == j2) {
                return variable1.id < variable2.id;
            }

            return j1 > j2;
        }

    public:
        explicit JeroslowWang(std::deque<Var> variables) : Heuristic(variables, false) {
            sortVariables();
        }

        void sortVariables() {
            std::sort(variables.begin(), variables.end(), compare);
        }
};

class MomsFreeman: public Heuristic {
    private:
        static constexpr double MOMS_PARAMETER = 10.0;
        std::vector<Cl>& clauses;
        std::deque<Var> allVariables;


        static bool compare(const Var variable1, const Var variable2) {
            unsigned int posCount1 = 0;
            unsigned int negCount1 = 0;
            unsigned int posCount2 = 0;
            unsigned int negCount2 = 0;

            for (Cl* clause: variable1.negOccList) {
                if (clause->literals.size() == minClauseLength) {
                    negCount1 += 1;
                }
            }

            for (Cl* clause: variable1.posOccList) {
                if (clause->literals.size() == minClauseLength) {
                    posCount1 += 1;
                }
            }

            for (Cl* clause: variable2.negOccList) {
                if (clause->literals.size() == minClauseLength) {
                    negCount2 += 1;
                }
            }

            for (Cl* clause: variable2.posOccList) {
                if (clause->literals.size() == minClauseLength) {
                    posCount2 += 1;
                }
            }

            unsigned int momsValue1 = (posCount1 + negCount1) * std::pow(2, MOMS_PARAMETER) + posCount1 * negCount1;
            unsigned int momsValue2 = (posCount2 + negCount2) * std::pow(2, MOMS_PARAMETER) + posCount2 * negCount2;

            if (momsValue1 == momsValue2) {
                return variable1.id < variable2.id;
            }

            return momsValue1 > momsValue2;
        }

        public:
            static unsigned int minClauseLength;

            explicit MomsFreeman(std::deque<Var> variables, std::vector<Cl>& clauses, bool dynamic) : Heuristic(variables, dynamic), clauses(clauses), allVariables(variables) {
                sortVariables();
            }

            void sortVariables() {
                //set for all variables that are contained in the shortest clauses
                std::set<Var> currentVariables;

                //determine the length of the shortest clause
                minClauseLength = clauses[0].literals.size();

                unsigned int i = 1;

                //find the first clause that is not satisfied
                while (minClauseLength == 0 && i < clauses.size()) {
                    minClauseLength = clauses[i].literals.size();
                    i +=1 ;
                }

                //add variables of first clause to set if set to dynamic
                if (dynamicHeuristic) {
                    for (Lit lit: clauses[i - 1].literals) {
                        Var var = allVariables[lit.id - 1];
                        currentVariables.insert(var);
                    }
                }
                
                

                while (i < clauses.size()) {
                    unsigned int currentSize = clauses[i].literals.size();

                    if (currentSize != 0 && currentSize < minClauseLength) {
                        minClauseLength = currentSize;
                        currentVariables.clear();
                    }

                    //insert variables into set if clause is of mininmal length if set to dynamic
                    if (dynamicHeuristic && currentSize == minClauseLength) {
                        for (Lit lit: clauses[i].literals) {
                            Var var = allVariables[lit.id - 1];
                            currentVariables.insert(var);
                        }
                    }

                    i += 1;
                }

                //std::cout << "min length heuristic: " << minClauseLength << std::endl;
                if (dynamicHeuristic) {
                    variables = std::deque(currentVariables.begin(), currentVariables.end());
                }

                std::sort(variables.begin(), variables.end(), compare);

                //std::sort(allVariablesSorted.begin(), allVariablesSorted.end(), compare);
            }
};

#endif