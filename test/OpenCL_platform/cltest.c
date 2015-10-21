__kernel void filter(__global char *data){
	for(int i = 0;i < 1024; ++i){
		++data[i];
	}
}
