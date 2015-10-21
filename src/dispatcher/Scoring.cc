#include "Scoring.h"
#include "kernel.h"
#include "HistoryFileIO.h"
#include "newscl.h"

#include <pthread.h>
#include <math.h>

using std::unique_ptr;
using namespace newscl;

unique_ptr<elastic::Scoring>& elastic::Scoring::Instance() {
  static bool is_init = true;
  static std::unique_ptr<elastic::Scoring> dispatcher = nullptr;
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  if (is_init) {
    pthread_mutex_lock(&mutex);
    {
      dispatcher =
        std::move(std::unique_ptr<elastic::Scoring> {new elastic::Scoring});
    }
    pthread_mutex_unlock(&mutex);
    is_init = false;
  }
  return dispatcher;
}

double findProb(const double time, const double delta, const double deadline)
{
	double prob =0.5;
/*	double howManyDelta = deadline / delta;
	if (howManyDelta > 3 && deadline > time)
	{
		prob = 1.0;
	}
	else if (howManyDelta > 3 && deadline < time){
		prob = 0.0;
	}
	else{
		prob = 1/sqrt(2*M_PI) * exp(-howManyDelta*howManyDelta/2);//標準常態機率公式
	}*/
	return prob;
}

CNInfo::HASH_KEY_T*	elastic::Scoring::chooseBestCN(const CNInfo::HASH_KEY_T client_id, elastic::KernelInfo kinfo){ 
//用一個陣列或link list排序每個CN的分數 
//return an array of CNInfo::HASH_KEY_T , array length is levelCnt
	
	CNInfo::HASH_KEY_T* idx= new CNInfo::HASH_KEY_T [kinfo.levelCnt()];
	auto &pCNList = CNList::Instance();
	pCNList->Lock();
	
	    /*
	    ++num;
	    auto iter = pCNList->CNs_.begin();
	    for (int i = 0; i < num % pCNList->CNs_.size(); ++i) {
	      ++iter;
	    }
	    idx = iter->first;
	    */
	    int i=0;
	    for (auto &&iter = pCNList->cbegin(); iter != pCNList->cend(); ++iter,++i) {
	      const auto &cn = iter->second;
	      const auto &cn_idx = iter->first;
	      /*float exeTime, exeStdev, netTime, netStdev;
	      HistoryFileIO hfIO("exe_" + (string)cn_idx, "network_" +　(string)cn_idx)；
	      exeTime  = hfIO.getExeTime(appName, );
	      exeStdev = hfIO.getExeStdev(appName, );
	      netTime  = hfIO.getNetworkTime(client_id, );
	      netStdev = hfIO.getNetworkTime(client_id, );

	      double time_sum = iter->second.spend_time() + iter->second.wireless_time();
	      double delta_sum = sqrt(pow(iter->second.spend_delta(),2) + 
	      					 pow(iter->second.wireless_delta(),2));
	      double deadline;
	      double failProb = findProb(time_sum, delta_sum, deadline);
	      double expectLostTIme = deadline * failProb;
	      double WCratio = iter->second.wireless_time() / iter->second.spend_time()

	      double distance = sqrt(pow(expectLostTIme, 2)+ pow(WCratio, 2));
	      if (distance < min) {
	        min = distance;

	        idx = cn_idx;
	      }*/
	    
    
	        
	        idx[i] = cn_idx;
	    }
	    
    
	
	pCNList->Unlock();
	printf("Scoring::chooseBestCN end\n");

	return idx;
}
