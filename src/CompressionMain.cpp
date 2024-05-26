#include <iostream>
#include <deque>
#include <fstream>
#include <filesystem>

#include "Parser.h"
#include "Propagation.h"
#include "SATTypes.h"
#include "Heuristics.h"
#include "Output.h"
#include "StringCompression.h"

namespace fs = std::filesystem;

CompressionInfo compressModel(const char* formulaFile, const char* modelFile, const char* outputFile) {
    Parser parser(formulaFile, modelFile);

    std::cout << "Reading clauses" << std::endl;
    std::vector<Cl> clauses = parser.readClauses();
    std::vector<Var> variables = parser.readVariables();
    std::cout << "Reading model" << std::endl;
    std::map<unsigned int, ModelVar> model = parser.readModel();

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
    std::deque variablesDq(variables.begin(), variables.end());
    Heuristic* heuristic = new JeroslowWang(variablesDq);

    std::vector<uint64_t> compresssionDistances;
    bool allSatisfied = false;
    uint64_t currentDistance = 0;
    uint64_t nrPredictions = 0;

    while (!allSatisfied) {
        //get next value from the heuristic and assign it to the variable
        Var nextVar = heuristic->getNextVar();

        //get next value if the variable is already assigned or no model value exists
        while ((variables[nextVar.id -1].state != Assignment::OPEN) || !model.count(nextVar.id)) {
            nextVar = heuristic->getNextVar();
            currentDistance += 1;
        }
        nrPredictions += 1;
        
        ModelVar modelVar = model.at(nextVar.id);

        Var& propVar = variables[modelVar.id - 1];
        propVar.state = modelVar.assignment;

        //check if the model value matches the prediction model
        if ((modelVar.assignment == Assignment::FALSE && propVar.nrPosOcc >= propVar.nrNegOcc) || (modelVar.assignment == Assignment::TRUE && propVar.nrPosOcc < propVar.nrNegOcc)) {
            compresssionDistances.push_back(currentDistance);
            currentDistance = 0;
        } else {
            currentDistance += 1;
        }

        //std::cout << "Assigned Variable: " << propVar.id << " with " << propVar.state << std::endl;

        //propagate the new assigned variable
        Propagation::propagate(clauses, variables, propVar);

        //recalculate the heuristic values. Only does something if the heuristic is not static
        heuristic->updateHeuristic();

        //std::cout << "\nDuring propagation assigned: " << assigned << std::endl;

        //std::cout << "Number of assigned variables: " << nrAssigned << std::endl;

        
        //check if all clauses are already satisfied
        allSatisfied = true;

        for (Cl clause: clauses) {
            if (clause.literals.size() != 0) {
                allSatisfied = false;
                break;
            }
        }
    }

    std::cout << "Number of distances: " << compresssionDistances.size() << std::endl;

    std::string outputString;

    for (int i = 0; i < compresssionDistances.size(); i++) {
        unsigned int distance = compresssionDistances[i];
        outputString.append(std::to_string(distance));

        //check if distance is the last
        if (i != compresssionDistances.size() - 1) {
            outputString.append(" ");
        }
    }

    //compress the string using zlib
    std::string compressedOutput = StringCompression::compressString(outputString);

    //write the compressed model to the output file
    std::ofstream outputFileStream(outputFile);
    outputFileStream << compressedOutput;
    outputFileStream.close();

    delete heuristic;

    //get the file sizes
    std::uintmax_t modelFileSize = fs::file_size(modelFile);
    std::uintmax_t compressionFileSize = fs::file_size(outputFile);

    //calculate hite rate
    float predictionHitRate = (float) compresssionDistances.size() / nrPredictions;
    predictionHitRate = 1.0 - predictionHitRate;

    CompressionInfo info(clauses.size(), model.size(), variables.size(), modelFileSize, compressionFileSize, predictionHitRate);
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
        std::cout <<  "Number of clauses: " << info.formulaSize << ", number of variables: " << info.variablesSize << ", size of model: " << info.modelSize
                  << ", file size of model: " << info.modelFileSize << ", size of compressed file: " << info.compressionFileSize << ", compression ratio: " << info.compressionRatioFileSize << std::endl;
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

        //write the statistics to a csv file
        fs::path statisticsFile = outputPath;
        statisticsFile.append("statistics");
        statisticsFile.replace_extension(".csv");
        std::string statisticsFileString(statisticsFile);
        
        output.writeToCsv(statisticsFileString.c_str());

    } else {
        throw std::runtime_error("Wrong Arguments. Arguments must be either files or directories.");
    }

    return 0;
}