#!/bin/bash

protocols=("MESI" "Dragon")
benchmarks=("blackscholes" "bodytrack" "fluidanimate")
cache_sizes=(1024 4096 16384 65536 131072)
associativities=(1 2 4 8)
block_sizes=(4 32 64 128)

for protocol in "${protocols[@]}"; do
    for benchmark in "${benchmarks[@]}"; do
        for cache_size in "${cache_sizes[@]}"; do
            for associativity in "${associativities[@]}"; do
                for block_size in "${block_sizes[@]}"; do
                    ./coherence "$protocol" "$benchmark" "$cache_size" "$associativity" "$block_size"
                done
            done
        done
    done
done

for protocol in "${protocols[@]}"; do
    for benchmark in "${benchmarks[@]}"; do
        for cache_size in "${cache_sizes[@]}"; do
            for block_size in "${block_sizes[@]}"; do
                associativity=$((cache_size / block_size))
                ./coherence "$protocol" "$benchmark" "$cache_size" "$associativity" "$block_size"
            done
        done
    done
done
