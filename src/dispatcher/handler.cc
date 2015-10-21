// Copyright [2013] (JoenChen)
#include "./handler.h"

#include <array>

#include <cstring>

#include "./VMAFS_N_Dispatcher.h"
#include "./cn.h"
#include "newscl.h"
#include "Scoring.h"
#include "kernel.h"
auto &cn_list = newscl::CNList::Instance();

static void Paste(char *buf, size_t *buf_offset, void const *ptr, size_t size) {
  memcpy(buf + *buf_offset, ptr, size);
  *buf_offset += size;
}
void newscl::HandlerUpdateScore(const int &sock,
                                 std::array<char, newscl::kMsgMaxSize> &buf) {
  //CN直接傳JSON檔過來，收完之後依照CN ID為檔名存起來
  using namespace dispatcher_msg_type;
	//CNInfo::HASH_KEY_T key;
	char end[3];
	auto &cn_list = CNList::Instance();
  int recv_size = 0, total_recv_size = 0;
  size_t buf_offset = sizeof(kMsgMaxSize);

  //recv file
  CNInfo::HASH_KEY_T cn_id =
    *(reinterpret_cast<CNInfo::HASH_KEY_T*>(buf.data() + sizeof(kLoadInfo)));
  printf("Update CN%d score...\n", cn_id);
  std::string filename = "history"+std::to_string(cn_id)+".json";
  FILE *fp = fopen(filename.data(),"w");
  bzero(buf.data(), kMsgMaxSize);
  while ((recv_size = ::recv(sock,
                             buf.data(),
                             kMsgMaxSize, 0)) > 0){
      //buf_offset = read(sock, buf, buf.size());
      //printf("recv_size=%d\n%s\n", recv_size, buf.data());
      strncpy(end , buf.data(),3);
      if (strcmp(end, "END") == 0) {
        printf("Recv score end\n");
        fclose(fp);
        break;
      }
      fwrite(buf.data(),sizeof(char),recv_size,fp);
      bzero(buf.data(), kMsgMaxSize);
  }
  //recv end
  send(sock, "END", 3,0);
  
  

  cn_list->Lock();
	cn_list->UpdateCNScore(cn_id);
  cn_list->Unlock();
	
}
void newscl::HandlerGetBestCN(const int &sock,
                                 std::array<char, newscl::kMsgMaxSize> &buf) {
  using namespace std;
  //double app_weight = 1.f;
  size_t buf_offset = 0;
  
  //parse buf
    buf_offset += sizeof(kMsgMaxSize);
    int nameSize = *(reinterpret_cast<int*>(buf.data() + buf_offset));
    buf_offset += sizeof(nameSize);
    std::string appName (buf.data() + buf_offset, buf.data() + buf_offset + nameSize); 
    buf_offset += nameSize;
    int deadline =
      *(reinterpret_cast<int*>(buf.data() + buf_offset)); 
    buf_offset += sizeof(deadline);
    int levelCnt =
      *(reinterpret_cast<int*>(buf.data() + buf_offset)); 
    buf_offset += sizeof(levelCnt);

    CNInfo::HASH_KEY_T client_id =
      *(reinterpret_cast<CNInfo::HASH_KEY_T*>(buf.data() + buf_offset));  
    buf_offset += sizeof(client_id);
    
    
    printf("client_id:%d\n", client_id);
    printf("nameSize:%d\n", nameSize);
    printf("appName:%s\n", appName.data());
    printf("deadline:%d\n", deadline);
    printf("levelCnt:%d\n", levelCnt);
    
    
    
    elastic::KernelInfo kinfo (appName, deadline, levelCnt);
    //buf_offset += sizeof(kinfo);
    //printf("kinfo.appName:%s\n", kinfo.appName());
    //printf("kinfo.deadline:%d\n", kinfo.deadline());
    //printf("kinfo.levelCnt:%d\n", kinfo.levelCnt());
	//parse end
  //
  CNInfo::HASH_KEY_T* key = elastic::Scoring::Instance()->chooseBestCN(client_id, kinfo);
	std::string ip[levelCnt], fuck ("127.0.0.1");//140.112.28.114
	for (int i = 0; i < levelCnt; ++i)
  {
      auto& cn = CNList::Instance()->GetCN(key[i]);
      ip[i]=cn.IP();
	}
  //cn.set_weight(cn.weight() + 1);
  //fprintf(stderr, "key:%d\n", key);
  //fprintf(stderr, "cn size:%ld\n", CNList::Instance()->CNs_.size());
  //fprintf(stderr, "cn weight:%f\n", cn.weight());
  //cn_list->UpdateCNWeight(cn_id, weight);

  std::array<char, kMsgMaxSize> send_buf;
  send_buf.fill(0);
  
  //Paste(send_buf.data(), &buf_offset, &app_weight, sizeof(app_weight));
  printf("sending IP\n");
  for (int i=0; i< levelCnt; i++){
    //ip[i] = fuck;
    //if (i==0) ip[i] = "140.112.28.114";
    //else ip[i] = "140.112.28.100";
    printf("IP %d:%s\n", i, ip[i].data());
    buf_offset = 0;
  	Paste(send_buf.data(), &buf_offset, ip[i].data(), ip[i].length());
    SockSend(sock, send_buf.data(), send_buf.size());
    //SockSend(sock, ip[i].data(), ip[i].size());
  }
  ////
  //
}

