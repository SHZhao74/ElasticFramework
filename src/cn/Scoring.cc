#include "Scoring.h"

#include <pthread.h>
#include "math.h"
using std::unique_ptr;

unique_ptr<newscl::Scoring>& newscl::Scoring::Instance() {
  static bool is_init = true;
  static std::unique_ptr<Scoring> dispatcher = nullptr;
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  if (is_init) {
    pthread_mutex_lock(&mutex);
    {
      dispatcher =
        std::move(std::unique_ptr<Scoring> {new Scoring});
    }
    pthread_mutex_unlock(&mutex);
    is_init = false;
  }
  return dispatcher;
}

double findProb(const double time, const double delta, const double deadline)
{
	double prob;
	double howManyDelta = deadline / delta;
	if (howManyDelta > 3 && deadline > time)
	{
		prob = 1.0;
	}
	else if (howManyDelta > 3 && deadline < time){
		prob = 0.0;
	}
	else{
		prob = 1/sqrt(2*M_PI) * exp(-howManyDelta*howManyDelta/2);//標準常態機率公式
	}
	return prob;
}
newscl::CNInfo::HASH_KEY_T	newscl::Scoring::chooseBestCN(){

	CNInfo::HASH_KEY_T idx;
	auto &pCNList = CNList::Instance();
	double min=999.0;
	pCNList->Lock();
	{
	    /*
	    ++num;
	    auto iter = pCNList->CNs_.begin();
	    for (int i = 0; i < num % pCNList->CNs_.size(); ++i) {
	      ++iter;
	    }
	    idx = iter->first;
	    */
	    
	    for (auto &&iter = pCNList->cbegin(); iter != pCNList->cend(); ++iter) {
	      const auto &cn = iter->second;
	      const auto &cn_idx = iter->first;
	      
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
	      }
	    }
    
	}
	pCNList->Unlock();
	return idx;
}