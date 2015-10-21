#include <iostream>       // std::cout
#include <string>         // std::string
#include <json/json.h>
#include <fstream>
using namespace std;
int ParseJsonFromString()  
{  
  const char* str = "{\"uploadid\": \"UP000000\",\"code\": 100,\"msg\": \"\",\"files\": \"\"}";  
  
  Json::Reader reader;  
  Json::Value root;  
  if (reader.parse(str, root))  // reader將Json字符串解析到root，root將包含Json裡所有子元素  
  {  
    std::string upload_id = root["uploadid"].asString();  // 訪問節點，upload_id = "UP000000"  
    int code = root["code"].asInt();    // 訪問節點，code = 100  
  }  
  return 0;  
} 
int ParseJsonFromFile(const char* filename)  
{  
  // 解析json用Json::Reader  
  Json::Reader reader;  
  // Json::Value是一種很重要的類型，可以代表任意類型。如int, string, object, array...  
  Json::Value root;         
  
  std::ifstream is;  
  is.open (filename, std::ios::binary );    
  if (reader.parse(is, root))  
  {  
    std::string code;  
    if (!root["files"].isNull())  // 訪問節點，Access an object value by name, create a null member if it does not exist.  
      code = root["uploadid"].asString();  
      
    // 訪問節點，Return the member named key if it exist, defaultValue otherwise.  
    code = root.get("uploadid", "null").asString();  
  
    // 得到"files"的數組個數  
    int file_size = root["files"].size();  
  	cout << "file_size:"<<file_size<<endl;
    // 遍曆數組  
    for(int i = 0; i < file_size; ++i)  
    {  
      Json::Value val_image = root["files"][i]["images"];  
      int image_size = val_image.size();
      cout << "image_size:"<<image_size<<endl;  
      for(int j = 0; j < image_size; ++j)  
      {  
        std::string type = val_image[j]["type"].asString();  
        std::string url = val_image[j]["url"].asString();  
      }  
    }  
  }  
  is.close();  
  return 0;  
}
int ParseHD(const char* filename)  
{  
  // 解析json用Json::Reader  
  Json::Reader reader;  
  // Json::Value是一種很重要的類型，可以代表任意類型。如int, string, object, array...  
  Json::Value root;         
  
  std::ifstream is;  
  is.open (filename, std::ios::binary );    
  if (reader.parse(is, root))  
  {  
    std::string code;  
    //if (!root["apps"].isNull())  // 訪問節點，Access an object value by name, create a null member if it does not exist.  
      //code = root["uploadid"].asString();  
      
    // 訪問節點，Return the member named key if it exist, defaultValue otherwise.  
    //code = root.get("uploadid", "null").asString();  
  
    // 得到"files"的數組個數  
    int apps_size = root["apps"].size();  
  	cout << "apps_size:"<<apps_size<<endl;
    // 遍曆數組  
    for(int i = 0; i < apps_size; ++i)  
    {  
      Json::Value val_image = root["apps"][i]["history"];  
      int history_size = val_image.size();
      cout << "history_size:"<<history_size<<endl;  
      for(int j = 0; j < history_size; ++j)  
      {  
        int argv = val_image[j]["argv"].asInt(); printf("argv =%d\n", argv);
        int exeTime = val_image[j]["exeTime"].asInt(); printf("exeTime =%d\n", exeTime);
        int exeStv = val_image[j]["exeStv"].asInt(); printf("exeStv =%d\n", exeStv);
      }  
    }  
  }  
  is.close();  
  return 0;  
}

int main() {
	ParseHD("hd.json");
	//ParseJsonFromFile("test.json");
    //ParseJsonFromString(); 
    return 0;
}