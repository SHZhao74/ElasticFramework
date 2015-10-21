#include "CL/cl.h"
#include "CL/opencl.h"
#include "CL/cl_ext.h"
#include "./cl_dispatch.h"
#include "./cl_api.h"
#include "parser.h"
#include "config.h"
#include "newscl.h"
#include "IO.h"
#include "elastic.h"
#include <vector>
#include <algorithm>
#include <iterator>
#include <stdlib.h>
#include <string.h>
#include <netinet/tcp.h>
#include <array>
#include <cassert>
#include <memory>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <cctype>
#include <boost/regex.hpp>
using namespace newscl;

void *ElasticClient::runThread(void* runThreadInput){
  if(runThreadInput == NULL){
    return NULL;
  }
  unsigned char level = ((RunThreadInput*)runThreadInput)->level;
  ElasticClient *client = ((RunThreadInput*)runThreadInput)->elasticClient;
  for(int i = 0; i < client->io->output[level].size(); i++) {
    client->EnqueueReadBuffer(level, client->io->output[level][i].clmem, CL_TRUE, 0, client->io->output[level][i].size, client->io->output[level][i].address, 0, NULL, NULL);
  }
  client->finishCallback(level);
  delete (RunThreadInput*)runThreadInput;
}

void ElasticClient::init(char* kernel_fileName, char* kernel_name, ElasticIO& io){
  this->cn_num = GetDevices(kernel_name, 10, io.level);
  dim = 1;
  this->io = &io;
  int *temp;
  for(int i = 0; i < cn_num; i++){
    cl_program program = load_program(i, kernel_fileName);
    cl_kernel kernel = CreateKernel(i, program, kernel_name, NULL);
    this->kernel_vector.push_back(kernel);
  }
  for(int level = 0; level < cn_num; level++) {
    for(int i = 0; i < io.input[level].size(); i++) {
      io.input[level][i].clmem = CreateBuffer(level, CL_MEM_READ_ONLY, io.input[level][i].size, NULL, NULL);
      SetKernelArg(kernel_vector[level], i, sizeof(cl_mem), &(io.input[level][i].clmem));

    }
    for(int i = 0; i < io.output[level].size(); i++) {
      io.output[level][i].clmem = CreateBuffer(level, CL_MEM_WRITE_ONLY, io.output[level][i].size, NULL, NULL);
      SetKernelArg(kernel_vector[level], io.input[level].size()+i, sizeof(cl_mem), &(io.output[level][i].clmem));
    }
    //TODO
    temp = (int*)(io.input[level][3].address);
    work_item[level] = *temp * *(temp + 1);
   // printf("width %d, heigth %d\n", *temp, *(temp + 1));
  }
  setFinishCallback(defaultFinishCallback);

}




void ElasticClient::run() {
  for(int level = 0; level < cn_num; level++) {
    for(int i = 0; i < io->input[level].size(); i++) {
      EnqueueWriteBuffer(level, io->input[level][i].clmem, CL_TRUE, 0, io->input[level][i].size, io->input[level][i].address, 0, NULL, 0);
    }
    EnqueueNDRangeKernel(level, this->kernel_vector[level], this->dim, NULL, &(this->work_item[level]), 0, 0, NULL, NULL);

    pthread_t pid;
    RunThreadInput *input = new RunThreadInput();
    input->level = level;
    input->elasticClient = this;
    pthread_create(&pid, NULL, runThread, input);
    run_pid_vector.push_back(pid);
  }
  //wait all thread finish
  for(int i = 0; i < run_pid_vector.size(); i++){
    pthread_join(run_pid_vector[i], NULL);
  }

}

void ElasticClient::setFinishCallback(void (*finishCallback)(int level)){
  this->finishCallback = finishCallback;
}

void ElasticClient::defaultFinishCallback(int level){
  printf("level: %d is finish\n", level);
}


void* catcher(void *data) {
  newscl::SocketIOClient *conn = (reinterpret_cast<CatchData*>(data))->conn_;
  CatchData *catch_data = reinterpret_cast<CatchData*>(data);
  int size = (reinterpret_cast<CatchData*>(data))->size_;
  char *dest = (reinterpret_cast<CatchData*>(data))->dest_;
  bool ack = true;
  conn->Recv(dest, size);
  //fprintf(stderr, "recv OK");
  conn->Send(reinterpret_cast<char*>(&ack), sizeof(ack));
  delete catch_data;
  return NULL;
}

