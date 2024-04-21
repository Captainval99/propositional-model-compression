#ifndef SRC_PROPAGATION_HEURISTICS_H
#define SRC_PROPAGATION_HEURISTICS_H

#include <deque>
#include <algorithm>

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

#endif