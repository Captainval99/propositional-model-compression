#ifndef SRC_PARSER_SATTYPES_H_
#define SRC_PARSER_SATTYPES_H_

#include <stdlib.h>
#include <vector>

enum Assignment {
    FALSE,
    TRUE,
    OPEN
};

struct Lit
{
    unsigned int id;
    bool negative;

    Lit(unsigned int id, bool negative) : id(id), negative(negative) {}
};

struct Cl
{
    std::vector<Lit> literals;
    unsigned int nrPos;
    unsigned int nrNeg;

    Cl() : nrPos(0), nrNeg(0) {}

    void addLiteral(Lit lit) {
        literals.push_back(lit);

        if (lit.negative) {
            nrNeg += 1;
        } else {
            nrPos += 1;
        }
    }
};

struct ModelVar
{
    unsigned int id;
    Assignment assignment;

    explicit ModelVar(int inputId) {
        id = abs(inputId);
        if (inputId < 0) {
            assignment = Assignment::FALSE;
        } else {
            assignment = Assignment::TRUE;
        }
    }
};

struct Var
{
    unsigned int id;
    Assignment state;
    unsigned int nrPosOcc;
    unsigned int nrNegOcc;
    std::vector<Cl*> posOccList;
    std::vector<Cl*> negOccList; 

    Var(unsigned int id) : id(id), state(Assignment::OPEN), nrPosOcc(0), nrNegOcc(0) {}

    bool operator<(Var var) const {
        return id < var.id;
    }

    void addPosClause(Cl* clause) {
        posOccList.push_back(clause);
        nrPosOcc += 1;
    }

    void addNegClause(Cl* clause) {
        negOccList.push_back(clause);
        nrNegOcc += 1;
    }

};

#endif