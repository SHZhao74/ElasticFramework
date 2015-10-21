// Copyright <2013> (Joen Chen)
#ifndef NEWSCL_DISPATCHER_VMAFS_N_DISPATCHER_H_
#define NEWSCL_DISPATCHER_VMAFS_N_DISPATCHER_H_
#include <unordered_map>
#include <string>
#include <list>
#include <memory>
#include <cassert>

#include "boost/utility.hpp"

#include "./cn.h"
#include "newscl.h"

namespace newscl {
  class VMAFS_N_Dispatcher: private boost::noncopyable {
    pthread_mutex_t sched_proc_lock_;
    double based_inner_product_;

    // v vector
    double v_leng_square_;
    // the ideal vector
    double i_leng_square_;
    VMAFS_N_Dispatcher();
    friend class CNList;

    public:

      // manipulator
      static std::unique_ptr<VMAFS_N_Dispatcher>& Instance();
      inline void Lock() { pthread_mutex_lock(&sched_proc_lock_); }
      inline void Unlock() { pthread_mutex_unlock(&sched_proc_lock_); }
      CNInfo::HASH_KEY_T Dispatch(const double& app_weight);

      // mutator
      int set_CN_weight(const CNInfo::HASH_KEY_T &id, const double &weight);
      int set_CN_used_time(const CNInfo::HASH_KEY_T &id,
                           const double &used_time);
      // accessor
  };

  inline VMAFS_N_Dispatcher::VMAFS_N_Dispatcher()
      : sched_proc_lock_(PTHREAD_MUTEX_INITIALIZER),
        based_inner_product_(0.f),
        v_leng_square_(0.f),
        i_leng_square_(0.f) {
  }
}  // namespace NEWSCL

#endif  // NEWSCL_DISPATCHER_VMAFS_N_DISPATCHER_H_
