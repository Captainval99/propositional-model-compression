#ifndef SRC_PARSER_PARSER_H_
#define SRC_PARSER_PARSER_H_

#include <vector>
#include <algorithm>
#include <map>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

#include "StreamBuffer.h"
#include "SATTypes.h"
#include "StringCompression.h"

class Parser
{
private:
    const char *formulaFilename;
    const char *modelFilename;
public:
    explicit Parser(const char *formulaFilename, const char *modelFilename) : formulaFilename(formulaFilename), modelFilename(modelFilename) {}

    std::vector<Cl> readClauses() {
        StreamBuffer reader(formulaFilename);
        std::vector<Cl> formula;

        while (reader.skipWhitespace()) {
            Cl clause;

            if (*reader == 'p' || *reader == 'c') {
                if (!reader.skipLine()) {
                    break;
                }
            }

            int literal;
            while(reader.readInteger(&literal) && literal != 0) {
                Lit newLit = Lit(abs(literal), (literal < 0));

                //check if the inverted literal is contained in the clause in which case the clause is a tautology and can be ignored
                /*
                if (clause.containsLiteral(~newLit)) {
                    continue;
                }
                */

                clause.addLiteral(newLit);
            }
            formula.push_back(clause);
        }

        return formula;
    }

    std::map<unsigned int, ModelVar> readModel() {
        StreamBuffer reader(modelFilename);
        std::map<unsigned int, ModelVar> model;

        while (reader.skipWhitespace()) {
            if (*reader == 'v') {
                reader.skip();
            }

            int assignment;

            reader.readInteger(&assignment);

            if (assignment != 0) {
                ModelVar newModelVar = ModelVar(assignment);

                //check if variable is already contained in the vector to prevent multiple assignents for the same variable
                if (model.count(newModelVar.id)) {
                    throw std::runtime_error("A variable gets assigned multiple times in the model: " + std::to_string(newModelVar.id));
                }

                model.insert(std::pair<unsigned int, ModelVar>(newModelVar.id, newModelVar));
            } else {
                break;
            }
        }

        return model;
        
    }

    std::vector<Var> readVariables() {
        StreamBuffer reader(formulaFilename);
        std::vector<Var> variables;
        int nrVariables = 0;

        while (reader.skipWhitespace()) {
            if (*reader == 'c') {
                if (!reader.skipLine()) {
                    break;
                }
            }

            if (*reader == 'p') {
                if (!reader.skipString("p cnf ")) {
                    break;
                }

                //read the number of variables
                reader.readInteger(&nrVariables);
                break;
            }
        }

        //create vector with variables
        for (int i = 1; i <= nrVariables; i++) {
            variables.push_back(Var(i));
        }

        return variables;
    }

    std::deque<uint64_t> readCompressedFile(std::string genericCompression, unsigned int golombRiceParameter, unsigned int variablesSize) {
        std::deque<uint64_t> distances;

        //read the whole file into a string
        std::ifstream compressedFile(modelFilename);
        std::stringstream buffer;
        buffer << compressedFile.rdbuf();
        std::string compressedString = buffer.str();

        //decompress the string and write the contents to a temporary file
        std::string decompressedString;

        if (genericCompression == "golrice") {
            decompressedString = StringCompression::golombRiceDecompression(compressedString, golombRiceParameter);
        } else if (genericCompression == "zip") {
            decompressedString = StringCompression::decompressString(compressedString);
        } else if (genericCompression == "lz4") {
            decompressedString = StringCompression::lz4Decompression(compressedString, variablesSize * 3);
        } else {
            throw std::runtime_error("Unknown compression algorithm: " + genericCompression);
        }

        if (!decompressedString.empty()) {
            std::string modelFilenameString(modelFilename);
            std::string temporaryFileName = modelFilenameString.substr(0, modelFilenameString.size() - 4) + "_tmp.txt";

            //std::cout << "temporaryFileName: " << temporaryFileName << std::endl;
            //std::cout << "decompressed string: " << decompressedString << std::endl;

            std::ofstream temporaryFileStream(temporaryFileName.c_str());
            temporaryFileStream << decompressedString;
            temporaryFileStream.close();

            StreamBuffer reader(temporaryFileName.c_str());
            uint64_t currentDistance;

            while (reader.readUInt64(&currentDistance)) {
                distances.push_back(currentDistance);
            }

            //delete the temporary file
            fs::path temoraryPath(temporaryFileName.c_str());
            fs::remove(temoraryPath);
        }
        
        return distances;
    }
};

#endif