// Copyright [2013] (JoenChen)
#include "./cn.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include <array>
#include <iterator>
#include <string>
#include <map>
#include <typeinfo>
#include <cmath>
#include <vector>
#include <list>
#include <memory>
#include <time.h>

#include "IO.h"
#include "newscl.h"
#include "config.h"
#include "./handler.h"
#include "./command.h"
#include "./VMAFS_N_Scheduler.h"
#include "./cn_info.h"
#include "./common.h"
//#include "../../../BasicGpuDemo/BasicGpuDemo.h"
#include <boost/property_tree/ptree.hpp>                                         
#include <boost/property_tree/json_parser.hpp> 
#define MSG_TYPE_DEBUG

using std::shared_ptr;
using std::dynamic_pointer_cast;
using std::weak_ptr;

std::vector<cl_platform_id> platforms;
std::vector<cl_device_id> devs;
std::vector<cl_command_queue> queues;
std::vector<cl_context> contexts;
sem_t sem;

newscl::SocketIOServer non_blocking_server(newscl::kCNNonBlockingPort);
newscl::SocketIOServer finish_server(newscl::kCNFinishPort);

// The dev queue is the cl_command_queue on the specific device
// The task queue is the CommandQueue of the client.
void* DevScheduler(void* data) {
  cl_command_queue dev_queue = *(reinterpret_cast<cl_command_queue*>(data));
  //int dev_id = find(queues.begin(), queues.end(), dev_queue) - queues.begin());
  decltype(VMAFS_N_Scheduler::Instance()) &sched = VMAFS_N_Scheduler::Instance();
  while (true) {
    //fprintf(stderr, "wait and decrease:%d\n", CommandPool::Instance()->SemValue());
    CommandPool::Instance()->WaitAndDecrease();
    if (!CommandPool::Instance()->check(dev_queue)) {
      CommandPool::Instance()->Increase();
      pthread_yield();
      //fprintf(stderr, "dev_queue:%p continue\n", dev_queue);
      continue;
    }
    Command *comm = NULL;
    decltype(sched->InOrderSchedule()) client_queue;
    sched->Lock();
    {
      client_queue = sched->InOrderSchedule();
      client_queue->set_skip(true);
      assert(client_queue != nullptr);
      comm = client_queue->front();
#ifdef CPU_TIME
      comm->set_start_time(GetCPUTime());
#endif  // CPU_TIME
    }
    sched->Unlock();

    comm->Exec(dev_queue);

    sched->Lock();
    {
      decltype(client_queue->VM()) &vm = client_queue->VM();
#ifdef CPU_TIME
      double used_time = GetCPUTime() - comm->start_time();
#else
      double used_time = comm->exec_time();
#endif  // CPU_TIME
      sched->IncreaseInnerProduct(used_time * vm->weight);
      sched->IncreaseTimeLengSquare(
          2 * vm->total_used_time * used_time + used_time * used_time);
      vm->total_used_time += used_time;

      client_queue->Pop();
      client_queue->Lock();
      {
        if (!client_queue->IsEmpty()) {
          CommandPool::Instance()->Increase();
        }
      }
      client_queue->Unlock();
      client_queue->set_skip(false);
    }
    sched->Unlock();
  }
  return nullptr;
}



