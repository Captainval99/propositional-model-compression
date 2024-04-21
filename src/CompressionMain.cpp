#include <iostream>
#include <deque>
#include <fstream>

#include "Parser.h"
#include "Propagation.h"
#include "SATTypes.h"
#include "Heuristics.h"


int main(int argc, char** argv) {

    if (argc != 4) {
        throw std::runtime_error("Wrong number of arguments: " + std::to_string(argc - 1) + ", expected 3 arguments.");
    }

    Parser parser(argv[1], argv[2]);

    std::vector<Cl> clauses = parser.readClauses();
    std::vector<Var> variables = parser.readVariables();
    std::deque<ModelVar> model = parser.readModel();

    std::cout << "Number of Variables: " << variables.size() << std::endl;
    std::cout << "Number of Clauses: " << clauses.size() << std::endl;

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

    std::cout << "Made it!" << std::endl;

    //create Heuristic object to sort the variables using a specific heuristic
    Heuristic* heuristic = new ParsingOrder(model);

    int nrAssigned = 0;
    std::vector<Var> compressedModel;

    while (nrAssigned < variables.size()) {
        //get next value from the heuristic and assign it to the variable
        ModelVar modelVar = heuristic->getNextVar();

        std::cout << modelVar.id << std::endl;

        //get next value if the variable is already assigned
        while (variables[modelVar.id -1].state != Assignment::OPEN) {
            modelVar = heuristic->getNextVar();
        }
        

        Var& propVar = variables[modelVar.id - 1];
        propVar.state = modelVar.assignment;

        nrAssigned += 1;

        compressedModel.push_back(propVar);

        std::cout << "\nAssigned Variable: " << propVar.id << " with " << propVar.state << std::endl;

        //propagate the new assigned variable
        int assigned = Propagation::propagate(clauses, variables, propVar);
        nrAssigned += assigned;

        //recalculate the heuristic values. Only does something if the heuristic is not static
        heuristic->updateHeuristic();

        std::cout << "\nDuring propagation assigned: " << assigned << std::endl;

        std::cout << "Number of assigned variables: " << nrAssigned << std::endl;
    }

    std::cout << "\nSize of compressed model: " << compressedModel.size() << std::endl;


    //write the compressed model to the output file
    std::ofstream outputFile(argv[3]);

    outputFile << "v ";

    for (Var var: compressedModel) {
        if (var.state == Assignment::FALSE) {
            outputFile << "-";
        }

        outputFile << var.id << " ";
    }

    outputFile << "0";
    outputFile.close();

    return 0;
}