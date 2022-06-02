#ifndef CADS_UPLOAD
#define CADS_UPLOAD 1

#include <string>
#include <variant>
#include <cstdint>

#include <readerwriterqueue.h>
#include <profile.h>

namespace cads{

void http_post_thread(moodycamel::BlockingReaderWriterQueue<uint64_t> &upload_fifo);
void http_post_thread_bulk(moodycamel::BlockingReaderWriterQueue<y_type> &upload_fifo);
void http_post_profile_properties(std::string json, std::string ts = "");
void http_post_profile_properties(double y_resolution, double x_resolution, double z_resolution, double z_offset,std::string ts = "");

}

#endif