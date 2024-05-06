#ifndef SRC_PARSER_PARSER_H_
#define SRC_PARSER_PARSER_H_

#include <vector>
#include <algorithm>
#include <map>

#include "StreamBuffer.h"
#include "SATTypes.h"

class Parser
{
private:
    const char *formulaFilename;
    const char *modelFilename;
public:
    explicit Parser(const char *formulaFilename, const char *modelFilename) : formulaFilename(formulaFilename), modelFilename(modelFilename) {}

    std::vector<Cl> readClauses() {
        StreamBuffer reader(formulaFilename);
        std::vector<Cl> formula;

        while (reader.skipWhitespace()) {
            Cl clause;

            if (*reader == 'p' || *reader == 'c') {
                if (!reader.skipLine()) {
                    break;
                }
            }

            int literal;
            while(reader.readInteger(&literal) && literal != 0) {
                Lit newLit = Lit(abs(literal), (literal < 0));

                //check if the inverted literal is contained in the clause in which case the clause is a tautology and can be ignored
                /*
                if (clause.containsLiteral(~newLit)) {
                    continue;
                }
                */

                clause.addLiteral(newLit);
            }
            formula.push_back(clause);
        }

        return formula;
    }

    std::deque<ModelVar> readModel() {
        StreamBuffer reader(modelFilename);
        std::deque<ModelVar> model;
        std::map<unsigned int, ModelVar> modelCopy;

        while (reader.skipWhitespace()) {
            if (*reader == 'v') {
                reader.skip();
            }

            int assignment;

            reader.readInteger(&assignment);

            if (assignment != 0) {
                ModelVar newModelVar = ModelVar(assignment);

                //check if variable is already contained in the vector to prevent multiple assignents for the same variable
                if (modelCopy.count(newModelVar.id)) {
                    throw std::runtime_error("A variable gets assigned multiple times in the model: " + std::to_string(newModelVar.id));
                }

                model.push_back(ModelVar(assignment));
                modelCopy.insert(std::pair<unsigned int, ModelVar>(newModelVar.id, newModelVar));
            } else {
                break;
            }
        }

        return model;
        
    }

    std::vector<Var> readVariables() {
        StreamBuffer reader(formulaFilename);
        std::vector<Var> variables;
        int nrVariables = 0;

        while (reader.skipWhitespace()) {
            if (*reader == 'c') {
                if (!reader.skipLine()) {
                    break;
                }
            }

            if (*reader == 'p') {
                if (!reader.skipString("p cnf ")) {
                    break;
                }

                //read the number of variables
                reader.readInteger(&nrVariables);
                break;
            }
        }

        //create vector with variables
        for (int i = 1; i <= nrVariables; i++) {
            variables.push_back(Var(i));
        }

        return variables;
    }
};

#endif