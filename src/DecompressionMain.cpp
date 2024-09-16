#include <iostream>
#include <deque>
#include <fstream>
#include <filesystem>

#include "Parser.h"
#include "Propagation.h"
#include "SATTypes.h"
#include "Heuristics.h"

namespace fs = std::filesystem;

std::map<unsigned int, double> Heuristic::heuristicValues;

static const unsigned int PREDICTION_FLIP_VALUE = 30;

struct DecompressionInfo {
    std::string formulaName;
    std::string modelName;

    unsigned int formulaSize;
    unsigned int numberVariables;
    double parsingTime;
    double overallTime;

    explicit DecompressionInfo(unsigned int formulaSize, unsigned int numberVariables, double parsingTime, double overallTime) : 
                            formulaSize(formulaSize), numberVariables(numberVariables), parsingTime(parsingTime), overallTime(overallTime) {
    }

    void addNames(const std::string formulaName_, const std::string modelName_) {
        formulaName = formulaName_;
        modelName = modelName_;
    }

};

DecompressionInfo decompressModel(const char* formulaFile, const char* modelFile, const char* outputFile) {
    //set start time
    const auto startTime = std::chrono::high_resolution_clock::now();

    Parser parser(formulaFile, modelFile);

    std::vector<Cl> clauses = parser.readClauses();
    std::vector<Var> variables = parser.readVariables();
    std::deque<uint64_t> compresssionDistances = parser.readCompressedFile();

    std::cout << "Number of Variables: " << variables.size() << std::endl;
    std::cout << "Number of Clauses: " << clauses.size() << std::endl;
    std::cout << "Number of distances: " << compresssionDistances.size() << std::endl;

     //measure parsing time
    const auto parsingTime = std::chrono::high_resolution_clock::now();

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
    //Heuristic* heuristic = new MomsFreeman(variables, clauses, true);
    Heuristic* heuristic = new JeroslowWang(variables, true);
    bool allSatisfied = false;
    bool allDistancesUsed = false;
    uint64_t currentDistance;
    uint64_t missesCounter = 0;
    bool flipPredictionModel = false;

    if (compresssionDistances.empty()) {
        allDistancesUsed = true;
    } else {
        currentDistance = compresssionDistances.front();
        compresssionDistances.pop_front();

        if (currentDistance == 0) {
            missesCounter += 1;
        }
    }

    std::vector<Assignment> values(variables.size(), Assignment::OPEN);
    std::vector<unsigned int> trail;
    int head = 0;

    while (!allSatisfied) {
        //get next value from the model and assign it to the variable
        Var nextVar = heuristic->getNextVar();

        //get next value if the variable is already assigned
        while (values[nextVar.id -1] != Assignment::OPEN) {
            nextVar = heuristic->getNextVar();
            if (!allDistancesUsed) {
                currentDistance -= 1;
            }
        }

        Var& propVar = variables[nextVar.id - 1];

        
        
        //assign the variable according to the prediciton model and invert it if necessary
        if (propVar.nrPosOcc >= propVar.nrNegOcc) {
            if (flipPredictionModel) {
                values[nextVar.id - 1] = Assignment::FALSE;
            } else {
                values[nextVar.id - 1] = Assignment::TRUE;
            }
        } else {
            if (flipPredictionModel) {
                values[nextVar.id - 1] = Assignment::TRUE;
            } else {
                values[nextVar.id - 1] = Assignment::FALSE;
            }
        }

        if (!allDistancesUsed && currentDistance == 0) {
            //invert the assignment
            if (values[nextVar.id - 1] == Assignment::TRUE) {
                values[nextVar.id - 1] = Assignment::FALSE;
            } else {
                values[nextVar.id - 1] = Assignment::TRUE;
            }

            //check if the prediction model has to be flipped
            if (missesCounter == (PREDICTION_FLIP_VALUE - 1)) {
                flipPredictionModel = !flipPredictionModel;
                std::cout << "Prediction model was flipped" << std::endl;
            }
            
            if (compresssionDistances.empty()) {
                allDistancesUsed = true;
            } else {
                currentDistance = compresssionDistances.front();
                compresssionDistances.pop_front();

                if (currentDistance == 0) {
                    missesCounter += 1;
                } else {
                    missesCounter = 0;
                }
            }
        } else if (!allDistancesUsed) {
            currentDistance -= 1;
        }

        trail.push_back(propVar.id);


        //std::cout << "Assigned Variable: " << propVar.id << " with " << values[propVar.id - 1] << std::endl;

        //propagate the new assigned variable
        Propagation::propagate(clauses, variables, trail, head, heuristic, values);

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

    std::cout << "prediction flip: " << flipPredictionModel << std::endl; 


    //write the decompressed model to the output file
    std::ofstream outputFileStream(outputFile);

    //outputFileStream << "v ";

    for (int i = 0; i < variables.size(); i++) {
        Var var = variables[i];

        if (values[i] == Assignment::OPEN) {
            continue;
        } else if (values[i] == Assignment::FALSE) {
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

    //get overall execution time
    const auto overallTime = std::chrono::high_resolution_clock::now();

    //calculate durations
    std::chrono::duration<double, std::milli> parsingDuration = parsingTime - startTime;
    std::chrono::duration<double, std::milli> overallDuration = overallTime - startTime;

    DecompressionInfo info(clauses.size(), variables.size(), parsingDuration.count(), overallDuration.count());

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
        std::cout << "Decompress model: " << modelPath << std::endl;
        
        decompressModel(argv[1], argv[2], argv[3]);

        std::cout << "Done." << std::endl;
        return 0;
    } else if (fs::is_directory(formulaPath) && fs::is_directory(modelPath) && fs::is_directory(outputPath)) {
        //iterate over the subdirectories in the models directory
        fs::directory_iterator modelIterator(modelPath);

        for (fs::directory_entry modelsEntry: modelIterator) {
            std::vector<DecompressionInfo> infos;

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

                    DecompressionInfo info = decompressModel(instanceFileString.c_str(), modelFileString.c_str(), outputFileString.c_str());
                    info.addNames(instanceName, modelName);
                    infos.push_back(info);

                    std::cout << "Done." << std::endl;
                }

                //write the statistics to a csv file
                fs::path statisticsFile = outputPath;
                statisticsFile.append("statistics");
                statisticsFile.replace_extension(".csv");
                std::string statisticsFileString(statisticsFile);

                std::ofstream outputFile(statisticsFileString.c_str());

                //write the headers
                outputFile << "Instance, Model, Clauses count, Variables count, Parsing time, Execution time\n";

                //write the values
                for (DecompressionInfo stat: infos) {
                    outputFile << stat.formulaName << ", " << stat.modelName << ", " << stat.formulaSize << ", " << stat.numberVariables << ", " << stat.parsingTime << ", " << stat.overallTime << "\n";
                }
            }
        }

    } else {
        throw std::runtime_error("Wrong Arguments. Arguments must be either files or directories.");
    }

    return 0;
}