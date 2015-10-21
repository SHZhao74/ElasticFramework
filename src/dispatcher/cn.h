// Copyright [2013] (JoenChen)
#ifndef NEWSCL_DISPATCHER_CN_H
#define NEWSCL_DISPATCHER_CN_H
#include <cassert>

#include <unordered_map>
#include <string.h>
#include <memory>
#include <list>
#include "newscl.h"
#include "./kernel.h"
#define KERNEL_MAX 2

namespace newscl {
  struct DevInfo {
    const unsigned int dev_type_;
    const std::string dev_name_;
    const double capacity_;
    DevInfo(const decltype(dev_type_) & t_dev_type,
            const decltype(dev_name_) & t_dev_name,
            const decltype(capacity_) & t_capacity)
        : dev_type_(t_dev_type),
          dev_name_(t_dev_name),
          capacity_(t_capacity) {
    }
  };

  typedef std::list<DevInfo> DevList;

  class CNInfo {
    public:
      typedef int HASH_KEY_T;

    private:
      double weight_;
      double used_time_;
      double capacity_;
      const int port_, non_blocking_IO_port_;
      const std::string IP_;
      DevList devs_;  // to be discussed

      //add by zhao
      //std::unordered_map<string, kInfo> kernels_;

    public:
      CNInfo(
             const decltype(weight_) &t_weight,
             const decltype(used_time_) &t_used_time,
             const decltype(capacity_) &t_capacity,
             const decltype(port_)& t_port,
             const decltype(non_blocking_IO_port_) &t_non_blocking_IO_port,

             const decltype(IP_) &t_IP

             )//const decltype(spend_time_) &spend_time
           : weight_(t_weight),
             used_time_(t_used_time),
             capacity_(t_capacity),
             port_(t_port),
             non_blocking_IO_port_(t_non_blocking_IO_port),
             IP_(t_IP){
            }
      //string AddKernel(newscl::kInfo kinfo);
      //string AddKernel(string name, float deadline);
      // mutator
      inline void set_used_time(const decltype(used_time_) &val) {
        used_time_ = val;
      }
      inline void set_weight(const decltype(weight_) &val) { weight_ = val; }


      // accestor
      inline decltype(weight_) weight() const { return weight_; }
      inline decltype(used_time_) used_time() const { return used_time_; }
      inline decltype(capacity_) capacity() const { return capacity_; }
      inline decltype(port_) port() const { return port_; }
      inline decltype(non_blocking_IO_port_) non_blocking_IO_port() const {
        return non_blocking_IO_port_;
      }
      inline decltype(IP_) IP() const { return IP_; }

  };

  class CNList {
    pthread_mutex_t lock_;
    //std::unordered_map<CNInfo::HASH_KEY_T, CNInfo> CNs_;
      

    public:
      CNList() : lock_(PTHREAD_MUTEX_INITIALIZER) {}
      std::unordered_map<CNInfo::HASH_KEY_T, CNInfo> CNs_;



      static std::unique_ptr<CNList>& Instance();
      inline void Lock() { pthread_mutex_lock(&lock_); }
      inline void Unlock() { pthread_mutex_unlock(&lock_); }
      CNInfo::HASH_KEY_T AddCN(const CNInfo& cn);
      CNInfo::HASH_KEY_T AddCN(const std::string IP,
                               const double &capacity,
                               const double &weight = 0.f,
                               const double &used_time = 0.f);
      void DelCN(const CNInfo::HASH_KEY_T &key);
      CNInfo& GetCN(const CNInfo::HASH_KEY_T &key);
      int UpdateCNWeight(CNInfo::HASH_KEY_T const &key,
                         double const &weight);
      int UpdateCNUsedTime(CNInfo::HASH_KEY_T const &key,
                           double const &used_time);

      //add by zhao
      int UpdateCNScore(CNInfo::HASH_KEY_T const &key);


      // accesor
      inline decltype(CNs_.cbegin()) cbegin() const { return CNs_.cbegin(); }
      inline decltype(CNs_.cend()) cend() const { return CNs_.cend(); }
  };
}  // namespace NEWSCL
#endif  // NEWSCL_DISPATCHER_CN_H
