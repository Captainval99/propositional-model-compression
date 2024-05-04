#include <iostream>
#include <deque>
#include <fstream>
#include <filesystem>

#include "Parser.h"
#include "Propagation.h"
#include "SATTypes.h"
#include "Heuristics.h"
#include "Output.h"

namespace fs = std::filesystem;

CompressionInfo compressModel(const char* formulaFile, const char* modelFile, const char* outputFile) {
    Parser parser(formulaFile, modelFile);

    std::cout << "Reading clauses" << std::endl;
    std::vector<Cl> clauses = parser.readClauses();
    std::vector<Var> variables = parser.readVariables();
    std::cout << " Reading model" << std::endl;
    std::deque<ModelVar> model = parser.readModel();

    std::cout << "Number of Variables: " << variables.size() << std::endl;
    std::cout << "Number of Clauses: " << clauses.size() << std::endl;
    std::cout << "Size of model: " << model.size() << std::endl;

    //correct the length of the variables vector if the model is bigger than the size of the variable vector
    if (model.size() > variables.size()) {
        int diff = model.size() - variables.size();
        for (int i = 1; i <= diff; i++) {
            variables.push_back(Var(variables.size() + i));
        }
        std::cout << "Corrected number of variables: " << variables.size() << std::endl;
    }

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
    Heuristic* heuristic = new JeroslowWang(model, variables);

    int nrAssigned = 0;
    std::vector<Var> compressedModel;
    bool allSatisfied = false;

    while (nrAssigned < variables.size() && !allSatisfied) {
        //get next value from the heuristic and assign it to the variable
        ModelVar modelVar = heuristic->getNextVar();

        //get next value if the variable is already assigned
        while (variables[modelVar.id -1].state != Assignment::OPEN) {
            modelVar = heuristic->getNextVar();
        }
        

        Var& propVar = variables[modelVar.id - 1];
        propVar.state = modelVar.assignment;

        nrAssigned += 1;

        compressedModel.push_back(propVar);

        if (propVar.id % 1000 == 0) {
            std::cout << "Assigned Variable: " << propVar.id << " with " << propVar.state << std::endl;
        }

        //propagate the new assigned variable
        int assigned = Propagation::propagate(clauses, variables, propVar);
        nrAssigned += assigned;

        //recalculate the heuristic values. Only does something if the heuristic is not static
        heuristic->updateHeuristic();

        //std::cout << "\nDuring propagation assigned: " << assigned << std::endl;

        //std::cout << "Number of assigned variables: " << nrAssigned << std::endl;

        
        //check if all clauses are already satisfied
        allSatisfied = true;

        for (Cl clause: clauses) {
            bool satisfied = false;
            for (Lit lit: clause.literals) {
                if ((lit.negative && variables[lit.id -1].state == Assignment::FALSE) || (!lit.negative && variables[lit.id -1].state == Assignment::TRUE)) {
                    satisfied = true;
                    break;
                }
            }
            if (!satisfied) {
                allSatisfied = false;
                break;
            }
        }
    }

    std::cout << "Size of compressed model: " << compressedModel.size() << std::endl;

    //write the compressed model to the output file
    std::ofstream outputFileStream(outputFile);

    outputFileStream << "v ";

    for (Var var: compressedModel) {
        if (var.state == Assignment::FALSE) {
            outputFileStream << "-";
        }

        outputFileStream << var.id << " ";
    }

    outputFileStream << "0";
    outputFileStream.close();

    delete heuristic;

    CompressionInfo info(clauses.size(), model.size(), variables.size(), compressedModel.size());
    return info;
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
        std::cout << "Compress model: " << modelPath << std::endl;
        
        CompressionInfo info = compressModel(argv[1], argv[2], argv[3]);

        std::cout << "Stats: " << std::endl;
        std::cout <<  "Number of clauses: " << info.formulaSize << ", number of variables: " << info.variablesSize << ", size of model: " << info.modelSize << ", size of compressed model: " << info.compressedModelSize << std::endl;
        return 0;
    } else if (fs::is_directory(formulaPath) && fs::is_directory(modelPath) && fs::is_directory(outputPath)) {
        std::vector<CompressionInfo> compressionStats;
    
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

                    std::cout << "Compress model: " << model.path() << std::endl;

                    CompressionInfo info = compressModel(instanceFileString.c_str(), modelFileString.c_str(), outputFileString.c_str());
                    info.addNames(instanceName, modelName);
                    compressionStats.push_back(info);
                }
            }
        }

        //print the statistics
        StatsOutput output(compressionStats);
        output.printStatistics();

        



    } else {
        throw std::runtime_error("Wrong Arguments. Arguments must be either files or directories.");
    }

    return 0;
}