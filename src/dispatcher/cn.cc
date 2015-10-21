// Copyright [2013] (JoenChen)
#include "./cn.h"

#include <cassert>

#include <string>
#include "config.h"
#include "./VMAFS_N_Dispatcher.h"


std::unique_ptr<newscl::CNList>& newscl::CNList::Instance() {
  static bool is_init = false;
  static std::unique_ptr<newscl::CNList> CNs = nullptr;
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  if (!is_init) {
    pthread_mutex_lock(&mutex);
    {
      CNs =
        std::move(std::unique_ptr<newscl::CNList> {new CNList});
    }
    pthread_mutex_unlock(&mutex);
    is_init = true;
  }
  return CNs;
}
newscl::CNInfo::HASH_KEY_T newscl::CNList::AddCN(const CNInfo& cn) {
  static int cn_id = -1;
  CNInfo::HASH_KEY_T id;
  VMAFS_N_Dispatcher::Instance()->Lock();
  {
    id = ++cn_id;
    CNs_.insert({id, cn});
    VMAFS_N_Dispatcher::Instance()->i_leng_square_ +=
      cn.capacity() * cn.capacity();
  }
  VMAFS_N_Dispatcher::Instance()->Unlock();
  return id;
}

newscl::CNInfo::HASH_KEY_T newscl::CNList::AddCN(const std::string IP,
                                                 const double &capacity,
                                                 const double &weight,
                                                 const double &used_time) {
  static int cn_id = -1;
  CNInfo::HASH_KEY_T id;
  VMAFS_N_Dispatcher::Instance()->Lock();
  {
    id = ++cn_id;
    CNs_.insert({id, {weight,
                      used_time,
                      capacity,
                      newscl::kCNPort,
                      newscl::kCNNonBlockingPort,
                      IP}});
    VMAFS_N_Dispatcher::Instance()->i_leng_square_ += capacity * capacity;
  }
  VMAFS_N_Dispatcher::Instance()->Unlock();
  return id;
}

void newscl::CNList::DelCN(const CNInfo::HASH_KEY_T &cn_id) {
  VMAFS_N_Dispatcher::Instance()->Lock();
  {
    CNs_.erase(cn_id);
  }
  VMAFS_N_Dispatcher::Instance()->Unlock();
}

newscl::CNInfo& newscl::CNList::GetCN(const CNInfo::HASH_KEY_T &key) {
  auto iter = CNs_.find(key);
  assert(iter != CNs_.end());
  return iter->second;
}

int newscl::CNList::UpdateCNWeight(CNInfo::HASH_KEY_T const &key,
                                   double const &weight) {
  auto iter = CNs_.find(key);
  if (iter == CNs_.end()) {
    return -1;
  } else {
    iter->second.set_weight(weight);
    return 0;
  }
}

int newscl::CNList::UpdateCNUsedTime(CNInfo::HASH_KEY_T const &key,
                                     double const &used_time) {
  auto iter = CNs_.find(key);
  if (iter == CNs_.end()) {
    return -1;
  } else {
    iter->second.set_used_time(used_time);
    return 0;
  }
}

int newscl::CNList::UpdateCNScore(CNInfo::HASH_KEY_T const &key){

  auto iter = CNs_.find(key);
  if (iter == CNs_.end()) {
    return -1;
  } else {
    //iter->second.set_spend_time(buf[1],1);
    
    return 0;
  }
}
