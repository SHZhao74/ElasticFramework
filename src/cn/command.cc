// Copyright [2013] (JoenChen)
#include "./command.h"

#include <stdio.h>
#include <assert.h>

#include <algorithm>
#include <vector>

#include <pthread.h>
#include <semaphore.h>

#include "./common.h"
#include "./cn.h"

#include "IO.h"
#include "utils.h"

//#include "../../../BasicGpuDemo/BasicGpuDemo.h"
//#include "../../../BasicGpuDemo/b3GpuDynamicsWorld.h"
//#include "../../../../src/Bullet3OpenCL/ParallelPrimitives/b3OpenCLArray.h"
//#include "../../../../src/Bullet3Collision/NarrowPhaseCollision/b3RigidBodyCL.h"

CommandCopyBuffer::CommandCopyBuffer(cl_mem t_src_mem,
                                     cl_mem t_dst_mem,
                                     size_t t_src_offset,
                                     size_t t_dst_offset,
                                     size_t t_cb) :
                                     d_src_mem_(t_src_mem),
                                     d_dst_mem_(t_dst_mem),
                                     src_offset_(t_src_offset),
                                     dst_offset_(t_dst_offset),
                                     cb_(t_cb)
{
}
void CommandCopyBuffer::Exec(cl_command_queue dev_queue) {
  fprintf(stderr, "Copy Buffer\n");
  _ED(clEnqueueCopyBuffer(dev_queue,
                          d_src_mem_,
                          d_dst_mem_,
                          src_offset_,
                          dst_offset_,
                          cb_,
                          0,
                          NULL,
                          &event_));
  clWaitForEvents(1, &event_);  
}
// class CommandRead
void CommandRead::Exec(cl_command_queue dev_queue) {
//#ifdef DEBUG
  fprintf(stderr, "Read Buffer\n");
//#endif // DEBUG
  cl_int err_code;
  if (mem_->newest_on_dev != NULL) {
    _ED(err_code = clEnqueueReadBuffer(mem_->newest_on_dev,
                                       mem_->mem,
                                       true,
                                       offset_,
                                       cb_,
                                       data_.data(),
                                       0,
                                       NULL,
                                       &event_));
  } else {
    _ED(err_code = clEnqueueReadBuffer(queues.front(),
                                       mem_->mem,
                                       true,
                                       offset_,
                                       cb_,
                                       data_.data(),
                                       0,
                                       NULL,
                                       &event_));
  }
  set_err_code(err_code);
  if (blocking_read_) {
    pthread_cond_signal(&cond_);
  } else {
    newscl::SockSend(non_blocking_sock_, data_.data(), cb_);
    bool ack = false;                                                            
    newscl::SockRecv(non_blocking_sock_, reinterpret_cast<char*>(&ack), sizeof(ack));
  }
#ifndef CPU_TIME
  set_exec_time(GetCLExecTime(event_));
#endif  // CPU_TIME
  clWaitForEvents(1, &event_);
}

CommandRead::CommandRead(
    const decltype(mem_) &t_mem,
    const decltype(blocking_read_)& t_blocking_read,
    const decltype(offset_)& t_offset,
    const decltype(cb_) &t_cb,
    const decltype(event_) &t_event)
  : mem_(t_mem), blocking_read_(t_blocking_read),
    offset_(t_offset), cb_(t_cb), event_(t_event) {
  pthread_mutex_init(&mutex_, NULL);
  pthread_cond_init(&cond_, NULL);
  data_.resize(t_cb);
}

void CommandRead::Blocking() {
  pthread_mutex_lock(&mutex_);
  {
    pthread_cond_wait(&cond_, &mutex_);
  }
  pthread_mutex_unlock(&mutex_);
}

// class CommandWrite
CommandWrite::CommandWrite(
    const decltype(mem_)& t_mem,
    const decltype(blocking_write_)& t_blocking_write,
    const decltype(offset_)& t_offset,
    const decltype(cb_)& t_cb,
    const decltype(event_)& t_event,
    const decltype(data_)& t_data)
  : mem_(t_mem), blocking_write_(t_blocking_write), offset_(t_offset),
    cb_(t_cb), event_(t_event), data_(t_data),
    cond_mutex_(PTHREAD_MUTEX_INITIALIZER), cond_(PTHREAD_COND_INITIALIZER) {
}

void CommandWrite::Blocking() {
  pthread_mutex_lock(&cond_mutex_);
  {
    pthread_cond_wait(&cond_, &cond_mutex_);
  }
  pthread_mutex_unlock(&cond_mutex_);
}