void ElasticClient::Paste(char *buf, size_t *buf_offset, void *ptr, size_t size) {
  memcpy(buf + *buf_offset, ptr, size);
  *buf_offset += size;
}

double& ElasticClient::GetWeight(double const &t_weight){
  static double weight = t_weight;
  static double is_init = false;
  return weight;
}

SocketIOClient* ElasticClient::GetDispatcherConnection() {
  static SocketIOClient conn(newscl::kDispatcherIP, newscl::kDispatcherPort);
  static bool is_init = false;
  if (!is_init) {
    if(conn.Connect() < 0){
      return NULL;
    }
    is_init = true;
  }
  return &conn;
}

SocketIOClient* ElasticClient::GetNonBlockingConnection(int no, const std::string kCNHost) {
  static std::vector<SocketIOClient> connVector;
  if(kCNHost.size() > 0) {
    SocketIOClient *conn = new SocketIOClient(kCNHost, kCNNonBlockingPort);
    int result = conn->Connect();
    int flag = 1;
    setsockopt(conn->sock(),
        IPPROTO_TCP,
        TCP_NODELAY,
        (char*) &flag,
        sizeof(int));
    //assert(result>=0);
    if(result >= 0){
      connVector.push_back(*conn);
    }
  }
  if(no < connVector.size()){
    return &(connVector[no]);
  } else {
    return NULL;
  }
}

SocketIOClient* ElasticClient::GetFinishConnection(int no, const std::string &ip) {
  static std::vector<SocketIOClient> connVector;
  if(ip.size() > 0) {
    SocketIOClient *conn = new SocketIOClient(ip, kCNFinishPort);
    int result = conn->Connect();
    int flag = 1;
    setsockopt(conn->sock(),
        IPPROTO_TCP,
        TCP_NODELAY,
        (char*) &flag,
        sizeof(int));
    //assert(result>=0);
    if(result >= 0){
      connVector.push_back(*conn);
    }
  }
  if(no < connVector.size()){
    return &(connVector[no]);
  } else {
    return NULL;
  }
}

SocketIOClient* ElasticClient::GetConnection(int no, const std::string &ip) {
  static std::vector<newscl::SocketIOClient> connVector;

  if(ip.size() > 0) {
    SocketIOClient *conn = new SocketIOClient(ip, kCNPort);
    int result = conn->Connect();
    int flag = 1;
    setsockopt(conn->sock(),
        IPPROTO_TCP,
        TCP_NODELAY,
        (char*) &flag,
        sizeof(int));
    //assert(result>=0);
    if(result >= 0){
      connVector.push_back(*conn);
    }
  }
  if(no < connVector.size()){
    return &(connVector[no]);
  } else {
    return NULL;
  }
}

