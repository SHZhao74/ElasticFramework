#include <msgpack/rpc/client.h>
#include <iostream>

int main(void)
{
    msgpack::rpc::client c("127.0.0.1", 9090);
    int result = c.call("add", 1, 2).get<int>();
    std::cout << result << std::endl;
}
