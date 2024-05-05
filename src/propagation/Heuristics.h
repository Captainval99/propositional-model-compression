#ifndef SRC_PROPAGATION_HEURISTICS_H
#define SRC_PROPAGATION_HEURISTICS_H

#include <deque>
#include <algorithm>
#include <math.h>

#include "SATTypes.h"

//abstract base class
class Heuristic {

    protected:
        std::deque<ModelVar> model;
        bool staticHeuristic;
        virtual void sortModel() = 0;

    public:
        explicit Heuristic(std::deque<ModelVar> model, bool staticHeuristic) : model(model), staticHeuristic(staticHeuristic) {
        }

        ModelVar getNextVar() {
            if (model.size() == 0) {
                throw std::runtime_error("Error, the model is not satisfying!");
            }

            ModelVar nextVar = model.front();
            model.pop_front();
            return nextVar;
        }

        void updateHeuristic() {
            //if heuristic is static it is not necessary to recalculate the heuristic
            if (!staticHeuristic) {
                sortModel();
            }
        }
};

class ParsingOrder: public Heuristic {
    public:
        ParsingOrder(std::deque<ModelVar> model) : Heuristic(model, true) {}

        //the list does not get sorted so the method doesn't have to be implemented
        void sortModel() {} 
};

class JeroslowWang: public Heuristic {
    private:
        static inline std::vector<Var> vars;

        static bool compare (const ModelVar var1, const ModelVar var2) {
            //std::cout << "var1: " << var1.id << ", var2: " << var2.id << std::endl;
            Var variable1 = vars[var1.id - 1];
            Var variable2 = vars[var2.id - 1];

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

            return j1 > j2;
        }

    public:
        explicit JeroslowWang(std::deque<ModelVar> model, std::vector<Var> variables ) : Heuristic(model, true) {
            JeroslowWang::vars = variables;
            sortModel();
        }

        void sortModel() {
            std::sort(model.begin(), model.end(), compare);
        }
};

#endif