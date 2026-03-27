.PHONY: build run clean install_dev_env_arch install_dev_env_debian

build:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build -j
	ln -sf build/compile_commands.json compile_commands.json

run: build
	xhost +local:
	sudo -E ./build/NetPulseMonitor

clean:
	rm -rf build

install_dev_env_arch: clean
	sudo pacman --noconfirm -Syu clang cmake xorg-xhost libpcap curl

install_dev_env_debian:
	sudo apt update -y 
	sudo apt install -y libsfml-dev curl libpcap-dev x11-xserver-utils
