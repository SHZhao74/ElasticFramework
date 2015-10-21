#ifndef NEWSCL_DISPATCHER_HANDLER_H
#define NEWSCL_DISPATCHER_HANDLER_H
#include <array>
#include "newscl.h"

namespace newscl {
	void HandlerUpdateScore(const int &sock, std::array<char, newscl::kMsgMaxSize> &buf);
  	void HandlerGetBestCN(const int &sock,
                           std::array<char, newscl::kMsgMaxSize> &buf);
  void HandlerGetBindingCN(const int &sock,
                           std::array<char, newscl::kMsgMaxSize> &buf);
  int HandlerCNReg(const int &sock,
                   std::array<char, newscl::kMsgMaxSize> &buf);
  void HandlerLoadInfo(const int&sock,
                       std::array<char, newscl::kMsgMaxSize> &buf);
};

#endif //NEWSCL_DISPATCHER_HANDLER_H
