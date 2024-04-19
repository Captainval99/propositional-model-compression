#include <iostream>
#include <deque>
#include <fstream>

#include "Parser.h"
#include "Propagation.h"
#include "SATTypes.h"


int main(int argc, char** argv) {

    if (argc != 4) {
        throw std::runtime_error("Wrong number of arguments: " + std::to_string(argc - 1) + ", expected 3 arguments.");
    }

    Parser parser(argv[1], argv[2]);

    std::vector<Cl> clauses = parser.readClauses();
    std::vector<Var> variables = parser.readVariables();
    std::deque<ModelVar> compressedModel = parser.readModel();

    std::cout << "Number of Variables: " << variables.size() << std::endl;
    std::cout << "Number of Clauses: " << clauses.size() << std::endl;
    std::cout << "Size of compressed Model: " << compressedModel.size() << std::endl;

    //build occurence list
    for (Cl& clause: clauses) {
        for (Lit lit: clause.literals) {
            Var& var = variables.at(lit.id - 1);

            if (lit.negative) {
                var.addNegClause(&clause);
            } else {
                var.addPosClause(&clause);
            }
        }
    }

    int nrAssigned = 0;

    while (nrAssigned < variables.size()) {
        //get next value from the model and assign it to the variable
        ModelVar modelVar = compressedModel.front();
        compressedModel.pop_front();

        Var& propVar = variables[modelVar.id - 1];
        propVar.state = modelVar.assignment;

        nrAssigned += 1;

        std::cout << "\nAssigned Variable: " << propVar.id << " with " << propVar.state << std::endl;

        //propagate the new assigned variable
        int assigned = Propagation::propagate(clauses, variables, propVar);
        nrAssigned += assigned;

        std::cout << "\nDuring propagation assigned: " << assigned << std::endl;

        std::cout << "Number of assigned Variables: " << nrAssigned << std::endl;
    }


    //write the decompressed model to the output file
    std::ofstream outputFile(argv[3]);

    outputFile << "v ";

    for (Var var: variables) {
        if (var.state == Assignment::FALSE) {
            outputFile << "-";
        }

        outputFile << var.id << " ";
    }

    outputFile << "0";
    outputFile.close();

    return 0;
}