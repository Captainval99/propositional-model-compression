#include <iostream>
#include <deque>
#include <fstream>
#include <filesystem>
#include <chrono>

#include "Parser.h"
#include "Propagation.h"
#include "SATTypes.h"
#include "Heuristics.h"
#include "Output.h"
#include "StringCompression.h"
#include "BitvectorEncoding.h"

namespace fs = std::filesystem;

std::map<unsigned int, double> Heuristic::heuristicValues;

struct CompressionSetup
{
    std::string heuristic;
    std::string genericCompression;
    double momsParameter;
    unsigned int golombRiceParameter;
    unsigned int predictionFlip;

    explicit CompressionSetup() : heuristic("jewa_dyn"), genericCompression("golrice"), momsParameter(10.0), golombRiceParameter(2), predictionFlip(5) {}
};


CompressionInfo compressModel(const char* formulaFile, const char* modelFile, const char* outputFile, CompressionSetup setup) {
    //set start time
    const auto startTime = std::chrono::high_resolution_clock::now();

    Parser parser(formulaFile, modelFile);

    std::cout << "Reading clauses" << std::endl;
    std::vector<Cl> clauses = parser.readClauses();
    std::vector<Var> variables = parser.readVariables();
    std::cout << "Reading model" << std::endl;
    std::map<unsigned int, ModelVar> model = parser.readModel();

    //measure parsing time
    const auto parsingTime = std::chrono::high_resolution_clock::now();

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
    } else {
        throw std::runtime_error("Unknown heuristic: " + setup.heuristic);
    }

    //Heuristic* heuristic = new MomsFreeman(variables, clauses, true);
    //Heuristic* heuristic = new JeroslowWang(variables, true);
    std::vector<bool> bitvector;
    bool allSatisfied = false;
    uint64_t predictionMisses = 0;
    uint64_t nrPredictions = 0;
    uint64_t predictionDistance = 0;
    bool flipPredictionModel = false;

    std::vector<Assignment> values(variables.size(), Assignment::OPEN);
    std::vector<unsigned int> trail;
    int head = 0;

    while (!allSatisfied) {
        //get next value from the heuristic and assign it to the variable
        Var nextVar = heuristic->getNextVar();
        //std::cout << "nextVar: " << nextVar.id << std::endl;

        //get next value if the variable is already assigned or no model value exists
        while ((values[nextVar.id -1] != Assignment::OPEN) || !model.count(nextVar.id)) {
            nextVar = heuristic->getNextVar();
            bitvector.push_back(true);
        }

        //check if the prediction model has to be flipped
        if (predictionDistance == setup.predictionFlip) {
            flipPredictionModel = !flipPredictionModel;
            std::cout << "Prediction model was flipped." << std::endl;
        }

        nrPredictions += 1;
        
        ModelVar modelVar = model.at(nextVar.id);

        values[modelVar.id - 1] = modelVar.assignment;
        trail.push_back(modelVar.id);

        //check if the model value matches the prediction model
        Var& propVar = variables[modelVar.id - 1];
        if ((modelVar.assignment == Assignment::FALSE && propVar.nrPosOcc >= propVar.nrNegOcc) || (modelVar.assignment == Assignment::TRUE && propVar.nrPosOcc < propVar.nrNegOcc)) {
            bitvector.push_back(false != flipPredictionModel);
            if (!flipPredictionModel) {
                predictionDistance += 1;
                predictionMisses += 1;
            } else {
                predictionDistance = 0;
            }
        } else {
            bitvector.push_back(true != flipPredictionModel);
            
            if (flipPredictionModel) {
                predictionMisses += 1;
                predictionDistance += 1;
            } else {
                predictionDistance = 0;
            }
        }

        //std::cout << "Assigned Variable: " << propVar.id << " with " << values[propVar.id - 1] << std::endl;

        //propagate the new assigned variable
        Propagation::propagate(clauses, variables, trail, head, heuristic, values);

        //recalculate the heuristic values. Only does something if the heuristic is not static
        //heuristic->updateHeuristic();

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

    std::cout << "prediction flip: " << flipPredictionModel << std::endl; 

    std::vector<uint32_t> outputEncoding = BitvectorEncoding::diffEncoding(bitvector);

    //create output stream for the output file
    std::ofstream outputFileStream(outputFile);

    std::vector<char> compressedEncoding;

    //determine which generic compression algorith should be used 
    if (setup.genericCompression == "golrice") {
        compressedEncoding = StringCompression::golombRiceCompression(outputEncoding, setup.golombRiceParameter);

        std::ostream_iterator<char> outputIterator(outputFileStream);
        std::copy(compressedEncoding.begin(), compressedEncoding.end(), outputIterator);
    } else {
        //convert the vector to a string
        std::string outputString;
        for (uint32_t value: outputEncoding) {
            outputString.append(std::to_string(value));
            outputString.append(" ");
        }
        //remove last whitespace
        if (outputString.length() > 0) {
            outputString.pop_back();
        }

        std::string compressedOutput;

        if (setup.genericCompression == "zip") {
            compressedOutput = StringCompression::compressString(outputString);
        } else if (setup.genericCompression == "lz4") {
            compressedOutput = StringCompression::lz4Compression(outputString);
        } else {
            throw std::runtime_error("Unknown compression algorithm: " + setup.genericCompression);
        }

        outputFileStream << compressedOutput;
    }
    
    outputFileStream.close();

    delete heuristic;

    //get overall execution time
    const auto overallTime = std::chrono::high_resolution_clock::now();

    //calculate durations
    std::chrono::duration<double, std::milli> parsingDuration = parsingTime - startTime;
    std::chrono::duration<double, std::milli> overallDuration = overallTime - startTime;

    //get the file sizes
    std::uintmax_t modelFileSize = fs::file_size(modelFile);
    std::uintmax_t compressionFileSize = fs::file_size(outputFile);

    //calculate hite rate
    float predictionHitRate = (float) predictionMisses / nrPredictions;
    predictionHitRate = 1.0 - predictionHitRate;

    CompressionInfo info(clauses.size(), model.size(), variables.size(), modelFileSize, compressionFileSize, bitvector.size(), outputEncoding.size(), predictionHitRate, parsingDuration.count(), overallDuration.count());
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

    CompressionSetup setup;

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
            } else {
                throw std::runtime_error("Unknown argment: " + argString);
            }
        }
    }

    std::vector<CompressionInfo> compressionStats;

    //input is files so only one compression has to be done
    if (fs::is_regular_file(formulaPath) && fs::is_regular_file(modelPath)) {
        std::cout << "Compress model: " << modelPath << std::endl;
        
        CompressionInfo info = compressModel(argv[1], argv[2], argv[3], setup);

        compressionStats.push_back(info);

        std::cout << "Stats: " << std::endl;
        StatsOutput output(compressionStats);
        output.printStatistics();
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

                    std::cout << "Compress model: " << model.path() << std::endl;

                    CompressionInfo info = compressModel(instanceFileString.c_str(), modelFileString.c_str(), outputFileString.c_str(), setup);
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