#include <msgpack/rpc/client.h>
#include <string>
#include <iostream>

using namespace std;
using namespace msgpack::rpc;

int main(void)
{
    // create an client instance
    client c("127.0.0.1", 9090);
    std::string text = ...;

    // send some heavy request and receives its future result.
    future f1 = c.call("analyze", text);
    // the server starts to process this request.

    // send other requests.
    future f2 = c.call("count", text);
    // the server starts to process this request on another CPU core.

    // receive actual result of request that will be finished first.
    int result2 = f2.get<int>();

    // you can do some calcuation while server processing first heavy request ...

    // then receive result of the heavy request.
    string result1 = f1.get<string>();

    std::cout << result1 << std::endl;
    std::cout << result2 << std::endl;
}
