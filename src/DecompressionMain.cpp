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

struct DecompressionSetup
{
    std::string heuristic;
    std::string genericCompression;
    double momsParameter;
    unsigned int golombRiceParameter;
    unsigned int predictionFlip;
    unsigned int hybridHeuristicParam;

    explicit DecompressionSetup() : heuristic("jewa_dyn"), genericCompression("golrice"), momsParameter(10.0), golombRiceParameter(2), predictionFlip(5) {}
};


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

DecompressionInfo decompressModel(const char* formulaFile, const char* modelFile, const char* outputFile, DecompressionSetup setup) {
    //set start time
    const auto startTime = std::chrono::high_resolution_clock::now();

    Parser parser(formulaFile, modelFile);

    std::vector<Cl> clauses = parser.readClauses();
    std::vector<Var> variables = parser.readVariables();
    std::deque<uint64_t> compresssionDistances = parser.readCompressedFile(setup.genericCompression, setup.golombRiceParameter, variables.size());

    std::cout << "Number of Variables: " << variables.size() << std::endl;
    std::cout << "Number of Clauses: " << clauses.size() << std::endl;
    std::cout << "Number of distances: " << compresssionDistances.size() << std::endl;

    std::cout << "Diff encoding: ";
    for (uint32_t i: compresssionDistances) {
    	std::cout << i << ", ";
    }
    std::cout << std::endl;

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

    Heuristic* heuristic;

    //create Heuristic object to sort the variables using a specific heuristic
    if (setup.heuristic == "none") {
        heuristic = new ParsingOrder(variables);
    } else if (setup.heuristic == "jewa") {
        heuristic = new JeroslowWang(variables, false);
    } else if (setup.heuristic == "jewa_dyn") {
        heuristic = new JeroslowWang(variables, true);
    } else if (setup.heuristic == "moms") {
        heuristic = new MomsFreeman(variables, clauses, false, setup.momsParameter);
    } else if (setup.heuristic == "moms_dyn") {
        heuristic = new MomsFreeman(variables, clauses, true, setup.momsParameter);
    } else if (setup.heuristic == "hybr") {
        heuristic = new HybridHeuristic(variables, false, setup.hybridHeuristicParam);
    } else if (setup.heuristic == "hybr_dyn") {
        heuristic = new HybridHeuristic(variables, true, setup.hybridHeuristicParam);
    } else {
        throw std::runtime_error("Unknown heuristic: " + setup.heuristic);
    }

    //Heuristic* heuristic = new MomsFreeman(variables, clauses, true);
    //Heuristic* heuristic = new JeroslowWang(variables, true);
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

        //reset the number of misses if the current distance is over 1
        if (currentDistance != 0) {
                    missesCounter = 0;
                }

        Var& propVar = variables[nextVar.id - 1];

        
        
        //assign the variable according to the prediciton model and invert it if necessary
        Assignment predictionValue = heuristic->getPredictedAssignment(propVar);

        if (predictionValue == Assignment::TRUE) {
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

            missesCounter += 1;

            //check if the prediction model has to be flipped
            if (missesCounter == setup.predictionFlip) {
                flipPredictionModel = !flipPredictionModel;
                std::cout << "Prediction model was flipped" << std::endl;
            }
            
            if (compresssionDistances.empty()) {
                allDistancesUsed = true;
            } else {
                currentDistance = compresssionDistances.front();
                compresssionDistances.pop_front();
            }
        } else if (!allDistancesUsed) {
            currentDistance -= 1;
        }

        trail.push_back(propVar.id);


        std::cout << "Assigned Variable: " << propVar.id << " with " << values[propVar.id - 1] << std::endl;

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

    //check if don't care variables were propagated
    if (compresssionDistances.size() != 0 || currentDistance != 0) {
        while(heuristic->hasNextVar()) {
            Var nextVar = heuristic->getNextVar();

            if (values[nextVar.id - 1] != Assignment::OPEN) {
                continue;
            }

            Assignment predictedAssignment = heuristic->getPredictedAssignment(nextVar);
            values[nextVar.id - 1] = predictedAssignment;

            if (currentDistance == 0) {
                if (predictedAssignment == Assignment::TRUE) {
                    values[nextVar.id - 1] = Assignment::FALSE;
                } else {
                    values[nextVar.id - 1] = Assignment::TRUE;
                }

                currentDistance = compresssionDistances.front();
                compresssionDistances.pop_front();
            } else {
                currentDistance -= 1;
            }
            
        }

        //iterate over the propagated don't care variables and reset their assignment to don't care
        for (uint64_t dontCareVar: compresssionDistances) {
            values[dontCareVar - 1] = Assignment::OPEN;
        }
    }


    //write the decompressed model to the output file
    std::ofstream outputFileStream(outputFile);

    outputFileStream << "v ";

    for (int i = 0; i < variables.size(); i++) {
        Var var = variables[i];

        if (values[i] == Assignment::OPEN) {
            outputFileStream << "D ";
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
    if (argc < 4) {
        throw std::runtime_error("Wrong number of arguments: " + std::to_string(argc - 1) + ", expected at least 3 arguments.");
    } else if ((argc % 2) != 0) {
        throw std::runtime_error("Wrong number of arguments.");
    }

    fs::path formulaPath(argv[1]);
    fs::path modelPath(argv[2]);
    fs::path outputPath(argv[3]);

    DecompressionSetup setup;

    if (argc > 4) {
        for (int i = 4; i < argc; i += 2) {
            std::string argString = std::string(argv[i]);

            if (argString == "-h") {
                setup.heuristic = std::string(argv[i + 1]);
            } else if (argString == "-c") {
                setup.genericCompression = std::string(argv[i + 1]);
            } else if (argString == "-mp") {
                setup.momsParameter = atof(argv[i + 1]);
            } else if (argString == "-grp") {
                setup.golombRiceParameter = std::stoi(argv[i + 1]);
            } else if (argString == "-p") {
                setup.predictionFlip = std::stoi(argv[i + 1]);
            } else if (argString == "-hp") {
                setup.hybridHeuristicParam = std::stoi(argv[i + 1]);
            } else {
                throw std::runtime_error("Unknown argment: " + argString);
            }
        }
    }

    //input is files so only one compression has to be done
    if (fs::is_regular_file(formulaPath) && fs::is_regular_file(modelPath)) {
        std::cout << "Decompress model: " << modelPath << std::endl;
        
        decompressModel(argv[1], argv[2], argv[3], setup);

        std::cout << "Done." << std::endl;
        return 0;
    } else if (fs::is_directory(formulaPath) && fs::is_directory(modelPath) && fs::is_directory(outputPath)) {
        std::vector<DecompressionInfo> infos;

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

                    DecompressionInfo info = decompressModel(instanceFileString.c_str(), modelFileString.c_str(), outputFileString.c_str(), setup);
                    info.addNames(instanceName, modelName);
                    infos.push_back(info);

                    std::cout << "Done." << std::endl;
                }
            }
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

    } else {
        throw std::runtime_error("Wrong Arguments. Arguments must be either files or directories.");
    }

    return 0;
}