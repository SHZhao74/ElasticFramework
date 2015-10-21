// Copyright [2013] (JoenChen: joen@joen.cc)
#ifndef NEWSCL_CONFIG_H_
#define NEWSCL_CONFIG_H_
#include <string>
namespace newscl {
  const std::string kDispatcherIP = "140.112.28.114";
	const int kDispatcherPort = 6788;

	// deprecated ?
	const std::string kCNIP = "127.0.0.1";//48.153
	const int kCNPort = 6789;
	const int kCNNonBlockingPort = 6790;
	const int kCNFinishPort = 6792;
};
#endif  // NEWSCL_CONFIG_H_
