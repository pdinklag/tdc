# Engineering Predecessor Data Structures for Dynamic Integer Sets

This is the supplementary code repository for our SEA 2021 submission entitled *Engineering Predecessor Data Structures for Dynamic Integer Sets*. It contains the source code for our dynamic predecessor data structures that are the fastest and most memory efficient in our experiments.

Note that the repository is a stripped down version of a repository containing many other experimental data structures. For this reason, there may be code left that is not actually used in the context of our dynamic predecessor data structures.

The main source code for our data structures is contained in the `include/tdc/pred` directory.

## Building

This is a CMake project that should be built using a compiler supporting C++17 or later as well as the `__uint128_t` type. We used GCC version 9.3 with the highest optimization (`-O3`). In order to activate optimizations, make sure to build the project with the `Release` CMake build type.

We recommend the following command sequence, executed in the repository root, to build the project:

```shell
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

You may encounter warnings about `LEDA` and `STree` not being found. These are optional, refer to the *Dependencies* section below for details.

Note that compiling may take a minute as we make heavy use of templates.

### Requirements

Your CPU must support the [BMI2 instruction set](https://en.wikipedia.org/wiki/Bit_manipulation_instruction_set).

We have only tested the project on Linux operating systems and additional setup may be required for others.

### Dependencies

Our hard dependencies are included as submodules linked to other GitHub repositories and will automatically be checked out and built using the commands above.

The only exception is the stratisfied tree (STree) by Dementiev et al. It was released along with their 2004 ALENEX paper [*Engineering a Sorted List Data Structure for 32 Bit Key*](http://algo2.iti.kit.edu/dementiev/files/veb.pdf), but without any license information so that we cannot include it in this repository. Furthermore, their code was written in a very old version of C++ and required some changes, as well as LEDA (the [free version](https://www.algorithmic-solutions.com/index.php/products/leda-free-edition) 6.3 or higher suffices).

The [URL stated in their paper](http://www.mpi-sb.mpg.de/~kettner/proj/veb/) appears to be dead by now, so we offer to provide a copy in direct contact.

## Results

The raw results of our experiment are contained in the file `results/pred_dynamic.txt`.

The output is in a format that can be processed by Bingmann's [sqlplot-tools](https://github.com/bingmann/sqlplot-tools), but is also human readable. In our paper, we refer to the following fields:

* `algo` - the data structure in question.
* `time_insert` - the runtime in milliseconds for inserting *n* keys.
* `time_predecessor_rnd` - the runtime in milliseconds for performing `q` random predecessor queries.
* `time_delete` - the runtime in milliseconds for deleting the *n* keys.
* `memData` - the heap size of the data structure, in bytes,  after inserting `n` keys.

## Reproducing Experiments

After building the project, the `bench_predecessor_dynamic` executable can be used to run our benchmarks. There are several modes of which we only used the `basic` mode for our experiments, which  corresponds to the three-step experiments described in the *Methodology* section of our paper.

The following command line runs the experiment for all data structures for 2^20 keys drawn from a 32-bit universe, performing one million predecessor queries using the random seed 777:

```
 benchmark/bench_predecessor_dynamic basic -u 32 -n 1Mi -q 1M -s 777
```

Passing `--help` following `basic` will list the available parameters.

#### Random Seeds

We used the following five random seeds mentioned in the *Methodology* section:

1. `34989010171594391`
2. `2086595458081381017`
3. `3771623990908855579`
4. `7857122929249898776`
5. `8974570631975370903`

Note these are just random numbers drawn uniformly from a 64-bit universe with no special properties; we generated a batch and picked the first five. In the list, they are ordered for purely aesthetic reasons.
