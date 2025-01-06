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
    unsigned int bitvectorSize;
    unsigned int diffEncodingSize;
    unsigned int nrPropDontCareVars;
    float compressionRatioFileSize;
    float compressionRatioBitvector;
    float predictionHitRate;
    double parsingTime;
    double overallTime;

    explicit CompressionInfo(std::size_t formulaSize, std::size_t modelSize, std::size_t variablesSize, std::uintmax_t modelFileSize, std::uintmax_t compressionFileSize, unsigned int bitvectorSize, unsigned int diffEncodingSize, unsigned int nrPropDontCareVars, float predictionHitRate, double parsingTime, double overallTime) : 
                            formulaSize(formulaSize), modelSize(modelSize), variablesSize(variablesSize), modelFileSize(modelFileSize), compressionFileSize(compressionFileSize), bitvectorSize(bitvectorSize), diffEncodingSize(diffEncodingSize), nrPropDontCareVars(nrPropDontCareVars), predictionHitRate(predictionHitRate), parsingTime(parsingTime), overallTime(overallTime) {
        compressionRatioFileSize = (float) modelFileSize / compressionFileSize;
        unsigned int bitvectorFileSize = 1 + ((modelSize - 1) / 8);
        compressionRatioBitvector = (float) bitvectorFileSize / compressionFileSize;
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
        double geometricMeanFileSize;
        double ratioMedianFileSize;
        double geometricMeanBitvector;
        double ratioMedianBitvector;
        double geometricMeanHitRate;
        double avgNrDontCareVars;
        double avgParsingTime;
        double avgOverallTime;

    public:
        explicit StatsOutput(const std::vector<CompressionInfo> statistics) : statistics(statistics) {
            //calculate average model size, average compressed model size and geometric mean of the compression ratios
            avgModelSize = 0;
            avgModelFileSize = 0;
            avgCompressedSize = 0;
            geometricMeanFileSize = 1.0;
            geometricMeanBitvector = 1.0;
            geometricMeanHitRate = 1.0;
            avgNrDontCareVars = 0;
            avgParsingTime = 0;
            avgOverallTime = 0;
            for (CompressionInfo stat: statistics) {
                avgModelSize += stat.modelSize;
                avgModelFileSize += stat.modelFileSize;
                avgCompressedSize += stat.compressionFileSize;
                geometricMeanFileSize *= std::pow(stat.compressionRatioFileSize, 1.0/statistics.size());
                geometricMeanBitvector *= std::pow(stat.compressionRatioBitvector, 1.0/statistics.size());
                if (stat.predictionHitRate != 0) {
                    geometricMeanHitRate *= std::pow(stat.predictionHitRate, 1.0/statistics.size());
                }
                avgNrDontCareVars += stat.nrPropDontCareVars;
                avgParsingTime += stat.parsingTime;
                avgOverallTime += stat.overallTime;

            }

            avgModelSize = avgModelSize / statistics.size();
            avgModelFileSize = avgModelFileSize / statistics.size();
            avgCompressedSize = avgCompressedSize / statistics.size();
            avgNrDontCareVars = avgNrDontCareVars / statistics.size();
            avgParsingTime = avgParsingTime / statistics.size();
            avgOverallTime = avgOverallTime / statistics.size();

            //calculate the median for the file size
            std::vector<CompressionInfo> statisticsCopy = statistics;

            std::sort(statisticsCopy.begin(), statisticsCopy.end(), [](const CompressionInfo &a, const CompressionInfo &b){
            return a.compressionRatioFileSize < b.compressionRatioFileSize;
            });

            std::size_t middle = statisticsCopy.size() / 2;
            ratioMedianFileSize = 0;

            if(statisticsCopy.size() % 2 != 0) {
                ratioMedianFileSize = statisticsCopy[middle].compressionRatioFileSize;
            } else {
                ratioMedianFileSize = (statisticsCopy[middle - 1].compressionRatioFileSize + statisticsCopy[middle].compressionRatioFileSize) / 2.0;
            }

            //calculate the median for the bitvector
            statisticsCopy = statistics;

            std::sort(statisticsCopy.begin(), statisticsCopy.end(), [](const CompressionInfo &a, const CompressionInfo &b){
            return a.compressionRatioBitvector < b.compressionRatioBitvector;
            });

            middle = statisticsCopy.size() / 2;
            ratioMedianBitvector = 0;

            if(statisticsCopy.size() % 2 != 0) {
                ratioMedianBitvector = statisticsCopy[middle].compressionRatioBitvector;
            } else {
                ratioMedianBitvector = (statisticsCopy[middle - 1].compressionRatioBitvector + statisticsCopy[middle].compressionRatioBitvector) / 2.0;
            }
        }

        void printStatistics() {
            //print the headers
            std::cout << std::left << std::setw(36) << "Instance:" << std::setw(40) << "Model:" << std::setw(10) << "Clauses" << std::setw(10) << "Vars" << std::setw(10) << "Model" << std::setw(10) << "File" << std::setw(10) << "Compr." << std::setw(10) << "Compr." << std::setw(10) << "Bitvec" << std::setw(10) << "Predic" << std::setw(10) << "Pars." << std::setw(10) << "Exec." << std::setw(10) << "Bitvec" << std::setw(10) << "Diff." << std::setw(10) << "NrDC" << std::endl;
            std::cout << std::left << std::setw(76) << "" << std::setw(10) << "count:" << std::setw(10) << "count:" << std::setw(10) << "size:" << std::setw(10) << "size:" << std::setw(10) << "size:" << std::setw(10) << "ratio:" << std::setw(10) << "ratio:" << std::setw(10) << "HitRt:" << std::setw(10) << "time:" << std::setw(10) << "time:" << std::setw(10) << "size:" << std::setw(10) << "size:" << std::setw(10) << "vars:" << std::endl;

            //print the values
            for (CompressionInfo stat: statistics) {
                std::cout << std::left << std::setw(36) << stat.formulaName << std::setw(40) << stat.modelName << std::setw(10) << stat.formulaSize << std::setw(10) << stat.variablesSize << std::setw(10)
                << stat.modelSize << std::setw(10) << stat.modelFileSize << std::setw(10) << stat.compressionFileSize << std::setw(10) << stat.compressionRatioFileSize << std::setw(10) << stat.compressionRatioBitvector << std::setw(10) << stat.predictionHitRate << std::setw(10) << stat.parsingTime << std::setw(10) << stat.overallTime << std::setw(10) << stat.bitvectorSize << std::setw(10) << stat.diffEncodingSize << std::setw(10) << stat.nrPropDontCareVars << std::endl;
            }

            //print the general statistics
            std::cout << "\nAverage model size: " << avgModelSize << std::endl;
            std::cout << "Average model file size: " << avgModelFileSize << std::endl;
            std::cout << "Average compressed file size: " << avgCompressedSize << std::endl;
            std::cout << "Geometric mean of compression ratios with file sizes: " << geometricMeanFileSize << std::endl;
            std::cout << "Median of compression ratio with file sizes: " << ratioMedianFileSize << std::endl;
            std::cout << "Geometric mean of compression ratios compared to a bitvector: " << geometricMeanBitvector << std::endl;
            std::cout << "Median of compression ratio compared to a bitvector: " << ratioMedianBitvector << std::endl;
            std::cout << "Geometric mean of prediction model hit rates: " << geometricMeanHitRate << std::endl;
            std::cout << "Average number of propagated don't care variables: " << avgNrDontCareVars << std::endl;
            std::cout << "Average parsing time per model: " << avgParsingTime << std::endl;
            std::cout << "Average execution time per model: " << avgOverallTime << std::endl;
        }

        void writeToCsv(const char* filePath) {
            std::ofstream outputFile(filePath);

            //write the headers
            outputFile << "Instance, Model, Clauses count, Variables count, Model variable count, Model file size, Compressed file size, Compression ratio file sizes, Compression ratio bitvector, Prediction model hit rate, Parsing time, Execution time, Bitvector size, Diff encoding size, Number of propagated don't care vars\n";

            //write the values
            for (CompressionInfo stat: statistics) {
                outputFile << stat.formulaName << ", " << stat.modelName << ", " << stat.formulaSize << ", " << stat.variablesSize << ", " << stat.modelSize << ", " << stat.modelFileSize << ", " << stat.compressionFileSize << ", " << stat.compressionRatioFileSize << ", " << stat.compressionRatioBitvector << ", " << stat.predictionHitRate
                           << ", " << stat.parsingTime << ", " << stat.overallTime << ", " << stat.bitvectorSize << ", " << stat.diffEncodingSize << ", " << stat.nrPropDontCareVars << "\n";
            }

            //write general statistics
            outputFile << "\nAverage model file size:, " << avgModelFileSize << "\nAverage compressed file size:, " << avgCompressedSize << "\nGeometric mean of compression ratios with file sizes:, " << geometricMeanFileSize << "\nMedian of compression ratio with file sizes:, " << ratioMedianFileSize
                       << "\nGeometric mean of compression ratios compared to a bitvector:, " << geometricMeanBitvector << "\nMedian of compression ratio compared to a bitvector:, " << ratioMedianBitvector << "\nGeometric mean of prediction model hit rates:, " << geometricMeanHitRate
                       << "\nNumber of propagated don't care variables:, " << avgNrDontCareVars << "\nAverage parsing time per model:, " << avgParsingTime << "\nAverage execution time per model:, " << avgOverallTime;
        }
};

#endif