#include <iostream>       // std::cout
#include <string>         // std::string
#include <fstream>

//using namespace std;


int main(int argc, char const *argv[])
{
	//string appName="NTU";  
	FILE* fp = fopen("history.json", "w");

	for (int i = 0; i <512 ; ++i)
	{
		fprintf(fp,"%s", std::to_string(6).data());
	}
	fclose(fp);
	return 0;
}
