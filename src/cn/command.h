// Copyright [2013] (JoenChen)
#ifndef NEWSCL_CN_COMMAND_H_
#define NEWSCL_CN_COMMAND_H_
#include <cstdio>

#include <string>
#include <list>
#include <memory>
#include <typeinfo>
#include <vector>

#include <boost/utility.hpp>

#include <CL/cl.h>
#include <semaphore.h>
#include <pthread.h>

#include "./wrapped_CL_type.h"
#include "./VM.h"
#include "./utils.h"

class CommandPool;

class Command {
  cl_int err_code_;
  double est_exec_time_;
  double exec_time_;
#ifdef CPU_TIME
  double start_time_;
#endif  // CPU_TIME

  protected:
    inline void set_exec_time(const decltype(exec_time_) &t_exec_time) {
      exec_time_ = t_exec_time;
    }
    inline void set_err_code(const decltype(err_code_) &t_err_code) {
      err_code_ = t_err_code;
    }

  public:
    inline decltype(err_code_) err_code() const { return err_code_; }
    inline decltype(exec_time_) exec_time() const { return exec_time_; }
    inline decltype(est_exec_time_) est_exec_time() const {
      return est_exec_time_;
    }

#ifdef CPU_TIME
    inline void set_start_time(const decltype(start_time_) &t_start_time) {
      start_time_ = t_start_time;
    }
    inline decltype(start_time_) start_time() const { return start_time_; }
#endif  // CPU_TIME

    virtual void Exec(cl_command_queue) = 0;

    Command(): err_code_(CL_SUCCESS), est_exec_time_(1.f), exec_time_(1.f) {
    }
};
class BasicGpuDemo;
class CommandBullet3ReadResultBack : public Command{
  BasicGpuDemo &demo_;
  int sock_;
  public:
    CommandBullet3ReadResultBack(BasicGpuDemo &demo, int sock): demo_(demo), sock_(sock) {
    }
    void Exec(cl_command_queue);
};
class CommandReleaseMemObject: public Command {
  cl_mem d_mem_;
  public:
    CommandReleaseMemObject(cl_mem t_mem): d_mem_(t_mem) { }
    void Exec(cl_command_queue) {
      clReleaseMemObject(d_mem_);
    }
};
class CommandCopyBuffer: public Command {
  cl_mem d_src_mem_;
  cl_mem d_dst_mem_;
  size_t src_offset_;
  size_t dst_offset_;
  size_t cb_;
  cl_event event_;
  public:
    CommandCopyBuffer(cl_mem, cl_mem, size_t, size_t, size_t);
    void Exec(cl_command_queue);
};
class CommandFinish : public Command {
  sem_t sem;
  public:
    void Exec(cl_command_queue) {
      fprintf(stderr, "Command Finish\n");
      sem_post(&sem);
    }
    void Wait() {
      sem_wait(&sem);
    }

    CommandFinish() {
      sem_init(&sem, 0, 0);
    }
};

class CommandKernel : public Command {
  public:
    std::vector<char> const_data;
  private:
  CL_Kernel *kernel_;
  bool is_NDRange_;
  unsigned int kernel_id_;
  cl_uint work_dim_;
  size_t *global_work_offset_;
  size_t *global_work_size_;
  size_t *local_work_size_;
  cl_event event_;
  CommandKernel();
  std::vector<std::pair<unsigned char, cl_mem> > arg_mem_id_;
  std::vector<std::pair<unsigned char, size_t> > arg_const_;

  public:
    CommandKernel(decltype(kernel_),
                  decltype(is_NDRange_),
                  decltype(kernel_id_),
                  decltype(work_dim_) = 0,
                  size_t * = 0,
                  decltype(global_work_size_) = 0,
                  decltype(local_work_size_) = 0);
    void AddArg(unsigned char arg_idx, cl_mem mem) {
      arg_mem_id_.push_back({arg_idx, mem});
    }
    void AddConst(unsigned char arg_idx, size_t arg_size) {
      arg_const_.push_back({arg_idx, arg_size});
    }
    void Exec(cl_command_queue);
};

class CommandRead: public Command {
  MemObject *mem_;
  cl_bool blocking_read_;
  size_t offset_;
  size_t cb_;
  cl_event event_;
  std::vector<char> data_;
  pthread_mutex_t mutex_;
  pthread_cond_t cond_;
  int non_blocking_sock_;

