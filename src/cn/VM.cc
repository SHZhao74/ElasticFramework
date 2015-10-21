// Copyright [2013] (JoenChen: joen@joen.cc)
#include "VM.h"

VMInfo::VMInfo(decltype(id) t_id,
               decltype(weight) t_weight,
               decltype(total_used_time) t_total_used_time)
  : id(t_id), weight(t_weight), total_used_time(t_total_used_time),
    reference_count(0){
}
