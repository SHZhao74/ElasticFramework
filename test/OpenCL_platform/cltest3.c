__kernel void filter(__global char *data){
  ++data[get_global_id(0)];
}
