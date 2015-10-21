// Copyright [2013] (Joen Chen)
#include <array>

#include <cstdio>
#include <cstring>
#include <cassert>
#include <assert.h>
#include "newscl.h"
#include "config.h"
#include "IO.h"
#include "./handler.h"
#include "./cn.h"
#include "./dispatcher.h"
#include "./Scoring.h"
static inline int Parser(const std::array<char, newscl::kMsgMaxSize> &buf) {
    return buf[0];
}

void* Run(void* data) {
  using newscl::SocketIOServer;
  int sock = (static_cast<SocketIOServer::ThreadData*>(data))->client_sock;
  int recv_size = 0, total_recv_size = 0;
  std::array<char, newscl::kMsgMaxSize> buf;
  int client_type = -1;
  int cn_id = -1;

  while ((recv_size = recv(sock, buf.data() + total_recv_size,
                           newscl::kMsgMaxSize - total_recv_size, 0)) > 0) {
    total_recv_size += recv_size;
    if (static_cast<unsigned long long>(total_recv_size) == buf.size()) {
      printf("receive control message :");
      int msg_type;
      msg_type = Parser(buf);
      switch (msg_type) {
        case newscl::dispatcher_msg_type::kGetBestCN: {
          printf("kGetBestCN\n");
          client_type = newscl::dispatcher::client_type::kApp;
          newscl::HandlerGetBestCN(sock, buf);
          break;
        }
        case newscl::dispatcher_msg_type::kUpdateScore: {
          printf("kUpdateScore\n");
          newscl::HandlerUpdateScore(sock, buf);
          
          break;
        }
        case newscl::dispatcher_msg_type::kGetBindingCN: {
          printf("kGetBindingCN\n");
          client_type = newscl::dispatcher::client_type::kApp;
          newscl::HandlerGetBindingCN(sock, buf);
          break;
        }
        case newscl::dispatcher_msg_type::kCNReg: {
          printf("kCNReg\n");
          client_type = newscl::dispatcher::client_type::kCN;
          cn_id = newscl::HandlerCNReg(sock, buf);
          break;
        }
        case newscl::dispatcher_msg_type::kLoadInfo: {
          printf("kLoadInfo\n");
          newscl::HandlerLoadInfo(sock, buf);
          break;
        }
        
        default: {
          assert(false);
        }
      }
      total_recv_size = 0;
      printf("\n");
    }
  }
  if (recv_size == 0) {
    printf("connection closed\n");
    if (client_type == newscl::dispatcher::client_type::kCN) {
      newscl::CNList::Instance()->DelCN(cn_id);
    }
  } else {
    perror("recv error");
  }
  return nullptr;
}


int main() {
  newscl::SocketIOServer server(newscl::kDispatcherPort);
  newscl::thread_per_socket = Run;
  //newscl::thread_per_socket = ;

  server.BindAndListen(100);
  server.MainLoop();
}
