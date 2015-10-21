__kernel void filter(__global char *tbreak, __global int *times){
	*times = -1;
	if(*tbreak == 1){
		*times == -3;
	}
	int abc = *tbreak;
	for(int i = 0 ; i < 10; ++i){
		for(int j = 0; j < 1000000; ++j){
			if(abc == 9){
				*times = i;
				break;
			}
		}
		if(*tbreak == 1){
			if(*times != -3)
				*times = i;
			break;
		}
	}
}
