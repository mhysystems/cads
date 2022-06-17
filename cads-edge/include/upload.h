#ifndef CADS_UPLOAD
#define CADS_UPLOAD 1

#include <string>
#include <variant>
#include <cstdint>

#include <readerwriterqueue.h>
#include <profile.h>

namespace cads{

void http_post_thread(moodycamel::BlockingReaderWriterQueue<uint64_t> &upload_fifo);
int http_post_whole_belt();

}

#endif