# Algorithm for the compression of propositional models
This repository contains algorithms to compress and decompress propositional models. It uses unit propagation in combination with other techniques to achieve compression of the models.
# Setup

## Prerequisites
Note that the code has only been testes on Linux systems.
- Boost
- libarchive
- lz4
- pkg-config

## Building
    mkdir build
    cd build
    cmake ..
    make

# Usage
    ./compression path_to_formula path_to_model path_to_output_file [parameters]
    ./decompression path_to_formula path_to_compressed_model path_to_output_file [parameters]

## Input formats
The formulas have to be given in the DIMACS CNF format and the models have to be formatted in the same way as specified in the Output Format of the [SAT Competition](https://satcompetition.github.io/2024/output.html).

## Use with single files
In order to compress or decompress a single file the formula path must lead to a single cnf file and the model and output path also must lead to a single file.

## Use with multiple files
To compress and decompress multiple models at once, the formula path must lead to a directory that contains the .cnf files. \
 The models (or compressed models) for each formula have to be strored in the following way:

```
models 
│
└───name_of_first_formula
│   │   model1
│   │   model2
│   
└───name_of_second_formula
|   │   model1
|   │   model2
|
└───...
```

The names of the subdirectories have to be identcal to the names of the .cnf files in the formula directory. The path that is given to the program must lead to the models directory. \
 The output path must lead to a directory in which the compressed models are stored in the same structure as the input. The subdirectories are created automatically by the program.

 ## Parameters
 The algorithms can be configured using multiple parameters. **The parameters must be the same for the compression and decompression in order to decompress correctly.**  

| Parameter | Description                       | Possible values | Default value |
| --------- | -----------                       | --------------- | ------------- |
| -h        | Ordering heuristic                | <ul><li>**none**: No heuristic</li><li>**jewa**: Jeroslow-Wang static</li><li>**jewa_dyn**: Jeroslow-Wang dynamic</li><li>**moms**: MOMS static</li><li>**moms_dyn**: MOMS dynamic</li><li>**hybr**: Hybrid heuristic static</li><li>**hybr_dyn**: Hybrid heuristic dynamic</li></ul>| Jeroslow-Wang dynamic |
| -c        | Generic compression algorithm     | <ul><li>**golrice**: Golomb-Rice coding</li><li>**zip**: ZIP compression</li><li>**lz4**: LZ4 compression</li></ul>| Golomb-Rice coding |
| -mp       | MOMS heuristic parameter <br> (Only necessary if MOMS is selected)        | Any double value | 10.0 |
| -grp      | Golomb-Rice compression parameter <br> (Only necessary if Golomb-Rice is selected) | Integer value, must be a power of two | 2 |
| -p        | Prediction model inversion value  | Any positive integer value | 5 |
| -hp       | Hybrid heuristic cutoff parameter | Any positive integer value | TODO |
