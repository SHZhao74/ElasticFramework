#include "IO.h"
#include <vector>
#include <sys/time.h>
//#define MEASURE_TIME
#ifdef MEASURE_TIME
static unsigned long long timetime = 0;
#endif 
int newscl::SockRecv(int sock, char * const buf, const unsigned int bufLen) {
#ifdef MEASURE_TIME
  static struct timeval tv, tv2;
  static unsigned long long start_utime, end_utime;
  gettimeofday(&tv,NULL);
  start_utime = tv.tv_sec * 1000000 + tv.tv_usec;
#endif
	unsigned int total_recv_size = 0;
	int recv_size;
	while ((recv_size = ::recv(sock,
                             buf + total_recv_size,
                             bufLen - total_recv_size, 0)) > 0) {
		total_recv_size += static_cast<unsigned int>(recv_size);
		if (bufLen == total_recv_size) {
			break;
		} else if (bufLen < total_recv_size) {
			perror("recv too much data");
			exit(1);
		}
	}
	if (recv_size <= 0) {
		perror("Error in recv\n");
		exit(1);
	}
#ifdef MEASURE_TIME
  gettimeofday(&tv2,NULL);
  end_utime = tv2.tv_sec * 1000000 + tv2.tv_usec;
  timetime += end_utime - start_utime;
  fprintf(stderr, "timetime:%ld\n", timetime);
#endif
	return recv_size > 0 ? static_cast<int>(total_recv_size) : recv_size;
}

int newscl::SockSend(int sock, char const * const buf, const unsigned int bufLen, bool is_buffer) {
#ifdef MEASURE_TIME
  static struct timeval tv, tv2;
  static unsigned long long start_utime, end_utime;
  gettimeofday(&tv,NULL);
  start_utime = tv.tv_sec * 1000000 + tv.tv_usec;
#endif
  std::vector<char> send_buf;
  std::copy(buf, buf + bufLen, std::back_inserter(send_buf));

  int send_size;
  unsigned int total_send_size = 0;
//  if (!is_buffer) {
    while ((send_size = ::send(sock, send_buf.data() + total_send_size, send_buf.size() - total_send_size, 0)) > 0) {
      total_send_size += static_cast<unsigned int>(send_size);
      if (send_buf.size() == total_send_size) {
        break;
      } else if(send_buf.size() < total_send_size) {
        perror("send too much data");
        exit(1);
      }
    }
    if (send_size <= 0) {
      perror("Error in send\n");
      exit(1);
    }
    send_buf.clear();
//  }
  
#ifdef MEASURE_TIME
  gettimeofday(&tv2,NULL);
  end_utime = tv2.tv_sec * 1000000 + tv2.tv_usec;
  timetime += end_utime - start_utime;
  fprintf(stderr, "timetime:%ld\n", timetime);
#endif
	return send_size > 0 ? static_cast<int>(total_send_size) : send_size;
}

