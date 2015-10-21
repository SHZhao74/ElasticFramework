// Copyright [2013] (JoenChen: joen@joen.cc)
#ifndef NEWSCL_CN_CN_H_
#define NEWSCL_CN_CN_H_
#include <semaphore.h>

#include <vector>

#include "./VMAFS_N_Scheduler.h"
#include "./command.h"
extern std::vector<cl_platform_id> platforms;
extern std::vector<cl_device_id> devs;
extern std::vector<cl_command_queue> queues;
extern std::vector<cl_context> contexts;
#endif  // NEWSCL_CN_CN_H_