int ElasticClient::GetDevices(std::string appName, int deadline, int levelCnt) {
  static int total_cn = 0;
  static bool is_init = false;
  bool accept;
  //elastic::KernelInfo kinfo(appName, deadline, levelCnt);
  if (!is_init) {
    std::array<char, kMsgMaxSize> send_buf;
    std::array<char, kMsgMaxSize> recv_buf;
    size_t buf_offset = 0;
    unsigned long int client_id = -1;
    int appID;

    {
      boost::property_tree::ptree pt;
      try {
        boost::property_tree::json_parser::read_json("/opt/client.json", pt);
      } catch(...){
      }
      client_id  = pt.get<decltype(client_id)>("id");
    }
    GetConnection(0, "127.0.0.1");  // connection init
    GetNonBlockingConnection(0, "127.0.0.1");  // connection init
    GetFinishConnection(0, "127.0.0.1");  // connection init
    if(GetConnection(total_cn)){
      levelCnt--;
    }
    std::string ip[levelCnt];
    int msg_type_num = dispatcher_msg_type::kGetBestCN;

    if(levelCnt > 0) {
      {//Pack
        Paste(send_buf.data(),
            &buf_offset,
            static_cast<void*>(&msg_type_num),
            sizeof(msg_type_num));
        int nameSize = appName.size();
        Paste(send_buf.data(),
            &buf_offset,
            static_cast<void*>(&nameSize),
            sizeof(nameSize));
        Paste(send_buf.data(),
            &buf_offset,
            const_cast<char*>(appName.data()),
            nameSize);
        Paste(send_buf.data(),
            &buf_offset,
            static_cast<void*>(&deadline),
            sizeof(deadline));
        Paste(send_buf.data(),
            &buf_offset,
            static_cast<void*>(&levelCnt),
            sizeof(levelCnt));
        //client_id一定要最後包 不然會出錯
        Paste(send_buf.data(),
            &buf_offset,
            static_cast<void*>(&client_id),
            sizeof(client_id));

           //printf("nameSize:%d\n", nameSize);
           //printf("appName:%s\n", appName.data());
          // printf("deadline:%d\n", deadline);
          // printf("levelCnt:%d\n", levelCnt);
        //strcpy (tmp, send_buf.data(), kMsgMaxSize);

      }//Pack end
      if(GetDispatcherConnection()) {
        GetDispatcherConnection()->Send(send_buf.data(), send_buf.size());
        for (int i = 0; i < levelCnt && GetDispatcherConnection()->Recv(recv_buf.data(), recv_buf.size()) > 0; i++){ 
          //要收到CN的IP
          //double client_weight = *(reinterpret_cast<double*>(recv_buf.data()));
          ip[i] = recv_buf.data();
          printf("IP[%d]:%s\n",i, ip[i].data());

        }
      }
    }
    msg_type_num = cn_msg_type::kReg;
    send_buf.fill(0);
    recv_buf.fill(0);
    buf_offset = 0;
    Paste(send_buf.data(), &buf_offset, &msg_type_num, sizeof(msg_type_num));

    if(GetConnection(total_cn)){
      GetConnection(total_cn)->Send(send_buf.data(), send_buf.size());
      GetConnection(total_cn)->Recv(recv_buf.data(), send_buf.size());
      total_cn++;
    }

    for (int i = 0; i< levelCnt; i++) {
      //fprintf(stderr, "app weight:%lf\n", client_weight);
      //auto idx = ip[i].find(' ');
      //if (idx != std::string::npos) {
      //  ip[i].erase(ip[i].begin() + ip[i].find(' '), ip[i].end());
      //}
      //   fprintf(stderr, "get connection() %d\n",i);
      GetConnection(0, ip[i]);  // connection init
      //   fprintf(stderr, "get non_blocking connection() %d\n",i);
      GetNonBlockingConnection(0, ip[i]);  // connection init
      //   fprintf(stderr, "get finish connection() %d\n", i);
      GetFinishConnection(0, ip[i]);  // connection init
      //   fprintf(stderr, "get connection() end %d\n", i);
      //GetDataConnection(ip);  // connection init
      //platform[i].dispatch = GetDispatch();  

      if(GetConnection(total_cn)){
        GetConnection(total_cn)->Send(send_buf.data(), send_buf.size());
        GetConnection(total_cn)->Recv(recv_buf.data(), send_buf.size());
        total_cn++;
      }
      //fprintf(stderr, "reg echo: %s\n", recv_buf.data());
    } //for
    is_init = true;
  }// if
  return total_cn;
}


int ElasticClient::GetDevice() {
#ifdef DEBUG
  fprintf(stderr, "GetDevice()\n");
#endif
  // static _cl_icd_dispatch dispatch;
  static bool is_init = false;
  static int total_cn = 0;
  bool accept;
  if (!is_init) {
    std::array<char, kMsgMaxSize> send_buf;
    std::array<char, kMsgMaxSize> recv_buf;
    size_t buf_offset = 0;
    unsigned long int client_id = -1;
    {
      boost::property_tree::ptree pt;
      try
      {
        boost::property_tree::json_parser::read_json("/opt/client.json", pt);
      } catch(...)
      {

      }
      client_id  = pt.get<decltype(client_id)>("id");
    }

    int msg_type_num = dispatcher_msg_type::kGetBindingCN;
    Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
    Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&client_id),
        sizeof(client_id));

    //    GetDispatcherConnection()->Send(send_buf.data(), send_buf.size());
    //    GetDispatcherConnection()->Recv(recv_buf.data(), recv_buf.size());
    double client_weight = *(reinterpret_cast<double*>(recv_buf.data()));
    
    msg_type_num = cn_msg_type::kReg;

    send_buf.fill(0);
    recv_buf.fill(0);
    buf_offset = 0;
    Paste(send_buf.data(), &buf_offset, &msg_type_num, sizeof(msg_type_num));
    Paste(send_buf.data(), &buf_offset, &client_weight, sizeof(client_weight));

   
    GetConnection(0, "127.0.0.1");  // connection init
    GetNonBlockingConnection(0, "127.0.0.1");  // connection init
    GetFinishConnection(0, "127.0.0.1");  // connection init
    if(GetConnection(total_cn)){
      GetConnection(total_cn)->Send(send_buf.data(), send_buf.size());
      GetConnection(total_cn)->Recv(recv_buf.data(), send_buf.size());
      total_cn++;
    }

    GetConnection(0, "140.112.28.114");  // connection init
    GetNonBlockingConnection(0, "140.112.28.114");  // connection init
    GetFinishConnection(0, "140.112.28.114");  // connection inita    
    if(GetConnection(total_cn)){
      GetConnection(total_cn)->Send(send_buf.data(), send_buf.size());
      GetConnection(total_cn)->Recv(recv_buf.data(), send_buf.size());
      total_cn++;
    }
    is_init = true;
  }
  return total_cn;
}

