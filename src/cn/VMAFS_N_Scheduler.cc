#include "./VMAFS_N_Scheduler.h"

#include <math.h>
#include <pthread.h>

#include <algorithm>
#include <memory>

#include "./common.h"
#include "./cn.h"
// class VMAFS_N_Scheduler

using std::shared_ptr;
std::unique_ptr<VMAFS_N_Scheduler>& VMAFS_N_Scheduler::Instance() {
  static std::unique_ptr<VMAFS_N_Scheduler> instance = nullptr;
  static bool is_init = false;
  static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
  if (!is_init) {
    pthread_mutex_lock(&init_mutex);
    {
      instance = std::move(std::unique_ptr<VMAFS_N_Scheduler>{new VMAFS_N_Scheduler});
    }
    pthread_mutex_unlock(&init_mutex);
    is_init = true;
  };
  return instance;
}
VMAFS_N_Scheduler::VMAFS_N_Scheduler()
    : based_inner_product_(0.f),
      weight_leng_square_(0.f),
      time_leng_square_(0.f) {
  pthread_mutex_init(&proc_lock_, NULL);
}

shared_ptr<VMInfo> VMAFS_N_Scheduler::CreateVM(double weight) {
  shared_ptr<VMInfo> vm;
  Lock();
  {
    vm = std::make_shared<VMInfo>(VMs_.size(), weight);
    VMs_.push_back(vm);
    weight_leng_square_ += weight * weight;
  }
  Unlock();
  return vm;
}
int VMAFS_N_Scheduler::DestroyVM(shared_ptr<VMInfo> vm) {
  int err_code = 0;
  if (vm->reference_count != 0) {
    err_code = -1;
  } else {
    auto &&iter = find_if(VMs_.begin(), VMs_.end(),
        [=](decltype(VMs_.front()) x) { return  vm.get() == x.get() ;});
    if (iter != VMs_.end()) {
      based_inner_product_ -= vm->weight * vm->total_used_time;
      weight_leng_square_ -= vm->weight * vm->weight;
      time_leng_square_ -= vm->total_used_time * vm->total_used_time;
#ifdef DEBUG
      printf("vm->total_used_time_: %f\n", vm->total_used_time);
      printf("time_leng_sqaure: %f\n", time_leng_square_);
#endif  // DEBUG
      VMs_.erase(iter);
    }
  }
  return err_code;
}

void VMAFS_N_Scheduler::Lock() {
  pthread_mutex_lock(&proc_lock_);
}

void VMAFS_N_Scheduler::Unlock() {
  pthread_mutex_unlock(&proc_lock_);
}

std::shared_ptr<CommandQueue> VMAFS_N_Scheduler::InOrderSchedule() {
  double cos_max_value = -1.f;
  std::shared_ptr<CommandQueue> chosen_queue = nullptr;
  auto &pool = CommandPool::Instance();

  double cos_value = 0.f;

#ifdef DEBUG
  fprintf(stderr, "queue size in pool: %ld\n", pool->queues().size());
#endif  // DEBUG
#ifdef CPU_TIME
  double jitter_inner_product = 0.f, jitter_time_leng_square = 0.f;
  for (const auto &queue : pool->queues()) {
    if (queue->skip()) {
      double jitter_time = GetCPUTime() - queue->front()->start_time();
      jitter_inner_product += queue->VM()->weight * jitter_time;
      jitter_time_leng_square +=
        2 * queue->VM()->total_used_time * jitter_time +
        jitter_time * jitter_time;
    }
  }
#endif  // CPU_TIME
  //fprintf(stderr, "queue size in pool: %ld\n", pool->queues().size());
  //int count = -1, idx = -1;
  //static int accu[10] = {};
  for (const auto &queue : pool->queues()) {
    //++idx;
    if (queue->skip() || queue->IsEmpty()) {
      continue;
    }
    auto &command = queue->front();

    double inner_product = 0.f, time_leng = 0.f;
#ifndef CPU_TIME
    //fprintf(stderr, "__LINE__:%d\n", __LINE__);
    //fprintf(stderr, "queue.size():%ld\n", queue->size());
    //fprintf(stderr, "queue->VM()->weight:%f\n", queue->VM()->weight);
    //fprintf(stderr, "command->est_exec_time():%f\n", command->est_exec_time());
    inner_product = based_inner_product_ +
      command->est_exec_time() * queue->VM()->weight;
    time_leng = time_leng_square_;
#else
    inner_product = based_inner_product_ +
      jitter_inner_product +
      command->est_exec_time() * queue->VM()->weight;
    time_leng = time_leng_square_ + jitter_time_leng_square;
#endif  // CPU_TIME
    //fprintf(stderr, "__LINE__:%d\n", __LINE__);
    time_leng += 2 * queue->VM()->total_used_time * command->est_exec_time() +
      command->est_exec_time() * command->est_exec_time();
    //fprintf(stderr, "__LINE__:%d\n", __LINE__);
    time_leng = sqrt(time_leng);
    //fprintf(stderr, "__LINE__:%d\n", __LINE__);

    cos_value = inner_product / (sqrt(weight_leng_square_) * time_leng);
    //fprintf(stderr, "idx:%d inner_product:%f cos_value:%f\n", idx, inner_product, cos_value);
    //fprintf(stderr, "__LINE__:%d\n", __LINE__);
    //fprintf(stderr, "cos_value: %f, inner:%f, weight_leng:%f, time_leng:%f \n",
    //                 cos_value, inner_product, weight_leng_square_, time_leng);

#ifdef DEBUG
    fprintf(stderr, "time_leng: %f\n", time_leng);
#endif  // DEBUG

    if (cos_value > cos_max_value) {
      cos_max_value = cos_value;
      chosen_queue = queue;
      //count = idx;
    }
  }
  //++accu[count];
  //int i = 0;
  //for (const auto &queue : pool->queues()) {
  //  fprintf(stderr, "%d(%d) ", accu[i++], queue->size());
  //}
  //fprintf(stderr, "\n");
  //fprintf(stderr, "chosen count: %d\n", count);
  //fgetc(stdin);
  return chosen_queue;
}
