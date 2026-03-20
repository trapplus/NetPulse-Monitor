.PHONY: build run clean

build:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
	cmake --build build -j

run: build
	xhost +local:
	sudo -E ./build/NetPulseMonitor

clean:
	rm -rf build