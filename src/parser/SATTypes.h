#ifndef SRC_PARSER_SATTYPEs_H_
#define SRC_PARSER_SATTYPES_H_

#include <vector>

struct Var
{
    unsigned int id;
    enum {TRUE, FALSE, OPEN} state;
    unsigned int nrPosOcc;
    unsigned int nrNegOcc;

};

struct Lit
{
    unsigned int id;
    bool negative;
};

struct Cl
{
    std::vector<Lit> literals;
    unsigned int nrPos;
    unsigned int nrNeg;
};





#endif