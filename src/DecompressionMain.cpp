#include <iostream>
#include <deque>
#include <fstream>
#include <filesystem>

#include "Parser.h"
#include "Propagation.h"
#include "SATTypes.h"
#include "Heuristics.h"

namespace fs = std::filesystem;

void decompressModel(const char* formulaFile, const char* modelFile, const char* outputFile) {
    Parser parser(formulaFile, modelFile);

    std::vector<Cl> clauses = parser.readClauses();
    std::vector<Var> variables = parser.readVariables();
    std::deque<uint64_t> compresssionDistances = parser.readCompressedFile();

    std::cout << "Number of Variables: " << variables.size() << std::endl;
    std::cout << "Number of Clauses: " << clauses.size() << std::endl;
    std::cout << "Number of distances: " << compresssionDistances.size() << std::endl;

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

    //create Heuristic object to sort the variables using a specific heuristic
    std::deque variablesDq(variables.begin(), variables.end());
    Heuristic* heuristic = new JeroslowWang(variablesDq);

    bool allSatisfied = false;
    bool allDistancesUsed = false;
    uint64_t currentDistance;
    if (compresssionDistances.empty()) {
        allDistancesUsed = true;
    } else {
        currentDistance = compresssionDistances.front();
        compresssionDistances.pop_front();
    }

    while (!allSatisfied) {
        //get next value from the model and assign it to the variable
        Var nextVar = heuristic->getNextVar();

        //get next value if the variable is already assigned
        while (variables[nextVar.id -1].state != Assignment::OPEN) {
            nextVar = heuristic->getNextVar();
            if (!allDistancesUsed) {
                currentDistance -= 1;
            }
        }

        Var& propVar = variables[nextVar.id - 1];
        
        //assign the variable according to the prediciton model and invert it if necessary
        if (propVar.nrPosOcc >= propVar.nrNegOcc) {
            propVar.state = Assignment::TRUE;
        } else {
            propVar.state = Assignment::FALSE;
        }

        if (!allDistancesUsed && currentDistance == 0) {
            propVar.invert();
            
            if (compresssionDistances.empty()) {
                allDistancesUsed = true;
            } else {
                currentDistance = compresssionDistances.front();
                compresssionDistances.pop_front();
            }
        } else if (!allDistancesUsed) {
            currentDistance -= 1;
        }


        //std::cout << "\nAssigned Variable: " << propVar.id << " with " << propVar.state << std::endl;

        //propagate the new assigned variable
        Propagation::propagate(clauses, variables, propVar);

        //std::cout << "\nDuring propagation assigned: " << assigned << std::endl;

        //std::cout << "Number of assigned Variables: " << nrAssigned << std::endl;

        //check if all clauses are already satisfied
        allSatisfied = true;

        for (Cl clause: clauses) {
            if (clause.literals.size() != 0) {
                allSatisfied = false;
                break;
            }   
        }
    }


    //write the decompressed model to the output file
    std::ofstream outputFileStream(outputFile);

    //outputFileStream << "v ";

    for (int i = 0; i < variables.size(); i++) {
        Var var = variables[i];

        if (var.state == Assignment::OPEN) {
            continue;
        } else if (var.state == Assignment::FALSE) {
            outputFileStream << "-";
        }

        outputFileStream << var.id;

        //check if variable is the last
        if (i != variables.size() - 1) {
            outputFileStream << " ";
        }
    }

    outputFileStream << "\n";
    outputFileStream.close();

    delete heuristic;
}


int main(int argc, char** argv) {
    if (argc != 4) {
        throw std::runtime_error("Wrong number of arguments: " + std::to_string(argc - 1) + ", expected 3 arguments.");
    }

    fs::path formulaPath(argv[1]);
    fs::path modelPath(argv[2]);
    fs::path outputPath(argv[3]);

    //input is files so only one compression has to be done
    if (fs::is_regular_file(formulaPath) && fs::is_regular_file(modelPath)) {
        std::cout << "Decompress model: " << modelPath << std::endl;
        
        decompressModel(argv[1], argv[2], argv[3]);

        std::cout << "Done." << std::endl;
        return 0;
    } else if (fs::is_directory(formulaPath) && fs::is_directory(modelPath) && fs::is_directory(outputPath)) {
        //iterate over the subdirectories in the models directory
        fs::directory_iterator modelIterator(modelPath);

        for (fs::directory_entry modelsEntry: modelIterator) {
            if (modelsEntry.is_directory()) {
                std::string instanceName = modelsEntry.path().filename();

                fs::path instancePath = formulaPath;
                instancePath.append(instanceName);
                instancePath.replace_extension(".cnf");

                //create new output folder
                fs::path outputSubdirectory = outputPath;
                outputSubdirectory.append(instanceName);
                fs::create_directory(outputSubdirectory);

                //iterate over all models in the folder and compress them
                for(fs::directory_entry model: fs::directory_iterator{modelsEntry.path()}) {
                    std::string instanceFileString(instancePath);
                    std::string modelFileString(model.path());

                    //get output file
                    std::string modelName = model.path().filename();
                    fs::path outputFile = outputSubdirectory;
                    outputFile.append(modelName);
                    std::string outputFileString(outputFile);

                    std::cout << "Deompress model: " << model.path() << std::endl;

                    decompressModel(instanceFileString.c_str(), modelFileString.c_str(), outputFileString.c_str());

                    std::cout << "Done." << std::endl;
                }
            }
        }

    } else {
        throw std::runtime_error("Wrong Arguments. Arguments must be either files or directories.");
    }

    return 0;
}