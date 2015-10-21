#include "json/json.h"
#include <string>
using namespace std;

int Parse()
{
 std::string strValue = "{\"key1\":\"value1\","
	                      "\"array\":["
						  	"{\"key2\":\"value2\"},"
						    "{\"key2\":\"value3\"},"
							"{\"key2\":\"value4\"}"
                          "]}"
						"{\"key1\":\"value5\","
	                      "\"array\":["
						  	"{\"key2\":\"value6\"},"
						    "{\"key2\":\"value7\"},"
							"{\"key2\":\"value8\"}"
                          "]}";
 Json::Reader reader;
 Json::Value value;
 if (reader.parse(strValue, value))
 {
  std::string out = value["key1"].asString();
  std::cout << out << std::endl;
  const Json::Value arrayObj = value["array"];
  for (int i=0; i<arrayObj.size(); i++)
  {
   out = arrayObj[i]["key2"].asString();
   std::cout << out;
   if (i != arrayObj.size() - 1 )
   std::cout << std::endl;
  }
 }
 std::cout << std::endl << std::endl << std::endl;
 return 0;
}

int Build()
{
 Json::Value root;
 root["key1"] = "value1";
 root["key2"] = "value2";

 Json::Value arrayObj;
 Json::Value item;
 for (int i=0; i<10; i++)
 {
  item["key"] = i;
  arrayObj.append(item);
 }
 root["array"] = arrayObj;
 root.toStyledString();
 std::string out = root.toStyledString();
 std::cout << out << std::endl;
 return 0;
}


int main()
{
 Parse();
 Build();
}

