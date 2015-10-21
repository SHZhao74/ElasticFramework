#include "IO.h"
#include "CL/cl.h"
#include "CL/opencl.h"
#include "CL/cl_ext.h"
#include <pthread.h>
#include <vector>
using namespace std;


class ElasticClient;

struct CatchData {
  newscl::SocketIOClient *conn_;
  int size_;
  char *dest_;
  CatchData(newscl::SocketIOClient *conn,
            int size,
            char *dest) : conn_(conn), size_(size), dest_(dest) {
  }
};

typedef struct RunThreadInput{
  unsigned char level;
  ElasticClient *elasticClient;
}RunThreadInput;


typedef struct IO{
  void* address;
  size_t size;
  cl_mem clmem;
}IO;

class ElasticIO{
  private:
    void setIO(vector<IO> *io, int &group, int &arg_num, void* buffer_address, size_t &size){
      if(group > level) {
        return;
      }
      //the argument is exit
      if(io[group].size() > arg_num){
        io[group][arg_num].address = buffer_address;
        io[group][arg_num].size = size;
      } else {
        IO io_element;
        io_element.address = buffer_address;
        io_element.size = size;
        io[group].push_back(io_element);
      }
    }
  public:
    unsigned int level;
    vector<IO> *input;
    vector<IO> *output;
    
    ElasticIO(int level){
      this->level = level;
      input = new vector<IO>[level];
      output = new vector<IO>[level];
    }
    ~ElasticIO(){
      delete[] input;
      delete[] output;
    }
    void setInput(int group, int arg_num, void* buffer_address, size_t size){
      setIO(input, group, arg_num, buffer_address, size);
    }
    void setOutput(int group, int arg_num, void* buffer_address, size_t size){
      setIO(output, group, arg_num, buffer_address, size);
    }
};



class ElasticClient{

  public:
    vector<cl_kernel> kernel_vector;
    ElasticIO *io;
    unsigned char dim;
    size_t work_item[10];
    void (*finishCallback)(int level);


    void init(char* kernel_filename, char* kernel_name, ElasticIO &io);
    void setFinishCallback(void (*finishCallback)(int level));
    void run();
    static void *runThread(void* runThreadInput);


  private:
    int cn_num;
    pthread_t connect_device_thread;
    vector<pthread_t> run_pid_vector;

    cl_program load_program(int devices, const char* filename);
    void Paste(char *buf, size_t * offset, void *ptr, size_t size);
    double& GetWeight(double const &tweight);
    newscl::SocketIOClient* GetDispatcherConnection();
    newscl::SocketIOClient* GetNonBlockingConnection(int no = 0, const std::string kCNHost="");
    newscl::SocketIOClient* GetFinishConnection(int no = 0, const std::string &ip = "");
    newscl::SocketIOClient* GetConnection(int no = 0, const std::string &ip = "");

    cl_int EnqueueNDRangeKernel(int devices,
        cl_kernel kernel,
        cl_uint work_dim,
        const size_t *global_work_offset,
        const size_t *global_work_size,
        const size_t *local_work_size,
        cl_uint num_events_in_wait_list,
        const cl_event *event_wait_list,
        cl_event *event);
    cl_int EnqueueReadBuffer(
        int devices,
        cl_mem buffer,
        cl_bool blocking_read,
        size_t offset,
        size_t cb,
        void *ptr,
        cl_uint num_events_in_wait_list,
        const cl_event *event_wait_list,
        cl_event *event);

    static void defaultFinishCallback(int level);
    int GetDevices(std::string appName, int deadline, int levelCnt);
    int GetDevice();
    cl_mem CreateBuffer(int devices,
        cl_mem_flags flags,
        size_t       size,
        void *       host_ptr,
        cl_int *     errcode_ret);
    cl_int BuildProgram(int devices, cl_program program,
        cl_uint num_devices,
        const cl_device_id *device_list,
        const char *options,
        void (*pfn_notify)(cl_program, void *user_data),
        void *user_data);
    cl_program CreateProgramWithSource(
        int       devices,
        cl_uint           count ,
        const char **     strings ,
        const size_t *    lengths ,
        cl_int *          errcode_ret);
    cl_kernel CreateKernel(int devices, cl_program  program,
        const char *kernel_name,
        cl_int *errcode_ret);

    cl_int SetKernelArg(
        cl_kernel kernel,
        cl_uint arg_index,
        size_t arg_size,
        const void *arg_value);

    cl_int EnqueueWriteBuffer(
        int devices,
        cl_mem buffer,
        cl_bool blocking_write,
        size_t offset,
        size_t cb,
        const void *ptr,
        cl_uint num_events_in_wait_list,
        const cl_event *event_wait_list,
        cl_event *event);

    cl_int GetProgramBuildInfo(cl_program program,
        cl_device_id device,
        cl_program_build_info param_name,
        size_t param_value_size,
        void *param_value,
        size_t *param_value_size_ret);

};