  public:
    int mem_id;
    CommandRead(const decltype(mem_)&,
                const decltype(blocking_read_)&,
                const decltype(offset_)&,
                const decltype(cb_)&,
                const decltype(event_)&);
    inline std::vector<char>& getData() { return data_; }
    inline void set_non_blocking_sock(
        const decltype(non_blocking_sock_) & t_non_blocking_sock) {
      non_blocking_sock_ = t_non_blocking_sock;
    }

    void Blocking();
    void Exec(cl_command_queue);
};

class CommandWrite: public Command {
  public:
    MemObject *mem_;
  private:
  cl_bool blocking_write_;
  size_t offset_;
  size_t cb_;
  cl_event event_;
  std::vector<char> data_;
  int non_blocking_sock_;
  pthread_mutex_t cond_mutex_;
  pthread_cond_t cond_;

  public:
    cl_mem mem_object;
    CommandWrite(const decltype(mem_)&,
                 const decltype(blocking_write_)&,
                 const decltype(offset_)&,
                 const decltype(cb_)&,
                 const decltype(event_)&,
                 const decltype(data_)&);
    inline std::vector<char>& data() {return data_;}
    inline void set_non_blocking_sock(
        const decltype(non_blocking_sock_) & t_non_blocking_sock) {
      non_blocking_sock_ = t_non_blocking_sock;
    }
    void Blocking();
    void Exec(cl_command_queue);
};

class CommandQueue: private boost::noncopyable {
  pthread_mutex_t mutex_;
  std::unique_ptr<CommandPool>& binding_pool_;
  std::shared_ptr<VMInfo> VM_;
  cl_command_queue binding_dev_;
  bool skip_;
  pthread_mutex_t mutex_is_finish_;
  pthread_cond_t  cond_is_finish_;
  sem_t sem_finish;
  int assign_;

  protected:

  public:
    std::list<Command*> queue_;
    CommandQueue(decltype(VM_),
                 decltype(binding_pool_),
                 const decltype(binding_dev_) = nullptr,
                 decltype(skip_) const & =false);
    ~CommandQueue();

    void NotifyFinish();
    inline void Lock() { P_LOCK(mutex_); }
    inline void Unlock() { P_UNLOCK(mutex_); }

    inline decltype(queue_.empty()) IsEmpty() const { return queue_.empty(); }
    void Push(Command *command);
    bool Pop();
    void WaitForFinishing();

    // mutator
    void set_skip(const decltype(skip_) &t_skip);
    inline void set_binding_dev(const decltype(binding_dev_) &dev) {
      binding_dev_ = dev;
    }

    // accessor
    inline decltype(queue_.size()) size() const { return queue_.size(); }
    inline decltype(queue_.front()) &front() { return queue_.front(); }
    inline Command* extract() { return queue_.front();}
    inline const decltype(VM_) &VM() { return VM_; }
    inline decltype(skip_) skip() const { return skip_; }
    inline decltype(binding_dev_) binding_dev() { return binding_dev_; }
};

class CommandPool {
  protected:
    std::list<std::shared_ptr<CommandQueue> > queues_;
    CommandPool() { sem_init(&sem_, 0, 0); }

  private:
    friend class CommandQueue;
    sem_t sem_;

  public:
    static std::unique_ptr<CommandPool>& Instance();

    std::shared_ptr<CommandQueue> CreateQueue(std::shared_ptr<VMInfo> vm);
    void DestroyQueue(std::shared_ptr<CommandQueue> queue);
    inline void Decrease() { sem_wait(&sem_); }
    inline void Increase() { sem_post(&sem_); }
    inline int SemValue() {
      int val = -1;
      sem_getvalue(&sem_, &val);
      return val;
    }
    void WaitAndDecrease() {
#ifdef DEBUG
      int val = -1;
      sem_getvalue(&sem_, &val);
      fprintf(stderr, "pool sem value before waitAndDecrease:%d\n", val);
#endif
      Decrease();
#ifdef DEBUG
      fprintf(stderr, "go~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
#endif
    }

    inline bool check(cl_command_queue dev_queue) {
      fprintf(stderr, "dev_queue:%p\n", dev_queue);
      for (auto &q : queues_) {
        fprintf(stderr, "skip:%d size:%ld q->binding_dev:%p\n", q->skip(), q->size(), q->binding_dev());
        if (q->skip() == false && q->size() != 0 &&
            (q->binding_dev() == nullptr || q->binding_dev() == dev_queue)) {
          return true;
        }
      }
      return false;
    }

    // accessor
    decltype(queues_.size()) size() const { return queues_.size(); }
    decltype(queues_) queues() { return queues_; }
};
#endif  // NEWSCL_CN_COMMAND_H_
