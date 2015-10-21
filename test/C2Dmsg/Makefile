CXXFLAGS := -fPIC --std=c++0x -g
all:run

run: client.cc
	make -C ../../client_build
	g++ $(CXXFLAGS) client.cc -lOpenCL ../../client_build/libelastic.so -I /opt/AMDAPP/include -I../../include -I../../src/client/ -o run

clean:
	make -C ../../client_build clean
	rm run
	