static void InitOpenCLContext() {
  cl_int error_code = CL_SUCCESS;
  cl_uint num_of_platforms, num_of_devs;
  // get all platforms
  _ED(error_code = clGetPlatformIDs(0, NULL, &num_of_platforms));

  platforms.resize(num_of_platforms);
  _ED(error_code = clGetPlatformIDs(num_of_platforms, &platforms.front(), NULL));
  // check platform
  char platform_name[1024];
  for (decltype(num_of_platforms) i = 0; i < num_of_platforms; ++i) {
    error_code = clGetPlatformInfo(platforms[i],
                                   CL_PLATFORM_NAME,
                                   1024,
                                   platform_name,
                                   NULL);
    if (strcmp(platform_name, "NEWSCL") == 0) {
      platforms.erase(platforms.begin() + i);
      --num_of_platforms;
      --i;
    }
#ifdef DEBUG
    printf("platform %d: %s\n", i, platform_name);
#endif //DEBUG
  }

  // get all devices
  _ED(error_code = clGetDeviceIDs(platforms[0],
                                  CL_DEVICE_TYPE_GPU,
                                  0,
                                  NULL,
                                  &num_of_devs));
  // temporary
  //num_of_devs = 1;
  fprintf(stderr, "num_of_devs:%d\n", num_of_devs);
  devs.resize(devs.size() + num_of_devs);
  _ED(error_code = clGetDeviceIDs(platforms[0],
                                  CL_DEVICE_TYPE_ALL,
                                  num_of_devs,
                                  &devs.front(),
                                  NULL));
#ifdef DEBUG
  printf("platform 0, num of devs: %d\n", num_of_devs);
#endif // DEBUG

  // create context
  contexts.resize(contexts.size() + 1);
  contexts[contexts.size() - 1] = clCreateContext(NULL,
                                                  num_of_devs,
                                                  &devs.front(),
                                                  NULL,
                                                  NULL,
                                                  &error_code);
  GetErrorMsg(error_code);

  // create queue;
  queues.resize(devs.size());
  for (decltype(devs.size()) i = 0 ; i < devs.size(); ++i) {
#ifdef CPU_TIME
    queues[i] = clCreateCommandQueue(contexts.front(),
                                     devs[i],
                                     0,
                                     &error_code);
#else
    queues[i] = clCreateCommandQueue(contexts.front(),
                                     devs[i],
                                     CL_QUEUE_PROFILING_ENABLE,
                                     &error_code);
#endif  // CPU_TIME
    GetErrorMsg(error_code);
  }
}

static inline int Parser(char *tbuf, int buf_size) {
  return *(reinterpret_cast<int*>(tbuf));
}

