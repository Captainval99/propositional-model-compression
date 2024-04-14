#ifndef SRC_PARSER_SATTYPEs_H_
#define SRC_PARSER_SATTYPES_H_

#include <stdlib.h>
#include <vector>

enum Value {
    TRUE,
    FALSE,
    OPEN
};

struct Var
{
    unsigned int id;
    Value state;
    unsigned int nrPosOcc;
    unsigned int nrNegOcc;

    Var(unsigned int id) : id(id), state(OPEN) {}

    bool operator<(Var var) const {
        return id < var.id;
    }

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
};

struct ModelVar
{
    unsigned int id;
    Value assignment;

    explicit ModelVar(int inputId) {
        id = abs(inputId);
        if (inputId < 0) {
            assignment = FALSE;
        } else {
            assignment = TRUE;
        }
    }
};

#endif