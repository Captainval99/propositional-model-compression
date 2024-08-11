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

    Lit operator~ () const {
        return Lit(id, !negative);
    }

    bool operator== (Lit lit) const {
        return (id == lit.id) && (negative == lit.negative);
    }
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

    bool containsLiteral(Lit lit) {
        return std::find(literals.begin(), literals.end(), lit) != literals.end();
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

    bool operator== (ModelVar var) const {
        return id == var.id;
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

    bool operator< (Var var) const {
        return id < var.id;
    }

    bool operator== (Var var) const {
        return id == var.id;
    }

    void addPosClause(Cl* clause) {
        posOccList.push_back(clause);
        nrPosOcc += 1;
    }

    void addNegClause(Cl* clause) {
        negOccList.push_back(clause);
        nrNegOcc += 1;
    }

    void invert() {
        if (state == Assignment::FALSE) {
            state = Assignment::TRUE;
        } else if (state == Assignment::TRUE) {
            state = Assignment::FALSE;
        }
    }

    operator int() const {
        return id;
    }

};

#endif