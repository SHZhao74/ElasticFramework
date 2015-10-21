#include <string>
using namespace std;
class HistoryFileIO{
	private:
		char exeFileName[20];
		char networkFileName[20];
	public:
		HistoryFileIO(char* exeFileName, char *networkFileName);
		void setFileName(char* exeFileName, char *networkFileName);
		float getExeTime(string appName, float argv);
		float getExeStdev(string appName, float argv);
		float getNetworkTime(int client_id, float argv);
		float getNetworkStdev(int client_id, float argv);
};
