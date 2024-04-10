#ifndef SRC_PARSER_PARSER_H_
#define SRC_PARSER_PARSER_H_

#include <vector>

#include "StreamBuffer.h"
#include "SATTypes.h"

class Parser
{
private:
    const char *formulaFilename;
    const char *modelFilename;
public:
    explicit Parser(char *formulaFilename, char *modelFilename) : formulaFilename(formulaFilename), modelFilename(modelFilename) {}

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
                clause.push_back(Lit(abs(literal), (literal < 0)));
            }
            formula.push_back(clause);
        }

        return formula;
    }

    std::vector<int> readModel() {
        StreamBuffer reader(modelFilename);
        std::vector<int> model;

        while (reader.skipWhitespace()) {
            if (*reader == 'v') {
                reader.skip();
            }

            int assignment;

            reader.readInteger(&assignment);

            if (assignment != 0) {
                model.push_back(assignment);
            } else {
                break;
            }
        }

        return model;
        
    }
};




#endif