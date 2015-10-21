CXXFLAGS := -fPIC --std=c++0x -g

all: Source test2

Source:
	g++ -c BilateralFilter.cpp -o BilateralFilter.o 
	g++ BilateralFilter.o Source.cpp  -I/opt/AMDAPP/include -L./include/TempSDKUtil/lib/x86_64 -L/opt/AMDAPP/lib/x86_64/ -lOpenCL `pkg-config --libs opencv` -g -o Source

#test:
#	g++ $(CXXFLAGS) /home/newslab/newscl/Client/client/NEWSCL3/build/elastic.so BilateralFilter.o Source2.cpp -L./include/TempSDKUtil/lib/x86_64 -I/home/newslab/newscl/Client/client/NEWSCL3/src/client/ -I/home/newslab/newscl/Client/client/NEWSCL3/include `pkg-config --libs opencv` -o test
test2: Source3.cpp
	g++ $(CXXFLAGS) ../../client_build/libelastic.so BilateralFilter.o Source3.cpp -L./include/TempSDKUtil/lib/x86_64 -I../../src/client/ -I../../include `pkg-config --libs opencv` -lpthread -o test2


clean:
	rm -f Source test2
