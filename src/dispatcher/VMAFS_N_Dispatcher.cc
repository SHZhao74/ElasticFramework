// Copyright [2013] <JoenChen>
#include "./VMAFS_N_Dispatcher.h"

#include <cmath>

#include <pthread.h>
using std::unique_ptr;
unique_ptr<newscl::VMAFS_N_Dispatcher>& newscl::VMAFS_N_Dispatcher::Instance() {
  static bool is_init = true;
  static std::unique_ptr<VMAFS_N_Dispatcher> dispatcher = nullptr;
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  if (is_init) {
    pthread_mutex_lock(&mutex);
    {
      dispatcher =
        std::move(std::unique_ptr<VMAFS_N_Dispatcher> {new VMAFS_N_Dispatcher});
    }
    pthread_mutex_unlock(&mutex);
    is_init = false;
  }
  return dispatcher;
}

newscl::CNInfo::HASH_KEY_T newscl::VMAFS_N_Dispatcher::Dispatch(
      const double& app_weight) {
  //static int num = 0;
  double cos_max_value = -1.f;
  double cos_value = 0.f;
  CNInfo::HASH_KEY_T idx = -1;
  auto &pCNList = CNList::Instance();
  pCNList->Lock();
  {
    /*
    ++num;
    auto iter = pCNList->CNs_.begin();
    for (int i = 0; i < num % pCNList->CNs_.size(); ++i) {
      ++iter;
    }
    idx = iter->first;
    */
    
    for (auto &&iter = pCNList->cbegin(); iter != pCNList->cend(); ++iter) {
      const auto &cn = iter->second;
      const auto &cn_idx = iter->first;
      double inner_product = based_inner_product_ + app_weight * cn.weight();
      double v_leng_square = v_leng_square_ +
        2 * cn.weight() * app_weight + app_weight * app_weight;

      cos_value = inner_product / (sqrt(i_leng_square_) * sqrt(v_leng_square));

      if (cos_value > cos_max_value) {
        cos_max_value = cos_value;
        idx = cn_idx;
      }
    }
    
  }
  pCNList->Unlock();
  return idx;
}
