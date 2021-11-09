#ifndef CADS_UPLOAD
#define CADS_UPLOAD 1

#include <queue>
#include <condition_variable>
#include <mutex>
#include <string>
#include <variant>


#include "cads.h"

namespace cads{

void http_post_thread(std::queue<std::variant<uint64_t,std::string>> &q, std::mutex &m, std::condition_variable &sig);

}

#endif