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
    unsigned int nrUnasignedVars;

    Cl() : nrUnasignedVars(0) {}

    void addLiteral(Lit lit) {
        literals.push_back(lit);

        nrUnasignedVars += 1;
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
    std::vector<Cl*> posOccList;
    std::vector<Cl*> negOccList; 

    Var(unsigned int id) : id(id) {}

    bool operator< (Var var) const {
        return id < var.id;
    }

    bool operator== (Var var) const {
        return id == var.id;
    }

    void addPosClause(Cl* clause) {
        posOccList.push_back(clause);
    }

    void addNegClause(Cl* clause) {
        negOccList.push_back(clause);
    }

    operator int() const {
        return id;
    }

};

#endif