cl_mem ElasticClient::CreateBuffer(int devices,
    cl_mem_flags flags,
    size_t       size,
    void *       host_ptr,
    cl_int *     errcode_ret) {

  if (flags & CL_MEM_ALLOC_HOST_PTR == CL_MEM_USE_HOST_PTR) {
    *errcode_ret = CL_INVALID_VALUE;
  }
  static unsigned int mem_id[3] = {0};
  cl_mem mem = new _cl_mem;

  char send_buf[kMsgMaxSize], recv_buf[kMsgMaxSize];
  size_t buf_offset = 0;
  int msg_type_num = newscl::cn_msg_type::kCreateBuffer;
  bzero(send_buf, kMsgMaxSize);
  Paste(send_buf,
      &buf_offset,
      static_cast<void*>(&msg_type_num),
      sizeof(msg_type_num));
  Paste(send_buf, &buf_offset, static_cast<void*>(&flags), sizeof(flags));
  Paste(send_buf, &buf_offset, static_cast<void*>(&size), sizeof(size));

  GetConnection(devices)->Send(send_buf, kMsgMaxSize);
  if ((flags & CL_MEM_COPY_HOST_PTR) == CL_MEM_COPY_HOST_PTR) {
    GetConnection(devices)->Send(reinterpret_cast<const char *>(host_ptr), size);
  }
  if(errcode_ret){
    *errcode_ret = CL_SUCCESS;
  }
  mem->id = mem_id[devices]++;
  mem->type = CL_MEM_OBJECT_BUFFER;
  mem->size = size;
  mem->flags = flags;

#ifdef DEBUG_
  if ( host_ptr == NULL) {
    fprintf(stderr, "create mem_id:%d size:%ld\n", mem->id, size);
  } else {
    fprintf(stderr, "create size:%ld have data transfer\n", size);
  }
#endif //DEBUG_
  return mem;
}

cl_int ElasticClient::BuildProgram(int devices, cl_program program,
    cl_uint num_devices,
    const cl_device_id *device_list,
    const char *options,
    void (*pfn_notify)(cl_program, void *user_data),
    void *user_data) {
  // disabled options, and pfn_notify and user_data
  char send_buf[kMsgMaxSize];
  char recv_buf[kMsgMaxSize];
  int msg_type_num = cn_msg_type::kBuildProgram;
  size_t buf_offset = 0;
  int error_code;
/*  if (device_list != NULL &&(num_devices != 1 || device_list[0] != GetDevice())) {
    return CL_INVALID_DEVICE;
  }*/
  unsigned int options_leng = options != NULL ? strlen(options) : 0;

  bzero(send_buf, kMsgMaxSize);
  Paste(send_buf,
      &buf_offset,
      reinterpret_cast<void*>(&msg_type_num),
      sizeof(msg_type_num));
  Paste(send_buf,
      &buf_offset,
      reinterpret_cast<void*>(&(program->id)),
      sizeof(num_devices));
  Paste(send_buf,
      &buf_offset,
      reinterpret_cast<void*>(&(options_leng)),
      sizeof(options_leng));
  GetConnection(devices)->Send(send_buf, kMsgMaxSize);
  if (options_leng != 0)
  {
    GetConnection(devices)->Send(options, options_leng);
  }
  GetConnection(devices)->Recv(recv_buf, kMsgMaxSize);
  error_code = *(reinterpret_cast<int*>(recv_buf));
  return error_code;
}