void *Run(void* data) {
  int sock = (static_cast<newscl::SocketIOServer::ThreadData*>(data))->client_sock;
  std::vector<cl_program> programs;
  std::vector<CL_Kernel> kernels;
  std::vector<MemObject> mems;

  char buf[newscl::kMsgMaxSize];
  unsigned int recv_size, total_recv_size;
  fprintf(stderr, "connection came\n");
  std::shared_ptr<VMInfo> vm = VMAFS_N_Scheduler::Instance()->CreateVM(1.f);;

#ifdef DEBUG
  printf("connection came\n");
#endif // DEBUG
  int non_blocking_sock = -1;
  // accept the non-blocking connection
  {
    struct sockaddr_in pin;
    unsigned int addrsize = sizeof(pin);

    non_blocking_sock = accept(non_blocking_server.sock(),
                               (struct sockaddr *)&pin, &addrsize);
    if (non_blocking_sock == -1) {
      assert(false);  // accept the non_blocking_sock failed
    }
  }
  int finish_sock = -1;
  // accept the non-blocking connection
  {
    struct sockaddr_in pin;
    unsigned int addrsize = sizeof(pin);

    finish_sock = accept(finish_server.sock(),
                               (struct sockaddr *)&pin, &addrsize);
    if (finish_sock == -1) {
      assert(false);  // accept the non_blocking_sock failed
    }
  }
#ifdef DEBUG
  printf("the non-blocking connection also came\n");
#endif // DEBUG

  //int flag = 1;                                                                
  //int result = setsockopt(sock,                                         
  //    IPPROTO_TCP,                                         
  //    TCP_NODELAY,                                         
  //    (char*) &flag,                                       
   //   sizeof(int));                                        
  //assert(result>=0); 

  fprintf(stderr, "init bullet3\n");
  //BasicGpuDemo demo;
  bool bullet3_is_init = true;
  static int switcher = 0;
  fprintf(stderr, "init bullet3 end\n");

  auto queue = CommandPool::Instance()->CreateQueue(vm);
  queue->set_binding_dev(queues[switcher]);
  std::vector<decltype(queue) > comm_queues;
  comm_queues.push_back(queue);

  total_recv_size = 0;
  bzero(buf, newscl::kMsgMaxSize);



  while ((recv_size = recv(sock, buf + total_recv_size,
                          newscl::kMsgMaxSize - total_recv_size, 0)) > 0) {
    total_recv_size += recv_size;
    if (total_recv_size == newscl::kMsgMaxSize) {
#ifdef DEBUG
      printf("receive control message\n");
#endif // DEBUG
      int msg_type_num = -1;
      msg_type_num = Parser(buf, recv_size);
      switch (msg_type_num) {
        case newscl::cn_msg_type::kReg: {
#ifdef MSG_TYPE_DEBUG
          fprintf(stderr, "MSG_TYPE: Reg\n");
#endif // DEBUG
          HandlerRegister(sock, buf + sizeof(msg_type_num));
          /*
          double weight = *(reinterpret_cast<double*>(buf + sizeof(msg_type_num)));
          fprintf(stderr, "app weight:%lf\n", weight);
          double cn_weight = newscl::CNInfo::Instance()->weight();

          newscl::CNInfo::Instance()->set_weight(cn_weight + weight);
          snprintf(send_buf, newscl::kMsgMaxSize, "Reg succeed");
          newscl::SockSend(sock, send_buf, newscl::kMsgMaxSize);
          */
          break;
        }
        case newscl::cn_msg_type::kCreateProgramWithSource: {
#ifdef MSG_TYPE_DEBUG
          fprintf(stderr, "MSG_TYPE: Create Program With Source\n");
#endif // DEBUG
          HandlerCreateProgramWithSource(sock, buf, programs);
          break;
        }
        case newscl::cn_msg_type::kBuildProgram: {
#ifdef MSG_TYPE_DEBUG
          fprintf(stderr, "MSG_TYPE: Build Program\n");
#endif // DEBUG
          HandlerBuildProgram(sock, buf, programs);
          break;
        }
        case newscl::cn_msg_type::kCreateKernel: {
#ifdef MSG_TYPE_DEBUG
          fprintf(stderr, "MSG_TYPE: Create Kernel\n");
#endif // DEBUG
          HandlerCreateKernel(sock, buf + sizeof(msg_type_num), programs, kernels);
          break;
        }
        case newscl::cn_msg_type::kEnqueueTask: {
#ifdef MSG_TYPE_DEBUG
          fprintf(stderr, "MSG_TYPE: Enqueue Task\n");
#endif // DEBUG
          HandlerEnqueueTask(sock,
                             buf + sizeof(msg_type_num),
                             kernels,
                             *comm_queues[0]);
          break;
        }
        case newscl::cn_msg_type::kCreateBuffer: {
#ifdef MSG_TYPE_DEBUG
          fprintf(stderr, "MSG_TYPE: Create Buffer\n");
#endif // DEBUG
          HandlerCreateBuffer(sock, buf + sizeof(msg_type_num), mems);
          break;
        }
        case newscl::cn_msg_type::kEnqueueReadBuffer: {
#ifdef MSG_TYPE_DEBUG
          fprintf(stderr, "MSG_TYPE: Enqueue Read Buffer\n");
#endif // DEBUG
          HandlerEnqueueReadBuffer(sock,
                                   buf + sizeof(msg_type_num),
                                   mems,
                                   *comm_queues[0],
                                   non_blocking_sock);
          break;
        }
        case newscl::cn_msg_type::kEnqueueWriteBuffer: {
#ifdef MSG_TYPE_DEBUG
          fprintf(stderr, "MSG_TYPE: Enqueue Write Buffer\n");
#endif // DEBUG
          HandlerEnqueueWriteBuffer(sock,
                                    buf + sizeof(msg_type_num),
                                    mems,
                                    comm_queues,
                                    non_blocking_sock);
          break;
        }
        case newscl::cn_msg_type::kSetKernelArg: {
          //printf("MSG_TYPE: Set Kernel Arg\n");
          HandlerSetKernelArg(sock, buf + sizeof(msg_type_num), kernels, mems);
          break;
        }
        case newscl::cn_msg_type::kFinish: {
#ifdef MSG_TYPE_DEBUG
          fprintf(stderr, "MSG_TYPE: FINISH\n");
#endif // DEBUG
          HandlerFinish(finish_sock, buf + sizeof(msg_type_num), comm_queues);
          break;
        }
        case newscl::cn_msg_type::kEnqueueNDRangeKernel: {
#ifdef MSG_TYPE_DEBUG
          fprintf(stderr, "MSG_TYPE: NDRangeKernel\n");
#endif // DEBUG
          HandlerEnqueueNDRangeKernel(sock,
                             buf + sizeof(msg_type_num),
                             kernels,
                             mems,
                             *comm_queues[0]);
          break;
        }
        case newscl::cn_msg_type::kEnqueueCopyBuffer: {
#ifdef MSG_TYPE_DEBUG
          fprintf(stderr, "MSG_TYPE: CopyBuffer\n");
#endif // DEBUG
          HandlerEnqueueCopyBuffer(sock,
                                   buf + sizeof(msg_type_num),
                                   mems,
                                   *comm_queues[0]);
          break;
        }
        case newscl::cn_msg_type::kReleaseMemObject: {
#ifdef MSG_TYPE_DEBUG
          fprintf(stderr, "MSG_TYPE: ReleaseMemObject\n");
#endif // DEBUG
          HandlerReleaseMemObject(sock,
                                  buf + sizeof(msg_type_num),
                                  mems,
                                  *comm_queues[0]);
          break;
        }
        default: {
          assert(false);  // Unknown message type
          break;
        }
      }
      total_recv_size = 0;
      bzero(buf, newscl::kMsgMaxSize);
#ifdef DEBUG
      puts("");
#endif // DEBUG
    }
  }
  for (auto& queue : comm_queues) {
    queue->WaitForFinishing();
  }
  if (recv_size == 0) {
#ifdef DEBUG
    printf("connection closed\n");
#endif // DEBUG
  } else {
    assert(false);  // recv error
  }
#ifdef DEBUG
  fprintf(stderr, "VM quit!\n");
#endif // DEBUG
  VMAFS_N_Scheduler::Instance()->Lock();
  {
    CommandPool::Instance()->DestroyQueue(queue);
    VMAFS_N_Scheduler::Instance()->DestroyVM(vm);
  }
  VMAFS_N_Scheduler::Instance()->Unlock();
  return nullptr;
}

