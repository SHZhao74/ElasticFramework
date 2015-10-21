#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "HistoryFileIO.h"
using namespace std;
HistoryFileIO::HistoryFileIO(char* exeFileName, char *networkFileName){
	setFileName(exeFileName, networkFileName);
}
void HistoryFileIO::setFileName(char* exeFileName, char *networkFileName){
	int i;
	for(i = 0; exeFileName[i]!='\0'; i++){
		this->exeFileName[i] = exeFileName[i];
	}
	this->exeFileName[i] = '\0';
	
	for(i = 0; networkFileName[i]!='\0'; i++){
		this->networkFileName[i] = networkFileName[i];
	}
	this->networkFileName[i] = '\0';
}
float HistoryFileIO::getExeTime(string appName, float argv){
	fstream file;
	file.open(exeFileName, std::fstream::in);
	if(!file) {
		cout << "Failed to open file" << endl;
	} else {
		string line;
		char buffer[30];

		while(getline(file, line)){
			strcpy(buffer, line.c_str());
			char * token = strtok(buffer, " ");
			while(token != NULL){
			   if(atoi(token) == argv){
			   	   token = strtok(NULL, " ");
				   return atof(token);
				   break;
			  } else {
				   token = strtok(NULL, " ");
			   }
			}	
		}
	}
	return 0.0;
	file.close();
}
float HistoryFileIO::getExeStdev(string appName, float argv){
	fstream file;
	file.open(exeFileName, std::fstream::in);
	if(!file) {
		cout << "Failed to open file" << endl;
	} else {
		string line;
		char buffer[30];

		while(getline(file, line)){
			strcpy(buffer, line.c_str());
			char * token = strtok(buffer, " ");
			while(token != NULL){
			   if(atoi(token) == argv){
			   	   token = strtok(NULL, " ");
				   token = strtok(NULL, " ");
				   return atof(token);
				   break;
			  } else {
				   token = strtok(NULL, " ");
			   }
			}	
		}
	}
	return 0.0;
	file.close();
}
float HistoryFileIO::getNetworkTime(int client_id, float argv){
	fstream file;
	file.open(networkFileName, fstream::in);
	if(!file) {
		cout << "Failed to open file" << endl;
	} else {
		string line;
		char buffer[30];

		while(getline(file, line)){
			strcpy(buffer, line.c_str());
			char * token = strtok(buffer, " ");
			while(token != NULL){
			   if(atoi(token) == argv){
				   token = strtok(NULL, " ");
				   return atof(token);
				   break;
			  } else {
				   token = strtok(NULL, " ");
			   }
			}	
		}
	}
	return 0.0;
	file.close();
}
float HistoryFileIO::getNetworkStdev(int client_id, float argv){
	fstream file;
	file.open(networkFileName, std::fstream::in);
	if(!file) {
		cout << "Failed to open file" << endl;
	} else {
		string line;
		char buffer[30];

		while(getline(file, line)){
			strcpy(buffer, line.c_str());
			char * token = strtok(buffer, " ");
			while(token != NULL){
			   if(atoi(token) == argv){
			   	   token = strtok(NULL, " ");
				   token = strtok(NULL, " ");
				   return atof(token);
				   break;
			  } else {
				   token = strtok(NULL, " ");
			   }
			}	
		}
	}
	return 0.0;
	file.close();
}




