#include "./cn_info.h"

#include <memory>

std::unique_ptr<newscl::CNInfo>& newscl::CNInfo::Instance(
    const decltype(weight_) t_weight,
    const decltype(used_time_) t_used_time,
    const decltype(capacity_) t_capacity) {
  static std::unique_ptr<CNInfo> instance = nullptr;
  static bool is_init = true;
  static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;

//  fprintf(stderr, "get instance\n");
  if (is_init) {
//    fprintf(stderr, "lock\n");
    pthread_mutex_lock(&init_mutex);
    {
      instance = std::move(std::unique_ptr<CNInfo>{
          new CNInfo{t_weight, t_used_time, t_capacity}});
    }
    pthread_mutex_unlock(&init_mutex);
//    fprintf(stderr, "unlock\n");
    is_init = false;
  };
//  fprintf(stderr, "return instance\n");
  return instance;
}