void newscl::HandlerGetBindingCN(const int &sock,
                                 std::array<char, newscl::kMsgMaxSize> &buf) {
  // double app_weight = *(reinterpret_cast<double*>(buf.data() + 1));
  double app_weight = 1.f;
  CNInfo::HASH_KEY_T key;
  key = VMAFS_N_Dispatcher::Instance()->Dispatch(app_weight);
  auto &cn = CNList::Instance()->GetCN(key);
  cn.set_weight(cn.weight() + 1);
  fprintf(stderr, "key:%d\n", key);
  fprintf(stderr, "cn size:%ld\n", CNList::Instance()->CNs_.size());
  fprintf(stderr, "cn weight:%f\n", cn.weight());
  //cn_list->UpdateCNWeight(cn_id, weight);

  std::array<char, kMsgMaxSize> send_buf = {};
  //send_buf.fill(0);
  size_t buf_offset = 0;
  Paste(send_buf.data(), &buf_offset, &app_weight, sizeof(app_weight));
  Paste(send_buf.data(), &buf_offset, cn.IP().data(), cn.IP().size());

  SockSend(sock, send_buf.data(), send_buf.size());
}

int newscl::HandlerCNReg(const int &sock,
                          std::array<char, newscl::kMsgMaxSize> &buf) {
  double capacity = *(reinterpret_cast<double*>(
        buf.data() + sizeof(dispatcher_msg_type::kCNReg)));
  std::string IP;
  {
    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr); // important! Must initialize length, garbage produced otherwise
    if (getpeername(sock, (sockaddr*) &addr, &socklen) < 0) {
    }
    else {
      IP = std::string(inet_ntoa(addr.sin_addr));
      printf("Address: %s, port: %d\n", IP.c_str(), ntohs(addr.sin_port));
    }
  }

  CNInfo::HASH_KEY_T cn_id = -1;
  cn_list->Lock();
  {
    cn_id = cn_list->AddCN(IP, capacity);
  }

  printf("capacity:%lf\n", capacity);
  cn_list->Unlock();
  SockSend(sock, reinterpret_cast<char*>(&cn_id), sizeof(cn_id));
  return cn_id;
}

void newscl::HandlerLoadInfo(const int&sock,
                             std::array<char, newscl::kMsgMaxSize> &buf) {
  using namespace dispatcher_msg_type;
  CNInfo::HASH_KEY_T cn_id =
    *(reinterpret_cast<CNInfo::HASH_KEY_T*>(buf.data() + sizeof(kLoadInfo)));
  double weight = *(reinterpret_cast<double*>(buf.data() +
                                              sizeof(kLoadInfo) +
                                              sizeof(cn_id)));
  double used_time = *(reinterpret_cast<double*>(buf.data() +
                                                 sizeof(kLoadInfo) +
                                                 sizeof(cn_id) +
                                                 sizeof(weight)));
  cn_list->Lock();
  {
    cn_list->UpdateCNWeight(cn_id, weight);
    cn_list->UpdateCNUsedTime(cn_id, used_time);
  }
  cn_list->Unlock();

  CNInfo cn = cn_list->GetCN(cn_id);
  printf("Update cn_id:%d ip:%s weight:%lf used_time:%lf\n", cn_id, cn.IP().c_str(), weight, used_time);
}
