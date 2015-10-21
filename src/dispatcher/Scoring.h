#ifndef SCORING_H
#define SCORING_H
#include <unordered_map>
#include <string>
#include <list>
#include <memory>
#include <cassert>

#include "boost/utility.hpp"

#include "./cn.h"
#include "newscl.h"
#include "kernel.h"
namespace elastic{
	class Scoring{
		pthread_mutex_t sched_proc_lock_;
		private:
			//list<double*> score;
		public:
			static std::unique_ptr<Scoring>& Instance();
			newscl::CNInfo::HASH_KEY_T*	chooseBestCN(const newscl::CNInfo::HASH_KEY_T, elastic::KernelInfo);
			inline Scoring():sched_proc_lock_(PTHREAD_MUTEX_INITIALIZER){}
	}; //class
} //namespace
#endif