cl_program ElasticClient::CreateProgramWithSource(
  int       devices,
  cl_uint           count ,
  const char **     strings ,
  const size_t *    lengths ,
  cl_int *          errcode_ret) {

  static int id = 0;
  ++id;
  cl_program p_program = new _cl_program;
  int msg_type_num = cn_msg_type::kCreateProgramWithSource;

  char buf[kMsgMaxSize], recv_buf[kMsgMaxSize];
  int buf_offset = 0;
  bzero(buf, kMsgMaxSize);
  memcpy(buf + buf_offset,
  reinterpret_cast<char*>(&msg_type_num),
  sizeof(msg_type_num));
  buf_offset += sizeof(msg_type_num);
  memcpy(buf + buf_offset, reinterpret_cast<char*>(&count), sizeof(count));
  buf_offset += sizeof(count);
  memcpy(buf + buf_offset, reinterpret_cast<char*>(&id), sizeof(id));
  buf_offset += sizeof(count);
  GetConnection(devices)->Send(buf, kMsgMaxSize);

  size_t *send_lengths = new size_t[count];

  p_program->code = "";
  for (int i = 0; i < count; ++i) {
    if (lengths == NULL || lengths[i] == 0) {
      send_lengths[i] = strlen(strings[0]);
    } else {
      send_lengths[i] = lengths[i];
    }
    GetConnection(devices)->Send( reinterpret_cast<char*>(const_cast<size_t*>(&send_lengths[i])), sizeof(size_t));
  }
  for (int i = 0; i < count; ++i) {
    GetConnection(devices)->Send(strings[i], send_lengths[i]);
    p_program->code += "\n";
    p_program->code += std::string(strings[i], send_lengths[i]);
  }
  Preprocessor(p_program->code);
  GetConnection(devices)->Recv(recv_buf, kMsgMaxSize);
  p_program->id = *reinterpret_cast<unsigned int*>(recv_buf);
  if(errcode_ret){
    *errcode_ret = *reinterpret_cast<cl_int*>(recv_buf + sizeof(p_program->id));
  }
  delete [] send_lengths;
  return p_program;
}

cl_kernel ElasticClient::CreateKernel(int devices, cl_program  program,
                                 const char *kernel_name,
                                 cl_int *errcode_ret) {
  char send_buf[kMsgMaxSize];
  char recv_buf[kMsgMaxSize];
  int msg_type_num = cn_msg_type::kCreateKernel;
  size_t buf_offset = 0;
  size_t kernel_name_length = strlen(kernel_name);
  _cl_kernel *kernel = new _cl_kernel;
  kernel->program = program;
  kernel->kernel_name = std::string(kernel_name);

  bzero(send_buf, kMsgMaxSize);
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&(program->id)),
        sizeof(program->id));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&kernel_name_length),
        sizeof(kernel_name_length));

  GetConnection(devices)->Send(send_buf, kMsgMaxSize);
  GetConnection(devices)->Send(kernel_name, kernel_name_length);
  GetConnection(devices)->Recv(recv_buf, kMsgMaxSize);

  cl_int actual_errcode_ret = *(reinterpret_cast<cl_int*>(recv_buf));
  if (errcode_ret != NULL)
    *errcode_ret = actual_errcode_ret;

  sscanf(recv_buf + sizeof(cl_int), "kernel id:%d", &(kernel->id));
  if (actual_errcode_ret == CL_SUCCESS) {
    std::vector<std::string> arg_list;
    newscl::FindFuncArgList(program->code, kernel_name, arg_list);
    if (arg_list.size() > 127) {
      perror("The length of parameter list cannot excceed 127");
    }
    kernel->is_pointer.resize(arg_list.size());
    for (int i = 0; i < kernel->is_pointer.size(); ++i) {
      kernel->is_pointer[i] = newscl::IsPointer(arg_list[i]);
    }
  }
  return kernel;
}

