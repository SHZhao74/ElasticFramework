// Copyright [2013] (JoenChen: joen@joen.cc)
#ifndef NEWSCL_CN_CN_INFO_H_
#define NEWSCL_CN_CN_INFO_H_
#include <memory>

#include "pthread.h"

#include "config.h"

namespace newscl {
  class CNInfo {
    public:
      typedef int HASH_KEY_T;

    private:
      int id_;
      double weight_;
      double used_time_;
      double capacity_;
      const int port_, non_blocking_IO_port_;
      const std::string IP_;
      mutable pthread_mutex_t mutex_;

    public:
      CNInfo(
             const decltype(weight_) &t_weight,
             const decltype(used_time_) &t_used_time,
             const decltype(capacity_) &t_capacity,
             const decltype(port_)& t_port = kCNPort,
             const decltype(non_blocking_IO_port_) &t_non_blocking_IO_port =
               kCNNonBlockingPort,
             const decltype(IP_) &t_IP = kCNIP)
           : weight_(t_weight),
             used_time_(t_used_time),
             capacity_(t_capacity),
             port_(t_port),
             non_blocking_IO_port_(t_non_blocking_IO_port),
             IP_(t_IP),
             mutex_(PTHREAD_MUTEX_INITIALIZER) {
      }
      static std::unique_ptr<CNInfo>& Instance(
          const decltype(weight_) t_weight = 0.f,
          const decltype(used_time_) t_used_time = 0.f,
          const decltype(capacity_) t_capacity = 0.f);

      inline void Lock() const { pthread_mutex_lock(&mutex_); }
      inline void Unlock() const { pthread_mutex_unlock(&mutex_); }

      // mutator
      inline void set_used_time(const decltype(used_time_) &val) {
        used_time_ = val;
      }
      inline void set_id(const decltype(id_) &id) { id_ = id; }
      inline void set_weight(const decltype(weight_) &val) {
        Lock();
        {
          weight_ = val;
        }
        Unlock();
      }

      // accestor
      inline decltype(id_) id() const { return id_; }
      inline decltype(weight_) weight() const {
        double weight = -1.f;
        Lock();
        {
          weight = weight_;
        }
        Unlock();
        return weight;
      }
      inline decltype(used_time_) used_time() const { return used_time_; }
      inline decltype(capacity_) capacity() const { return capacity_; }
      inline decltype(port_) port() const { return port_; }
      inline decltype(non_blocking_IO_port_) non_blocking_IO_port() const {
        return non_blocking_IO_port_;
      }
      inline decltype(IP_) IP() const { return IP_; }
  };
}  // namespace newscl
#endif  // NEWSCL_CN_CN_INFO_H_