static void Paste(char *buf, size_t *buf_offset, void *ptr, size_t size) {
  memcpy(buf + *buf_offset, ptr, size);
  *buf_offset += size;
}
static newscl::SocketIOClient& DispatcherConn() {
  static newscl::SocketIOClient dispatcher_conn(newscl::kDispatcherIP,
                                                newscl::kDispatcherPort);
  static bool is_init = false;
  if(!is_init) {
    dispatcher_conn.Connect();
    is_init = true;
  }
  return dispatcher_conn;
}
static int RegisterToDispatcher() {
  std::array<char, newscl::kMsgMaxSize> send_buf;
  int msg_type_num = newscl::dispatcher_msg_type::kCNReg;
  double capacity = newscl::CNInfo::Instance()->capacity();

  size_t buf_offset = 0;
  Paste(send_buf.data(),
        &buf_offset,
        reinterpret_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf.data(),
        &buf_offset,
        reinterpret_cast<void*>(&capacity),
        sizeof(capacity));
  //DispatcherConn().Connect();
  DispatcherConn().Send(send_buf.data(), send_buf.size());
  int cn_id = -1;
  DispatcherConn().Recv(reinterpret_cast<char*>(&cn_id), sizeof(cn_id));
#ifdef DEBUG
  printf("cn_id: %d\n", cn_id);
#endif // DEBUG
  return cn_id;
}

static void LoadConfig() {
  double capacity, weight, used_time;
  FILE *fp = fopen("/opt/cn.cfg", "r");
  if (fp == NULL) {
    assert(false);  // not found the cn.cfg
  }
  if (fscanf(fp, "capacity:%lf\n", &capacity) != EOF) {
    perror("");
  }
  if (fscanf(fp, "weight:%lf\n", &weight) != EOF) {
    perror("");
  }
  if (fscanf(fp, "used_time:%lf\n", &used_time) != EOF) {
    perror("");
  }
  fclose(fp);
#ifdef DEBUG
  printf("capacity: %lf\n", capacity);
  printf("weight: %lf\n", weight);
  printf("used_time: %lf\n", used_time);
#endif // DEBUG
  newscl::CNInfo::Instance(weight, used_time, capacity);  // init
}


