all:
	g++ -std=c++20 -pthread -g main.cpp -o coherence utils/processor.cpp utils/bus.cpp utils/lru_cache.cpp
clean:
	rm -rf coherence