#ifndef SRC_UTIL_OUTPUT_H
#define SRC_UTIL_OUTPUT_H


struct CompressionInfo {
    std::string formulaName;
    std::string modelName;

    unsigned int formulaSize;
    unsigned int modelSize;
    unsigned int variablesSize;
    unsigned int compressedModelSize;
    float compressionRatio;

    explicit CompressionInfo(std::size_t formulaSize, std::size_t modelSize, std::size_t variablesSize, std::size_t compressedModelSize) : 
                            formulaSize(formulaSize), modelSize(modelSize), variablesSize(variablesSize), compressedModelSize(compressedModelSize) {
        compressionRatio = (float) modelSize / compressedModelSize;
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
        double avgCompressedSize;
        double geometricMean;
        double ratioMedian;

    public:
        explicit StatsOutput(const std::vector<CompressionInfo> statistics) : statistics(statistics) {
            //calculate average model size, average compressed model size and geometric mean of the compression ratios
            avgModelSize = 0;
            avgCompressedSize = 0;
            geometricMean = 1.0;
            for (CompressionInfo stat: statistics) {
                avgModelSize += stat.modelSize;
                avgCompressedSize += stat.compressedModelSize;
                geometricMean *= std::pow(stat.compressionRatio, 1.0/statistics.size());

            }

            avgModelSize = avgModelSize / statistics.size();
            avgCompressedSize = avgCompressedSize / statistics.size();

            //calculate the median
            std::vector<CompressionInfo> statisticsCopy = statistics;

            std::sort(statisticsCopy.begin(), statisticsCopy.end(), [](const CompressionInfo &a, const CompressionInfo &b){
            return a.compressionRatio < b.compressionRatio;
            });

            std::size_t middle = statisticsCopy.size() / 2;
            float median = 0;

            if(statisticsCopy.size() % 2 != 0) {
                median = statisticsCopy[middle].compressionRatio;
            } else {
                median = (statisticsCopy[middle - 1].compressionRatio + statisticsCopy[middle].compressionRatio) / 2.0;
            }
        }

        void printStatistics() {
            //print the headers
            std::cout << std::left << std::setw(36) << "Instance:" << std::setw(40) << "Model:" << std::setw(10) << "Clauses" << std::setw(10) << "Vars" << std::setw(10) << "Model" << std::setw(10) << "Compr." << std::setw(10) << "Compr." << std::endl;
            std::cout << std::left << std::setw(76) << "" << std::setw(10) << "count:" << std::setw(10) << "count:" << std::setw(10) << "size:" << std::setw(10) << "size:" << std::setw(10) << "ratio:" << std::endl;

            //print the values
            for (CompressionInfo stat: statistics) {
                std::cout << std::left << std::setw(36) << stat.formulaName << std::setw(40) << stat.modelName << std::setw(10) << stat.formulaSize << std::setw(10) << stat.variablesSize << std::setw(10)
                << stat.modelSize << std::setw(10) << stat.compressedModelSize << std::setw(10) << stat.compressionRatio << std::endl;
            }

            //print the general statistics
            std::cout << "\nAverage model size: " << avgModelSize << std::endl;
            std::cout << "Average compressed model size: " << avgCompressedSize << std::endl;
            std::cout << "Geometic mean of compression ratios: " << geometricMean << std::endl;
            std::cout << "Median of compression ratio: " << ratioMedian << std::endl;
        }
};

#endif