void CommandWrite::Exec(cl_command_queue queue) {
//#ifdef DEBUG
  fprintf(stderr, "WriteBuffer Command\n");
//#endif // DEBUG
  cl_int err_code;
  if (mem_->newest_on_dev == nullptr) {
#ifdef DEBUG
#endif // DEBUG

    //fprintf(stderr, "init write buffer, queues size:%ld\n", queues.size());
    for (auto &queue2 : queues) {
      _ED(err_code = clEnqueueWriteBuffer(queue2,
                                          mem_object,
                                          true,
                                          offset_,
                                          cb_,
                                          data_.data(),
                                          0,
                                          NULL,
                                          &event_));
    }
  } else {
    _ED(err_code = clEnqueueWriteBuffer(queues.front(),
                                        mem_object,
                                        true,
                                        offset_,
                                        cb_,
                                        data_.data(),
                                        0,
                                        NULL,
                                        &event_));
  }
  if (blocking_write_) {
    pthread_cond_signal(&cond_);
  }
  set_err_code(err_code);
#ifndef CPU_TIME
  //set_exec_time(GetCLExecTime(event_));
#endif  // CPU_TIME
  clWaitForEvents(1, &event_);
}
// class CommandKernel
CommandKernel::CommandKernel() {}

CommandKernel::CommandKernel(
    decltype(kernel_) t_kernel,
    decltype(is_NDRange_) t_is_NDRange,
    decltype(kernel_id_) t_kernel_id,
    decltype(work_dim_) t_work_dim,
    decltype(global_work_offset_) t_global_work_offset,
    decltype(global_work_size_) t_global_work_size,
    decltype(local_work_size_) t_local_work_size)
  : kernel_(t_kernel), is_NDRange_(t_is_NDRange), kernel_id_(t_kernel_id),
    work_dim_(t_work_dim), global_work_offset_(t_global_work_offset) {
  global_work_offset_ = NULL;
  global_work_size_ = new size_t[work_dim_];
  memcpy(global_work_size_, t_global_work_size, sizeof(size_t) * work_dim_);
  local_work_size_ = new size_t[work_dim_];
  memcpy(local_work_size_, t_local_work_size, sizeof(size_t) * work_dim_);
}

void CommandKernel::Exec(cl_command_queue dev_queue) {
//#ifdef DEBUG
  fprintf(stderr, "Execute a kernel\n");
//#endif // DEBUG
  //char kernel_name[100];
  //_ED(clGetKernelInfo(kernel_->kernel, CL_KERNEL_FUNCTION_NAME, 100, kernel_name, NULL));
  //fprintf(stderr, "Kernel Name: %s\n", kernel_name);
  for (auto &x : arg_mem_id_) {
    _ED(clSetKernelArg(kernel_->kernel,
                       x.first,
                       sizeof(cl_mem),
                       &x.second));
  }
  size_t const_data_offset = 0;
  for (auto &x : arg_const_) {
    //fprintf(stderr, "const data arg_idx: %d arg_size:%ld\n", x.first, x.second);
    //if (strcmp(kernel_name, "initializeGpuAabbsFull") == 0) {
    //  int numNodes= *reinterpret_cast<int*>(const_data.data());
    //  fprintf(stderr, "numNodes:%d\n", numNodes);
    //}

    _ED(clSetKernelArg(kernel_->kernel,
                       x.first,
                       x.second,
                       const_data.data() + const_data_offset));
    const_data_offset += x.second;
  }
/*
  for (auto &map_item : kernel_->arg_cl_mem) {
    auto &arg = map_item.second;
    if (arg->newest_on_dev != dev_queue && arg->newest_on_dev != nullptr) {
#ifdef DEBUG
      fprintf(stdout, "transfer\n");
#endif // DEBUG
      std::vector<char> data(arg->size);
#ifdef DEBUG
      printf("arg->size:%d\n", arg->size);
#endif // DEBUG
      _ED(err = clEnqueueReadBuffer(arg->newest_on_dev,
                                    arg->mem,
                                    true,
                                    0,
                                    arg->size,
                                    data.data(),
                                    0,
                                    NULL,
                                    NULL));
      _ED(err = clEnqueueWriteBuffer(dev_queue,
                                     arg->mem,
                                     true,
                                     0,
                                     arg->size,
                                     data.data(),
                                     0,
                                     NULL,
                                     NULL));
#ifdef DEBUG
      fprintf(stdout, "transfer done\n");
#endif // DEBUG
    }

  }
*/
  if (is_NDRange_) {
    //printf("Enqueue start\n");
    if (local_work_size_[0] != 0) {
      //cl_int ret;
      _ED(clEnqueueNDRangeKernel(dev_queue,
                                 kernel_->kernel,
                                 work_dim_,
                                 NULL,
                                 global_work_size_,
                                 local_work_size_,
                                 0,
                                 NULL,
                                 &event_));
      //assert(ret == CL_SUCCESS);
    } else {
      _ED(clEnqueueNDRangeKernel(dev_queue,
                                 kernel_->kernel,
                                 work_dim_,
                                 NULL,
                                 global_work_size_,
                                 NULL,
                                 0,
                                 NULL,
                                 &event_));
    }

    //printf("Enqueue end\n");
  } else {
    _ED(clEnqueueTask(dev_queue, kernel_->kernel, 0, NULL, &event_));
  }
  //clFinish(dev_queue);

  for (auto &arg : kernel_->arg_cl_mem) {
    arg.second->newest_on_dev = dev_queue;
  }
#ifndef CPU_TIME
  //set_exec_time(GetCLExecTime(event_));
#endif  // CPU_TIME
  clWaitForEvents(1, &event_);
}