cl_int ElasticClient::SetKernelArg(
    cl_kernel kernel,
    cl_uint arg_index,
    size_t arg_size,
    const void *arg_value) {
  if (arg_index < 0 || arg_index >= kernel->is_pointer.size()) {
    return CL_INVALID_ARG_INDEX;
  }
  bool is_pointer = kernel->is_pointer[arg_index];
  if (is_pointer) {
    if (arg_value == NULL) {
      kernel->changed_mem_id.push_back({arg_index, 2147483247});
    } else {
      cl_mem mem = *(static_cast<cl_mem*>(const_cast<void*>(arg_value)));
      //fprintf(stderr, "Arg index:%d mem_id:%d kernel_id:%d\n", arg_index, mem->id, kernel->id);
      kernel->changed_mem_id.push_back({arg_index, mem->id});
    }
    return CL_SUCCESS;
  } else {
    //fprintf(stderr, "Arg index:%d size:%ld kernel_id:%d data:", arg_index, arg_size, kernel->id);
   // const char *targ_value = (const char*)(arg_value);
    //for(int i = 0; i < arg_size; ++i){
    //  fprintf(stderr, "%d ", targ_value[i]);
    //}
    //fprintf(stderr, "\n");
    kernel->const_data_info.push_back({(unsigned char)(arg_index), arg_size});
    std::copy(reinterpret_cast<const char*>(arg_value), reinterpret_cast<const char*>(arg_value) + arg_size, std::back_inserter(kernel->const_data));
    return CL_SUCCESS;
  }
}