/*
static void* Teller(void*) {
  using namespace newscl;
  using namespace newscl::dispatcher_msg_type;
  while(true){
    std::array<char, kMsgMaxSize> send_buf;
    size_t buf_offset = 0;
    double weight = CNInfo::Instance()->weight();
    int msg_type_num = kLoadInfo;
    int cn_id = CNInfo::Instance()->id();
    double used_time = CNInfo::Instance()->used_time();
#ifdef DEBUG
    printf("send loading information weight:%lf\n", weight);
#endif // DEBUG
    Paste(send_buf.data(), &buf_offset, &msg_type_num, sizeof(msg_type_num));
    Paste(send_buf.data(), &buf_offset, &cn_id, sizeof(cn_id));
    Paste(send_buf.data(), &buf_offset, &weight, sizeof(weight));
    Paste(send_buf.data(), &buf_offset, &used_time, sizeof(used_time));
    DispatcherConn().Send(send_buf.data(), send_buf.size());
    sleep(3);
  }
  return nullptr;
}
*/
static void* updateScore(void* data)
{ //封包只包msg_type 跟cn_id，之後傳整個JSON檔過去
  using namespace newscl;
  using namespace newscl::dispatcher_msg_type;

  while(true){
    printf("Time to update Score.....\n");
    FILE *fp = fopen("/opt/history.json","r"); //assert (fp == NULL && "cannot open history.json\n");
    std::array<char, kMsgMaxSize> send_buf;
    size_t buf_offset = 0;
    //double weight = CNInfo::Instance()->weight();
    int msg_type_num = kUpdateScore;
    int cn_id = CNInfo::Instance()->id();
    //double used_time = CNInfo::Instance()->used_time();
    size_t start = clock();
    Paste(send_buf.data(), &buf_offset, &msg_type_num, sizeof(msg_type_num));
    Paste(send_buf.data(), &buf_offset, &cn_id, sizeof(cn_id));
    //Paste(send_buf.data(), &buf_offset, &weight, sizeof(weight));
    //Paste(send_buf.data(), &buf_offset, &used_time, sizeof(used_time));
    DispatcherConn().Send(send_buf.data(), send_buf.size());

    buf_offset = 0;
    while (!feof(fp) ){
      //printf("fread\n");
      bzero(send_buf.data(), kMsgMaxSize);
      buf_offset += fread(send_buf.data(), sizeof(char), kMsgMaxSize, fp);
      //printf("buf_offset=%d\n%s\n", buf_offset, send_buf.data());
      DispatcherConn().Send(send_buf.data(), send_buf.size());
    }
    DispatcherConn().Send("END", 3);

    int sock = (static_cast<newscl::SocketIOServer::ThreadData*>(data))->client_sock;
    //printf("SHIT\n");
    bzero(send_buf.data(), kMsgMaxSize);
    recv(sock, send_buf.data(), 3,0);
    char end[3];
    strncpy(end , send_buf.data(),3);
    if (strcmp(end, "END") != 0) {
      printf("end = %s\n", end);
    }
    //end = clock();
    fclose(fp);
    printf("Totall send %d bytes, spand %lu ms\n", buf_offset, clock() - start);
    

    //getchar();
    sleep(5);
  }
  return nullptr;
}


int main() {
  newscl::SocketIOServer server(newscl::kCNPort);
  newscl::thread_per_socket = Run;
  //newscl::thread_per_socket = updateScore;
  server.BindAndListen(100);

  non_blocking_server.BindAndListen(100);
  finish_server.BindAndListen(100);

  InitOpenCLContext();
  LoadConfig();
  int cn_id = RegisterToDispatcher();
  newscl::CNInfo::Instance()->set_id(cn_id);

  //{
  //  pthread_t update_pid;
  //  pthread_create(&update_pid,
  //                 NULL,
  //                 updateScore,
  //                 NULL);
  //}
  
  // Loading Information Teller
  {
    //pthread_t teller_pid;
    //pthread_create(&teller_pid,
    //               NULL,
    //               Teller,
    //               NULL);
  }

  std::vector<pthread_t> pids(devs.size());
  for (decltype(devs.size()) i = 0; i < devs.size(); ++i) {
    fprintf(stderr, "create dev thread\n");
    pthread_create(&pids[i],
                   NULL,
                   DevScheduler,
                   static_cast<void*>(&queues[i]));
  }
  server.MainLoop();
}
