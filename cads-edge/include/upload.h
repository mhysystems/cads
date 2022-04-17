#ifndef CADS_UPLOAD
#define CADS_UPLOAD 1

#include <string>
#include <variant>
#include <cstdint>

#include <readerwriterqueue.h>

namespace cads{

void http_post_thread(moodycamel::BlockingReaderWriterQueue<std::variant<uint64_t,std::string>> &upload_fifo);

}

#endif