CXXFLAGS := -fPIC --std=c++0x -g

all: libelastic.so

clean:
	rm *.o
	rm *.so

cl_icd.o: ../src/client/cl_icd.cc ../src/client/cl_api.h ../include/newscl.h
	g++ $(CXXFLAGS) -c $< -I ../include -o $@

#newscl.o: ../src/client/newscl.cc ../src/client/cl_api.h ../include/newscl.h
#	g++ $(CXXFLAGS) -c $< -I ../include -o $@

IO.o : ../src/common_client/IO.cc ../include/IO.h
	g++ $(CXXFLAGS) -c $< -I ../include -o $@

IO_client.o: ../src/common_client/IO_client.cc ../include/IO.h
	g++ $(CXXFLAGS) -c $< -I ../include -o $@

parser.o: ../src/client/parser.cc ../src/client/parser.h
	g++ $(CXXFLAGS) -c $< -I ../include -o $@

elastic.o: ../src/client/elastic.cc ../src/client/elastic.h ../include/newscl.h 
	g++ $(CXXFLAGS) -c $< -I ../include -o $@

libelastic.so: IO_client.o IO.o parser.o elastic.o 
	g++ -g -shared $^ -lpthread -lboost_system -lboost_regex -o $@

test: cl_icd.o newscl.o IO_client.o
	g++ -g cl_icd.o newscl.o IO_client.o -lpthread -lboost_system -o ./test
