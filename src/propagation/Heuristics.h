#ifndef SRC_PROPAGATION_HEURISTICS_H
#define SRC_PROPAGATION_HEURISTICS_H

#include <deque>
#include <algorithm>
#include <math.h>

#include "SATTypes.h"

//abstract base class
class Heuristic {

    protected:
        std::deque<Var> variables;
        bool staticHeuristic;
        virtual void sortVariables() = 0;

    public:
        explicit Heuristic(std::deque<Var> variables, bool staticHeuristic) : variables(variables), staticHeuristic(staticHeuristic) {
        }

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
            if (!staticHeuristic) {
                sortVariables();
            }
        }
};

class ParsingOrder: public Heuristic {
    public:
        ParsingOrder(std::deque<Var> variables) : Heuristic(variables, true) {}

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
        explicit JeroslowWang(std::deque<Var> variables) : Heuristic(variables, true) {
            sortVariables();
        }

        void sortVariables() {
            std::sort(variables.begin(), variables.end(), compare);
        }
};

#endif