.PHONY: build run clean

build:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build -j
	ln -sf build/compile_commands.json compile_commands.json

run: build
	xhost +local:
	sudo -E ./build/NetPulseMonitor

clean:
	rm -rf build