cl_int ElasticClient::EnqueueWriteBuffer(
    int devices,
    cl_mem buffer,
    cl_bool blocking_write,
    size_t offset,
    size_t cb,
    const void *ptr,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event) {
  //if (blocking_write == false) {
    //printf("non-blocking_write\n");
  //}
  if (num_events_in_wait_list != 0) {
    fprintf(stderr, "we do not support event\n");
    return CL_INVALID_EVENT_WAIT_LIST;
  }
#ifdef DEBUG_
  fprintf(stderr, "write mem_id:%d size:%ld offset:%ld cb:%ld\n", buffer->id, buffer->size, offset, cb);
#endif //DEBUG_
  char send_buf[kMsgMaxSize], recv_buf[kMsgMaxSize];
  size_t buf_offset = 0;
  int msg_type_num = newscl::cn_msg_type::kEnqueueWriteBuffer;
  bzero(send_buf, kMsgMaxSize);
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&offset),
        sizeof(offset));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&cb),
        sizeof(cb));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&buffer->id),
        sizeof(buffer->id));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&blocking_write),
        sizeof(blocking_write));
  GetConnection(devices)->Send(send_buf, kMsgMaxSize);
  GetConnection(devices)->Send(reinterpret_cast<char*>(const_cast<void*>(ptr)), cb);
  if (blocking_write) {
    GetConnection(devices)->Recv(recv_buf, kMsgMaxSize);
    return *(reinterpret_cast<cl_int*>(recv_buf));
  } else {
    return CL_SUCCESS;
  }
}
cl_int ElasticClient::EnqueueNDRangeKernel(int devices,
                                      cl_kernel kernel,
                                      cl_uint work_dim,
                                      const size_t *global_work_offset,
                                      const size_t *global_work_size,
                                      const size_t *local_work_size,
                                      cl_uint num_events_in_wait_list,
                                      const cl_event *event_wait_list,
                                      cl_event *event) {
  size_t *tlocal_work_size = const_cast<size_t*>(local_work_size);

  if (num_events_in_wait_list > 0) {
    fprintf(stderr, "Sorry, we do not support event functionality\n");
    return CL_INVALID_EVENT_WAIT_LIST;
  }
  if (event != NULL) {
    fprintf(stderr, "Sorry, we do not support event functionality\n");
    //return CL_INVALID_EVENT_WAIT_LIST;
  }
  if (local_work_size == NULL) {
    tlocal_work_size = new size_t[work_dim];
    bzero(tlocal_work_size, sizeof(size_t) * work_dim);
  }

#ifdef DEBUG_
  fprintf(stderr, "EnqueueNDRKernel kernel_id:%d\n", kernel->id);
#endif //DEBUG_
  char send_buf[kMsgMaxSize]={}, recv_buf[kMsgMaxSize]={};
  bzero(send_buf, kMsgMaxSize);
  size_t buf_offset = 0;
  int msg_type_num = newscl::cn_msg_type::kEnqueueNDRangeKernel;
  unsigned char t_work_dim = work_dim;
  bzero(send_buf, kMsgMaxSize);
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&(kernel->id)),
        sizeof(kernel->id));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&(t_work_dim)),
        sizeof(t_work_dim));

  for (int i = 0; i < work_dim; ++i) {
    Paste(send_buf,
          &buf_offset,
          static_cast<void*>(const_cast<size_t*>(&(global_work_size[i]))),
          sizeof(global_work_size[i]));
  }
  buf_offset += (3 - work_dim) * sizeof(global_work_size[0]);

  for (int i = 0; i < work_dim; ++i) {
    Paste(send_buf,
          &buf_offset,
          static_cast<void*>(const_cast<size_t*>(&(tlocal_work_size[i]))),
          sizeof(tlocal_work_size[i]));
  }

  buf_offset += (3 - work_dim) * sizeof(tlocal_work_size[0]);
  unsigned char num_of_changed_num_id = kernel->changed_mem_id.size();
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&num_of_changed_num_id),
        sizeof(num_of_changed_num_id));
  unsigned char num_of_const_arg = kernel->const_data_info.size();
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&num_of_const_arg),
        sizeof(num_of_const_arg));
  unsigned int const_data_size = kernel->const_data.size();
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&(const_data_size)),
        sizeof(const_data_size));
  
  GetConnection(devices)->Send(send_buf, kMsgMaxSize);
  char arg_send_buf[kMsgMaxSize];
  const int max_mem_in_msg = kMsgMaxSize /
                             (sizeof(unsigned char) + sizeof(unsigned int));
  const int mem_send_times = (kernel->changed_mem_id.size() + max_mem_in_msg - 1) / max_mem_in_msg;
  //assert(mem_send_times<2);
  for (int i = 0; i < mem_send_times; ++i) {
    size_t arg_buf_offset = 0;
    const int end_j = 
      (i == mem_send_times - 1?kernel->changed_mem_id.size() : (i + 1) * max_mem_in_msg);
    for (int j = i * max_mem_in_msg; j < end_j ; ++j) {
      Paste(arg_send_buf,
            &arg_buf_offset,
            static_cast<void*>(&(kernel->changed_mem_id[j].first)),
            sizeof(kernel->changed_mem_id[j].first));
      Paste(arg_send_buf,
            &arg_buf_offset,
            static_cast<void*>(&(kernel->changed_mem_id[j].second)),
            sizeof(kernel->changed_mem_id[j].second));
    }
    GetConnection(devices)->Send(arg_send_buf, kMsgMaxSize);
  }
  if (const_data_size > 0) {
    GetConnection(devices)->Send(kernel->const_data.data(), kernel->const_data.size());
  }

  char const_send_buf[kMsgMaxSize];
  size_t const_buf_offset = 0;

  const int kMaxConstInfoInMsg = kMsgMaxSize /
                                    (sizeof(unsigned char) + sizeof(size_t));
  const int kConstSendTimes = ( num_of_const_arg + kMaxConstInfoInMsg - 1)
                              / kMaxConstInfoInMsg;
  //assert(kConstSendTimes<2);
  //if(kConstSendTimes>1 && kernel->kernel_name == "flipFloatKernel"){
  //  int a = 2;
  //  a++;
  //}
  for (int i = 0; i < kConstSendTimes; ++i) {
    const int kEndJ = 
      (i == kConstSendTimes - 1?kernel->const_data_info.size() : (i + 1) * kMaxConstInfoInMsg);
    for (int j = i * kMaxConstInfoInMsg; j < kEndJ ; ++j) {
      Paste(const_send_buf,
            &const_buf_offset,
            static_cast<void*>(&(kernel->const_data_info[j].arg_idx)),
            sizeof(kernel->const_data_info[j].arg_idx));
      Paste(const_send_buf,
            &const_buf_offset,
            static_cast<void*>(&(kernel->const_data_info[j].size)),
            sizeof(kernel->const_data_info[j].size));
    }
    GetConnection(devices)->Send(const_send_buf, kMsgMaxSize);
  }
  //fprintf(stderr, "const_data_size:%d, memSendTime:%d, kConstSendTime:%d\n", const_data_size, mem_send_times, kConstSendTimes);

  //GetConnection()->Recv(recv_buf, kMsgMaxSize);
  //errcode_ret = *(reinterpret_cast<cl_int*>(recv_buf));

  kernel->changed_mem_id.clear();
  kernel->const_data_info.clear();
  kernel->const_data.clear();
  if (local_work_size == NULL) {
    delete[] tlocal_work_size;
  }
  return CL_SUCCESS;
}

