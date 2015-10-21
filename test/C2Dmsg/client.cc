#include <stdio.h>
#include <iostream>
#include "CL/cl.h"
#include "newscl.h"
#include "elastic.h"
using namespace std;
int main(){
int levelCnt = 2;
//_cl_platform_id* id = new _cl_platform_id[levelCnt];

//newscl::GetPlatform();
elastic::GetBestPlatforms("NTU", 500, levelCnt);

return 0;
}
