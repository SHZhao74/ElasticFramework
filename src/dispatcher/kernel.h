#ifndef ELASTIC_KERNEL_H
#define ELASTIC_KERNEL_H
#include <string>
#include <stdio.h>

namespace elastic{
	class kSize{
		int argv_;
		int recv_size_;
		int send_size_;
		public:
			kSize(decltype(argv_) argv,
				  decltype(recv_size_) recv_size = 0,
				  decltype(send_size_) send_size = 0):
				argv_(argv),
				recv_size_(recv_size),
				send_size_(send_size) {}

			inline void set_argv(decltype(argv_) &val){argv_ = val;}
			inline void set_recv_size(decltype(recv_size_) &val){recv_size_ = val;}
			inline void set_send_size(decltype(send_size_) &val){send_size_ = val;}

			inline decltype(argv_) argv() const {return argv_;}
			inline decltype(recv_size_) recv_size() const {return recv_size_;}
			inline decltype(send_size_) send_size() const {return send_size_;}
	};
	class KernelInfo{
		std::string appName_;
		//int appName_;
		float deadline_;
		int levelCnt_;
		//kSize* ksize;
		public:
			explicit KernelInfo(decltype(appName_) appName, 
								decltype(deadline_) deadline, 
								decltype(levelCnt_) levelCnt):
				   				 deadline_(deadline), levelCnt_(levelCnt)
			{
				  //memcpy(appName_, appName, 20);
				  appName_ = appName;
			}

			KernelInfo(const KernelInfo &k)
			{
				  appName_ = k.appName_;
				  deadline_ = k.deadline_;
				  levelCnt_ = k.levelCnt_;
			}

			void set_appName (decltype(appName_) val){appName_ =val;}
			void set_deadline (decltype(deadline_) &val){deadline_ = val;}
			void set_levelCnt (decltype(levelCnt_) &val){levelCnt_ = val;}

			/*void set_kernel_info(decltype(appName_) &appName,
								 decltype(deadline_) &deadline,
								 decltype(levelCnt_) &levelCnt);*/

			//const char* appName() {return appName_.c_str();}
			decltype(appName_) appName() {return appName_.data();}
			decltype(deadline_) deadline() {return deadline_;}
			decltype(levelCnt_) levelCnt() {return levelCnt_;}

	};




}; // newscl
#endif
