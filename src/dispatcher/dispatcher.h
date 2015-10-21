//Copyright [2013] (JoenChen)
#ifndef NEWSCL_DISPATCHER_DISPATCHER_H_
#define NEWSCL_DISPATCHER_DISPATCHER_H_
#include <map>
namespace newscl {
  namespace dispatcher {
    namespace client_type{
      const int kApp = 0;
      const int kCN = 1;
    };
  }
  struct ClientInfo
  {
    double weight;
    ClientInfo(decltype(weight) t_weight) : weight(t_weight) {}
  };
  extern std::map<unsigned long int, ClientInfo> client_info;
};
#endif //NEWSCL_DISPATCHER_DISPATCHER_H_