// class CommandQueue

void CommandQueue::NotifyFinish() {
  //sem_post(&sem_finish);
  //pthread_cond_signal(&cond_is_finish_);
}

bool CommandQueue::Pop() {
  P_LOCK(mutex_);
  {
    if (!IsEmpty()) {
      queue_.pop_front();
    }
  }
  P_UNLOCK(mutex_);
  return true;
}
void CommandQueue::set_skip(const decltype(skip_) &skip) {
  if (skip) {
    skip_ = true;
  } else {
    skip_ = false;
  }
}
void CommandQueue::Push(Command* command) {
  bool is_increase = false;
  Lock();
  {
    is_increase = IsEmpty();
    queue_.push_back(command);
    //fprintf(stderr, " the command queue:%p\n", &queue_);
    //fprintf(stderr, " the first command queue:%p\n", &queue_.front());
    //queue_.front()->Exec(::queues.front());
    //fprintf(stderr,"press any key");
    //fgetc(stdin);
    if (is_increase && skip() == false) {
      CommandPool::Instance()->Increase();
      //fprintf(stderr, "queue size():%ld\n", queue_.size());
    }
  }
  Unlock();
  //if (is_increase) {
  //}
}
void CommandQueue::WaitForFinishing() {
  //if (!IsEmpty()) {
  //  sem_wait(&sem_finish);
  //}
  
  /*if (!IsEmpty()) {
    P_LOCK(mutex_is_finish_);
    {
      while(IsEmpty()) printf;
      pthread_cond_wait(&cond_is_finish_, &mutex_is_finish_);
    }
    P_UNLOCK(mutex_is_finish_);
  }*/
  if (!IsEmpty()) {
    P_LOCK(mutex_is_finish_);
    while (!IsEmpty()) {
      pthread_cond_wait(&cond_is_finish_, &mutex_is_finish_); 
    }
    P_UNLOCK(mutex_is_finish_);
  }
}

CommandQueue::~CommandQueue() {
}

CommandQueue::CommandQueue(decltype(VM_) VM,
                           decltype(binding_pool_) &binding_pool,
                           const decltype(binding_dev_) binding_dev,
                           decltype(skip_) const & skip) 
    : binding_pool_(binding_pool),
      VM_(VM),
      binding_dev_(binding_dev),
      skip_(skip) {
  pthread_mutex_init(&mutex_, NULL);
  //sem_init(&sem_finish, 0, 0);
}



//class CommandPool
std::shared_ptr<CommandQueue> CommandPool::CreateQueue(std::shared_ptr<VMInfo> vm) {
  std::shared_ptr<CommandQueue> queue;
  VMAFS_N_Scheduler::Instance()->Lock();
  {
    queue = std::make_shared<CommandQueue>(vm, Instance());
    ++vm->reference_count;
    queues_.push_back(queue);
  }
  VMAFS_N_Scheduler::Instance()->Unlock();
  return queue;
}

void CommandPool::DestroyQueue(std::shared_ptr<CommandQueue> queue) {
  auto &&iter = find(queues_.begin(), queues_.end(), queue);
#ifdef DEBUG
  assert(iter != queues_.end());
#endif
  --queue->VM()->reference_count;
  queues_.erase(iter);
#ifdef DEBUG
  fprintf(stderr, "del queue done\n");
#endif // DEBUG
}

std::unique_ptr<CommandPool>& CommandPool::Instance() {
  static std::unique_ptr<CommandPool> instance = nullptr;
  static bool is_init = false;
  static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
  if (!is_init) {
    pthread_mutex_lock(&init_mutex);
    {
      instance = std::move(std::unique_ptr<CommandPool>{new CommandPool});
    }
    pthread_mutex_unlock(&init_mutex);
    is_init = true;
  }

  return instance;
}