cl_int ElasticClient::EnqueueReadBuffer(
    int devices,
    cl_mem buffer,
    cl_bool blocking_read,
    size_t offset,
    size_t cb,
    void *ptr,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event) {

  //if (blocking_read == false) {
    //printf("non-blocking_read\n");
  //}
  if (num_events_in_wait_list != 0) {
    fprintf(stderr, "we do not support event\n");
    return CL_INVALID_EVENT_WAIT_LIST;
  }
#ifdef DEBUG_
  fprintf(stderr, "read mem_id:%d size:%ld offset:%ld cb:%ld\n", buffer->id, buffer->size, offset, cb);
#endif //DEBUG_
  char send_buf[kMsgMaxSize], recv_buf[kMsgMaxSize];
  size_t buf_offset = 0;
  int msg_type_num = newscl::cn_msg_type::kEnqueueReadBuffer;
  bzero(send_buf, kMsgMaxSize);
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf, &buf_offset, static_cast<void*>(&offset), sizeof(offset));
  Paste(send_buf, &buf_offset, static_cast<void*>(&cb), sizeof(cb));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&buffer->id),
        sizeof(buffer->id));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&blocking_read),
        sizeof(cl_bool));
  GetConnection(devices)->Send(send_buf, kMsgMaxSize);

  if (blocking_read) {
    //GetConnection()->Recv(recv_buf, kMsgMaxSize);
    GetNonBlockingConnection(devices)->Recv(reinterpret_cast<char*>(ptr), cb);
    //return *(reinterpret_cast<cl_int*>(recv_buf));
    return CL_SUCCESS;
  } else {
    //pthread_t *pid = new pthread_t();  // memory leak
    pthread_t pid;
    CatchData *catch_data = new CatchData(GetNonBlockingConnection(devices),
                                          cb,
                                          reinterpret_cast<char*>(ptr));
    if (pthread_create(&pid,
                       NULL,
                       catcher,
                       static_cast<void*>(catch_data)) != 0) {
      fprintf(stderr, "create thread failed\n");
    }
    return CL_SUCCESS;
  }
}


cl_program ElasticClient::load_program(int devices, const char* filename)
{
  std::ifstream in(filename, std::ios_base::binary);
  if(!in.good()) {
    return 0;
  }

  // get file length
  in.seekg(0, std::ios_base::end);
  size_t length = in.tellg();
  in.seekg(0, std::ios_base::beg);

  // read program source
  std::vector<char> data(length + 1);
  in.read(&data[0], length);
  data[length] = 0;
  // create and build program 
  const char* source = &data[0];
  cl_program program = CreateProgramWithSource(devices,  1, &source, 0, 0);
  if(program == 0) {
    return 0;
  }

  if(BuildProgram(devices, program, 0, 0, 0, 0, 0) != CL_SUCCESS) {
    return 0;
  }

  return program;
}

cl_int ElasticClient::GetProgramBuildInfo(cl_program program,
    cl_device_id device,
    cl_program_build_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret)
{
 /* if (device != GetDevice()){
    return CL_INVALID_DEVICE;
  }*/
  switch (param_name)
  {
    case CL_PROGRAM_BUILD_STATUS:
    {
      if (param_value_size_ret != NULL)
        *param_value_size_ret = sizeof(cl_build_status);
      if(param_value != NULL)
        *reinterpret_cast<cl_build_status*>(param_value) = CL_BUILD_SUCCESS;
      break;
    }
    case CL_PROGRAM_BUILD_OPTIONS:
    {
      if (param_value_size_ret != NULL)
        *param_value_size_ret = 1;
      if(param_value != NULL)
        *reinterpret_cast<char*>(param_value) = '\0';
      break;
    }
    case CL_PROGRAM_BUILD_LOG:
    {
      if (param_value_size_ret != NULL)
        *param_value_size_ret = 1;
      if(param_value != NULL)
        *reinterpret_cast<char*>(param_value) = '\0';
      break;
    }
    default:
    {
      return CL_INVALID_VALUE;
    }
  }
  return CL_SUCCESS;
}
int main(){}
