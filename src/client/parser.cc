// Copyright [2013] (JoenChen)

#include "./parser.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cctype>
#include <boost/regex.hpp>

int newscl::Preprocessor(std::string &code){
	//boost::cmatch what, what2;
	//define
	boost::regex defineLine("#define \\w*\\s*([^\\n\\r]+)");
	std::map<std::string, std::string> dict;
	std::string::const_iterator start = code.begin();
	std::string::const_iterator end = code.end();
	boost::match_results<std::string::const_iterator> what, what2;


	while (boost::regex_search(start, end, what, defineLine, boost::match_default)) {
		const std::string def(what[0]);
		//std::cout << def <<std::endl;
		boost::regex e("#define (\\w*)\\s*(.*)");
		std::string::const_iterator start2 = def.begin();
		std::string::const_iterator end2 = def.end();

		//boost::regex_search(start2, end2, what2, e);
		boost::regex_search(def.begin(), def.end(), what2, e);
		dict[what2[1]] = what2[2];

		start = what[0].second;
	}

	//for (const auto &x : dict) {
		//std::cout << x.first << std::endl;
		//std::cout << x.second << std::endl;
		//std::cout << "-------------------" << std::endl;

		//boost::regex identifier(x.first);
		//code = boost::regex_replace(code, identifier, x.second);
	//}

	boost::regex e("\\b(counter32_t)\\b");
	code = boost::regex_replace(code, e, "*");
	// remove the comment
  //std::cout << code << std::endl;
	boost::regex comment("//[^\n]*\n");
	code = boost::regex_replace(code, comment, "");
  //std::cout << code << std::endl;
	
	// remove the GUID_ARG tag
	boost::regex GUID_ARG_TAG("\\b(GUID_ARG)\\b");
	code = boost::regex_replace(code, GUID_ARG_TAG, "");
}

bool newscl::IsPointer (const std::string &targ) {
	std::string arg = targ;

	//remove the C99 type Qualifier
	boost::regex typeQualifier("(const|restrict|volatile)");
	arg = boost::regex_replace(arg, typeQualifier, "");

	//is array?
	boost::regex arraySymbol("\\[");
	if(boost::regex_search(arg,arraySymbol)){
		//std::cout << "array" << std::endl;
		return 1;
	}

	//is pointer?
	boost::regex pointerSymbol("[*]\\s*\\w*\\s*$");
	if(boost::regex_search(arg, pointerSymbol)){
		//std::cout << "pointer" << std::endl;
		return 1;
	}
	//std::cout << "const" << std::endl;
	return 0;
}
int newscl::FindFuncArgList(const std::string &code,
                    const std::string &name,
                    std::vector<std::string> &arg_list){
	boost::regex e("\\b" + name + "\\b\\s*\\(([^)]*)\\)\\s*\\{");
	boost::cmatch what;
	if (boost::regex_search(code.c_str(), what, e)) {
		assert(what.size() == 2);
		std::string result = what[1] + ",";
		char *pch;
		pch = strtok(const_cast<char*>(result.c_str()), ",");
		while (pch != NULL) {
			boost::regex e2("\\s+");
			arg_list.push_back(boost::regex_replace(std::string(pch), e2, " "));
			pch = strtok(NULL, ",");
		}
	}
}
