#!/bin/bash

protocols=("MESI")
benchmarks=("blackscholes" "bodytrack" "fluidanimate")
cache_sizes=(16384 65536)
associativities=(4)
block_sizes=(32 64)

for protocol in "${protocols[@]}"; do
    for benchmark in "${benchmarks[@]}"; do
        for cache_size in "${cache_sizes[@]}"; do
            for associativity in "${associativities[@]}"; do
                for block_size in "${block_sizes[@]}"; do
                    ./coherence "$protocol" "$benchmark" "$cache_size" "$associativity" "$block_size"
                    ./coherence "$protocol" "$benchmark" "$cache_size" "$associativity" "$block_size" "optimized"
                done
            done
        done
    done
done
