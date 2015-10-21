// Copyright [2013] (JoenChen: joen@joen.cc)
#ifndef NEWSCL_CN_VM_H_
#define NEWSCL_CN_VM_H_
#include <list>
struct VMInfo{
  int id;
  double weight;
  double total_used_time;
  unsigned int reference_count;
  VMInfo(decltype(id) t_id,
         decltype(weight) t_weight,
         decltype(total_used_time) t_total_used_time = 0.f);
};
#endif  // NEWSCL_CN_VM_H_
