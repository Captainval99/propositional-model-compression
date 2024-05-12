#ifndef SRC_UTIL_OUTPUT_H
#define SRC_UTIL_OUTPUT_H

#include <fstream>


struct CompressionInfo {
    std::string formulaName;
    std::string modelName;

    unsigned int formulaSize;
    unsigned int modelSize;
    unsigned int variablesSize;
    std::uintmax_t modelFileSize;
    std::uintmax_t compressionFileSize;
    float compressionRatio;

    explicit CompressionInfo(std::size_t formulaSize, std::size_t modelSize, std::size_t variablesSize, std::uintmax_t modelFileSize, std::uintmax_t compressionFileSize) : 
                            formulaSize(formulaSize), modelSize(modelSize), variablesSize(variablesSize), modelFileSize(modelFileSize), compressionFileSize(compressionFileSize) {
        compressionRatio = (float) modelFileSize / compressionFileSize;
    }

    void addNames(const std::string formulaName_, const std::string modelName_) {
        formulaName = formulaName_;
        modelName = modelName_;
    }

};

class StatsOutput {
    private:
        const std::vector<CompressionInfo> statistics;
        double avgModelSize;
        double avgModelFileSize;
        double avgCompressedSize;
        double geometricMean;
        double ratioMedian;

    public:
        explicit StatsOutput(const std::vector<CompressionInfo> statistics) : statistics(statistics) {
            //calculate average model size, average compressed model size and geometric mean of the compression ratios
            avgModelSize = 0;
            avgModelFileSize = 0;
            avgCompressedSize = 0;
            geometricMean = 1.0;
            for (CompressionInfo stat: statistics) {
                avgModelSize += stat.modelSize;
                avgModelFileSize += stat.modelFileSize;
                avgCompressedSize += stat.compressionFileSize;
                geometricMean *= std::pow(stat.compressionRatio, 1.0/statistics.size());

            }

            avgModelSize = avgModelSize / statistics.size();
            avgModelFileSize = avgModelFileSize / statistics.size();
            avgCompressedSize = avgCompressedSize / statistics.size();

            //calculate the median
            std::vector<CompressionInfo> statisticsCopy = statistics;

            std::sort(statisticsCopy.begin(), statisticsCopy.end(), [](const CompressionInfo &a, const CompressionInfo &b){
            return a.compressionRatio < b.compressionRatio;
            });

            std::size_t middle = statisticsCopy.size() / 2;
            ratioMedian = 0;

            if(statisticsCopy.size() % 2 != 0) {
                ratioMedian = statisticsCopy[middle].compressionRatio;
            } else {
                ratioMedian = (statisticsCopy[middle - 1].compressionRatio + statisticsCopy[middle].compressionRatio) / 2.0;
            }
        }

        void printStatistics() {
            //print the headers
            std::cout << std::left << std::setw(36) << "Instance:" << std::setw(40) << "Model:" << std::setw(10) << "Clauses" << std::setw(10) << "Vars" << std::setw(10) << "Model" << std::setw(10) << "File" << std::setw(10) << "Compr." << std::setw(10) << "Compr." << std::endl;
            std::cout << std::left << std::setw(76) << "" << std::setw(10) << "count:" << std::setw(10) << "count:" << std::setw(10) << "size:" << std::setw(10) << "size:" << std::setw(10) << "size:" << std::setw(10) << "ratio:" << std::endl;

            //print the values
            for (CompressionInfo stat: statistics) {
                std::cout << std::left << std::setw(36) << stat.formulaName << std::setw(40) << stat.modelName << std::setw(10) << stat.formulaSize << std::setw(10) << stat.variablesSize << std::setw(10)
                << stat.modelSize << std::setw(10) << stat.modelFileSize << std::setw(10) << stat.compressionFileSize << std::setw(10) << stat.compressionRatio << std::endl;
            }

            //print the general statistics
            std::cout << "\nAverage model size: " << avgModelSize << std::endl;
            std::cout << "Average model file size: " << avgModelFileSize << std::endl;
            std::cout << "Average compressed file size: " << avgCompressedSize << std::endl;
            std::cout << "Geometric mean of compression ratios: " << geometricMean << std::endl;
            std::cout << "Median of compression ratio: " << ratioMedian << std::endl;
        }

        void writeToCsv(const char* filePath) {
            std::ofstream outputFile(filePath);

            //write the headers
            outputFile << "Instance, Model, Clauses count, Variables count, Model variable count, Model file size, Compressed file size, Compression ratio: \n";

            //write the values
            for (CompressionInfo stat: statistics) {
                outputFile << stat.formulaName << ", " << stat.modelName << ", " << stat.formulaSize << ", " << stat.variablesSize << ", " << stat.modelSize << ", " << stat.modelFileSize << ", " << stat.compressionFileSize << ", " << stat.compressionRatio << "\n";
            }

            //write general statistics
            outputFile << "\nAverage model file size:, " << avgModelFileSize << "\nAverage compressed file size:, " << avgCompressedSize << "\nGeometric mean of compression ratios:, " << geometricMean << "\nMedian of compression ratio:, " << ratioMedian;
        }
};

#endif