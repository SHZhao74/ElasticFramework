// Copyright [2013] (JoenChen:joen@joen.cc)
#ifndef NEWSCL_CLIENT_PARSER_H_
#define NEWSCL_CLIENT_PARSER_H_
#include <string>
#include <vector>
namespace newscl {
  int Preprocessor(std::string &code);
  bool IsPointer (const std::string &targ);
  int FindFuncArgList(const std::string &code,                             
                              const std::string &name,                                     
                              std::vector<std::string> &arg_list);
}  // namespace newscl
#endif //NEWSCL_CLIENT_PARSER_H_
