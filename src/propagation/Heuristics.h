#ifndef SRC_PROPAGATION_HEURISTICS_H
#define SRC_PROPAGATION_HEURISTICS_H

#include <deque>
#include <algorithm>
#include <math.h>
#include <set>
#include <queue>
#include <map>

#include "SATTypes.h"

//abstract base class
class Heuristic {

    protected:
        std::deque<Var> variables;
        bool dynamicHeuristic;
        virtual void sortVariables() = 0;
        std::set<Var, std::function<bool(Var, Var)>> variablesSet;
        std::map<unsigned int, bool> activeVariables;


    public:
        explicit Heuristic(std::deque<Var> variables, bool dynamicHeuristic, std::function<bool(const Var, const Var)> compare) : variables(variables), dynamicHeuristic(dynamicHeuristic) {
            variablesSet = std::set<Var, std::function<bool(Var, Var)>>(compare);
        }

        virtual ~Heuristic() {}

        virtual void updateVariable(Var var, Cl* clause) = 0;

        Var getNextVar() {
            if (variablesSet.size() == 0) {
                throw std::runtime_error("Error, the model is not satisfying!");
            }

            Var nextVar = *(variablesSet.begin());
            variablesSet.erase(nextVar);
            activeVariables[nextVar.id] = false;
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
        ParsingOrder(std::deque<Var> variables) : Heuristic(variables, false, NULL) {}

        //the list does not get sorted so the method doesn't have to be implemented
        void sortVariables() {} 
};

class JeroslowWang: public Heuristic {
    protected:
        static bool compare (const Var variable1, const Var variable2) {
            double heuristicValue1 = heuristicValues[variable1.id - 1];
            double heuristicValue2 = heuristicValues[variable2.id - 1];

            //std::cout << "variable1: " << variable1.id << ", variable2: " << variable2.id << ", value1: " << heuristicValue1 << ", value2: " << heuristicValue2 << std::endl;

            if (heuristicValue1 == heuristicValue2) {
                return variable1.id < variable2.id;
            }

            return heuristicValue1 > heuristicValue2;
        }

    public:
        static std::vector<double> heuristicValues; 

        explicit JeroslowWang(std::deque<Var> variables_, bool dynamic) : Heuristic(variables_, dynamic, &compare) {
            for (Var var: variables_) {
                double heuristicValue = 0;

                //std::cout << "neg size: " << var.negOccList.size() << ", pos size: " << var.posOccList.size() << std::endl;

                for (Cl* clause: var.negOccList) {
                    if (clause->literals.size() > 0) {
                        double clauseSize = static_cast<double>(clause->literals.size());
                        heuristicValue += pow(2, -clauseSize);
                    }
                }

                for (Cl* clause: var.posOccList) {
                    if (clause->literals.size() > 0) {
                        double clauseSize = static_cast<double>(clause->literals.size());
                        heuristicValue += pow(2, -clauseSize);
                    }
                }

                //heuristicValues[var.id - 1] = heuristicValue;
                heuristicValues.push_back(heuristicValue);
                activeVariables[var.id] = true;

                //std::cout << "Var: " << var.id << ", heuristic value: " << heuristicValue << std::endl;
            }

            for (Var var: variables_) {
                variablesSet.insert(var);
            }
        }

        void sortVariables() {
            std::sort(variables.begin(), variables.end(), compare);
        }

        void updateVariable(Var var, Cl* clause) {
            if (dynamicHeuristic) {
                //std::cout << "Update variable: " << var.id << std::endl;
                //remove the variable from the set and reinsert it to update the position
                if (activeVariables[var.id]) {
                    variablesSet.erase(var);

                    double clauseSize = static_cast<double>(clause->literals.size());
                    heuristicValues[var.id - 1] -= pow(2, -clauseSize);

                    variablesSet.insert(var);
                }
                
            }
        }
};

class MomsFreeman: public Heuristic {
    private:
        static constexpr double MOMS_PARAMETER = 10.0;
        std::vector<Cl>& clauses;
        std::vector<bool> currentVariables;
        std::deque<Var> allVariables;

