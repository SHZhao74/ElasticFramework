// Copyright [2013] (JoenChen)
#ifndef NEWSCL_CN_VMAFS_N_SCHEDULER_H_
#define NEWSCL_CN_VMAFS_N_SCHEDULER_H_
#include <pthread.h>

#include <memory>
#include <list>
#include <boost/utility.hpp>

#include "./command.h"
#include "./VM.h"

typedef std::list<std::shared_ptr<VMInfo> > VMList;

class VMAFS_N_Scheduler: private boost::noncopyable {
  pthread_mutex_t proc_lock_;
  VMList VMs_;
  double based_inner_product_;
  double weight_leng_square_;
  double time_leng_square_;

  public:
    VMAFS_N_Scheduler();
    static std::unique_ptr<VMAFS_N_Scheduler>& Instance();

    // manipulator
    std::shared_ptr<VMInfo> CreateVM(double weight);
    int DestroyVM(std::shared_ptr<VMInfo>);
    void Lock();
    void Unlock();
    std::shared_ptr<CommandQueue> InOrderSchedule();
    inline void IncreaseInnerProduct(double value) {
      based_inner_product_ += value;
    }
    inline void IncreaseTimeLengSquare(double value) {
      time_leng_square_ += value;
    }

    // accessor
    inline const VMList& VMs() const { return VMs_; }
};
#endif  // NEWSCL_CN_VMAFS_N_SCHEDULER_H_