        void calculateHeuristicValue(Var variable) {
            unsigned int posCount = 0;
            unsigned int negCount = 0;

            for (Cl* clause: variable.negOccList) {
                if (clause->literals.size() == minClauseLength) {
                    negCount += 1;
                }
            }

            for (Cl* clause: variable.posOccList) {
                if (clause->literals.size() == minClauseLength) {
                    posCount += 1;
                }
            }

            unsigned int heuristicValue = (posCount + negCount) * std::pow(2, MOMS_PARAMETER) + posCount * negCount;

            heuristicValues[variable.id - 1] = heuristicValue;

        }

        static bool compare(const Var variable1, const Var variable2) {
            unsigned int momsValue1 = heuristicValues[variable1.id - 1];
            unsigned int momsValue2 = heuristicValues[variable2.id - 1];

            if (momsValue1 == momsValue2) {
                return variable1.id < variable2.id;
            }

            return momsValue1 > momsValue2;
        }

        public:
            static unsigned int minClauseLength;
            static std::vector<unsigned int> heuristicValues; 

            explicit MomsFreeman(std::deque<Var> variables, std::vector<Cl>& clauses, bool dynamic) : Heuristic(std::deque<Var>(), dynamic, &compare), clauses(clauses), allVariables(variables) {
                heuristicValues = std::vector<unsigned int>(variables.size(), 0);
                currentVariables = std::vector<bool>(variables.size(), false);
                
                sortVariables();
            }

            void sortVariables() {
                std::cout << "Size: " << variables.size() << std::endl;
                //check if there are still variables that are still unasigned in the current variables list
                unsigned int i = 0;
                while (i < variables.size()) {
                    Var var = variables[i];
                    calculateHeuristicValue(var);
                    //heuristic value is 0 when the variable isn't contained in any clauses with minimal size
                    if (heuristicValues[var.id - 1] == 0) {
                        variables.erase(variables.begin() + i);
                        currentVariables[var.id - 1] = false;
                    } else {
                        i += 1;
                    }
                }
                //std::cout << "variables empty: " << variables.empty() << currentVariables.size() << std::endl;  

                //if there are no variables left in the set the minimum length and heuristic values have to be recalculated
                if (variables.empty()) {
                    //determine the length of the shortest clause
                    minClauseLength = clauses[0].literals.size();

                    unsigned int i = 1;

                    //find the first clause that is not satisfied
                    while (minClauseLength == 0 && i < clauses.size()) {
                        minClauseLength = clauses[i].literals.size();
                        i +=1 ;
                    }

                    //add variables of first clause to set 
                    for (Lit lit: clauses[i - 1].literals) {
                        Var var = allVariables[lit.id - 1];
                        if (!currentVariables[var.id - 1]) {
                            variables.push_back(var);
                            currentVariables[var.id - 1] = true;
                        }
                    }
                    
                    while (i < clauses.size()) {
                        unsigned int currentSize = clauses[i].literals.size();

                        if (currentSize != 0 && currentSize < minClauseLength) {
                            minClauseLength = currentSize;
                            currentVariables.clear();
                            currentVariables.resize(allVariables.size());
                            variables.clear();
                        }
                        
                        //insert variables into set if clause is of mininmal length
                        if (currentSize == minClauseLength) {
                            for (Lit lit: clauses[i].literals) {
                                //std::cout << "lit id: " << lit.id << ", allVars size: " << allVariables.size() << std::endl;
                                Var var = allVariables[lit.id - 1];
                                if (!currentVariables[var.id - 1]) {
                                    variables.push_back(var);
                                    currentVariables[var.id - 1] = true;
                                }
                            }
                        }

                        i += 1;
                    }

                    for (Var var: variables) {
                        calculateHeuristicValue(var);
                    }
                }
                //std::cout << "min length heuristic: " << minClauseLength << std::endl;
                if (!dynamicHeuristic) {
                    variables = std::deque(allVariables.begin(), allVariables.end());
                }

                std::sort(variables.begin(), variables.end(), compare);
            }
};

#endif