
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow="
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

#include <spdlog/spdlog.h>

#pragma GCC diagnostic pop

#include <origin_detection_thread.h>
#include <fiducial.h>
#include <constants.h>
#include <coro.hpp>
#include <intermessage.h>
#include <filters.h>
#include <opencv2/core.hpp>

using namespace moodycamel;

namespace cads
{
  using window = std::deque<profile>;

  double left_edge_avg_height(const cv::Mat& belt, const cv::Mat& fiducial) {

    cv::Mat mout;
    cv::multiply(belt.rowRange(0,fiducial.rows),fiducial,mout);
    double div = cv::countNonZero(mout);
    auto avg_val = cv::sum(mout)[0] / div;
    return avg_val;

  }

  cv::Mat window_to_mat_fixed(const window& win, int width) {

    if(win.size() < 1) return cv::Mat(0,0,CV_32F);

    cv::Mat mat(win.size(),width,CV_32F,cv::Scalar::all(0.0f));
    
    int i = 0;
    for(auto p : win) {

      auto m = mat.ptr<float>(i++);

      int j = 0;
      
      for(auto z : p.z) {
        m[j++] = (float)z;
      }
    }

    return mat;
  }

  cv::Mat window_to_mat_fixed_transposed(const window& win, int width) {

    cv::Mat mat;

    cv::transpose(window_to_mat_fixed(win,width),mat);

    return mat;
  }


  void shift_Mat_rows(cv::Mat &m)
  {
    for (int i = m.rows - 1; i > 0; --i)
    {
      memcpy(m.ptr<float>(i), m.ptr<float>(i - 1), (size_t)m.cols * sizeof(float));
    }
  }

  void shift_Mat_cols(cv::Mat &m)
  {
    for(auto i = 0; i < m.rows; i++) {
      std::rotate(m.ptr<float>(i),m.ptr<float>(i)+1, m.ptr<float>(i) + m.cols);
    }
  }
  
  void prepend_Mat_cols(cv::Mat &m, const z_type &zs)
  {
    auto j = 0;
    for(auto z : zs) {
      *m.ptr<float>(j++) = z;
    }
  }
  
  void prepend_Mat_rows(cv::Mat &m, const z_type &z)
  {
    if constexpr (std::is_same_v<z_element, float>)
    {
      memcpy(m.ptr<float>(0), z.data(), (size_t)m.cols * sizeof(float));
    }
    else
    {
      auto m_i = m.ptr<float>(0);

      for (int i = 0; i < std::min(m.cols, (int)z.size()); ++i)
      {
        m_i[i] = z[i];
      }

      if (z.size() < m.cols)
      {
        for (int i = z.size(); i < m.cols; ++i)
        {
          m_i[i] = NAN;
        }
      }
    }
  }

  coro<std::tuple<profile,double,bool>,profile,1> origin_detection_coro(double x_resolution, double y_resolution, int width_n)
  {
    cv::Mat fiducial;
    cv::transpose(make_fiducial(x_resolution, y_resolution),fiducial);
    mat_as_image(fiducial,"fid");
    
    window profile_buffer;

    auto fdepth = fiducial_config.fiducial_depth;
    double lowest_correlation = std::numeric_limits<double>::max();
    double belt_crosscorr_threshold = config_origin_detection.cross_correlation_threshold;
    auto dump_match = config_origin_detection.dump_match;

    profile p;
    bool terminate = false;

    // Fill buffer
    std::tie(p,terminate) = co_yield {null_profile,0.0,false};
    for (int j = 0; j < fiducial.cols; j++)
    {
      std::tie(p,terminate) = co_yield {p,p.y,false};
      profile_buffer.push_back(p);
    }

    cv::Mat belt = window_to_mat_fixed_transposed(profile_buffer, width_n);
    if (!belt.isContinuous())
    {
      std::throw_with_nested(std::runtime_error("window_processing:OpenCV matrix must be continuous for row shifting using memcpy"));
    }

    //mat_as_image(belt,"belt");

    cv::Mat m1(fiducial.rows*2, belt.cols, CV_32F, cv::Scalar::all(0.0f));
    cv::Mat out(m1.rows - fiducial.rows + 1, m1.cols - fiducial.cols + 1, CV_32F, cv::Scalar::all(0.0f));

    auto y_max_length = global_belt_parameters.Length * 1.02; 
    auto trigger_length = std::numeric_limits<y_type>::lowest();
    y_type y_offset = 0;
    double y_lowest_correlation = 0;
    double cv_threshold_correleation = 0;
    cv::Mat matrix_correlation = belt.clone();
    long sequence_cnt = 0;
    auto valid = false;
    long cnt = 0;

    auto start = std::chrono::high_resolution_clock::now();

    while (true)
    {
      y_type y = profile_buffer.front().y;
 
      if((cnt++ % 10000) == 0 && sequence_cnt > 0 ) {
        publish_CadsToOrigin(y); //TODO
      }


      if (y >= trigger_length)
      {
        const auto cv_threshhold = left_edge_avg_height(belt, fiducial) - fdepth;
        auto [correlation,loc] = search_for_fiducial(belt, fiducial, m1, out, cv_threshhold);

        if(correlation <= lowest_correlation) {
          lowest_correlation = correlation;
          y_lowest_correlation = y;
          cv_threshold_correleation = cv_threshhold;
          matrix_correlation = belt.clone();
        }

        if((cnt % 10000) == 0) {
          //spdlog::get("cads")->info("SampleCorrelation: {}, Threshold: {}, Lowest: {}", correlation, cv_threshhold, lowest_correlation);
        }

        if (correlation < belt_crosscorr_threshold && (!(sequence_cnt > 0) || between(config_origin_detection.belt_length,y)))
        {
          ++sequence_cnt;
          spdlog::get("cads")->info("Correlation : {} at y : {} with threshold: {} and count : {}", correlation, y, cv_threshhold, cnt);
          auto now = std::chrono::high_resolution_clock::now();
          auto period = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
          start = now;
          cnt = 0;

          if(sequence_cnt > 1) {
            publish_RotationPeriod(period); //TODO
            trigger_length = y * 0.95; //TODO
          }
          
          if(sequence_cnt == 1) {
            trigger_length = y_max_length * 0.95; //TODO
          }

          if(dump_match) {
            mat_as_image(belt,cv_threshhold);
            mat_as_image(belt);
          }

          y_offset += y;

          // Reset buffer y values to origin
          for (auto off = y; auto &pro : profile_buffer)
          {
            pro.y -= off;
          }

          lowest_correlation = std::numeric_limits<double>::max();
          valid = true;
        }

        if (y > y_max_length)
        {
          sequence_cnt = 0;
          spdlog::get("cads")->info("Origin not found before Max samples. Lowest Correlation: {} at Y: {} with threshold: {}", lowest_correlation,y_lowest_correlation,cv_threshold_correleation);
          if(dump_match) {
            mat_as_image(matrix_correlation,"best-failed-match");
          }
          

          y_offset += y;

          // Reset buffer y values to origin
          for (auto off = y; auto &pro : profile_buffer)
          {
            pro.y -= off;
          }

          valid = false;
          y_max_length = global_belt_parameters.Length * 1.02; 
          trigger_length = std::numeric_limits<y_type>::lowest();
          lowest_correlation = std::numeric_limits<double>::max();
        }
      }

      std::tie(p,terminate) = co_yield {profile_buffer.front(),y,valid};

      if(terminate) break;

      shift_Mat_cols(belt);
      prepend_Mat_cols(belt, p.z);

      profile_buffer.pop_front();
      profile_buffer.push_back({p.y - y_offset, p.x_off, p.z});

    }
  }

  void window_processing_thread(double x_resolution, double y_resolution, int width_n, BlockingReaderWriterQueue<msg> &profile_fifo, BlockingReaderWriterQueue<msg> &next_fifo)
  {

    cads::msg m;

    auto start = std::chrono::high_resolution_clock::now();
    int64_t cnt = 0;
    auto buffer_size_warning = buffer_warning_increment;


    auto origin_detection = origin_detection_coro(x_resolution,y_resolution,width_n);

    long origin_sequence_cnt = 0;

    for (auto loop = true;loop;++cnt)
    {
      
      profile_fifo.wait_dequeue(m);

      switch(std::get<0>(m)) {

        case msgid::scan: {
          auto p = get<profile>(get<1>(m));
          auto [coro_end,result] = origin_detection.resume(p);
          
          if(!coro_end) {
            auto [op,estimated_belt_length,valid] = result;

            if(valid) {

              if(op.y == 0) {
                
                if(origin_sequence_cnt == 0) {
                  next_fifo.enqueue({msgid::begin_sequence, origin_sequence_cnt});
                }

                if(origin_sequence_cnt > 0) {
                  publish_CurrentLength(estimated_belt_length);
                  next_fifo.enqueue({msgid::complete_belt, estimated_belt_length});
                }
                
                origin_sequence_cnt++;
              }
              
              next_fifo.enqueue({msgid::scan, op});
            }else{
              next_fifo.enqueue({msgid::end_sequence, origin_sequence_cnt});
              origin_sequence_cnt = 0;
            }
            
          }else {
            loop = false;
            spdlog::get("cads")->error("Origin decetor stopped");
          }
          break;
        }
        case msgid::finished:
          next_fifo.enqueue(m);
          loop = false;
          break;
        default:
          next_fifo.enqueue(m);
      }

      if (profile_fifo.size_approx() > buffer_size_warning)
      {
        spdlog::get("cads")->warn("Cads Origin Detection showing signs of not being able to keep up with data source. Size {}", buffer_size_warning);
        buffer_size_warning += buffer_warning_increment;
      }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto rate = duration != 0 ? (double)cnt / duration : 0;
    spdlog::get("cads")->info("ORIGIN DETECTION - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, rate);
  }


  coro<std::tuple<profile,double,bool>,profile,1> maxlength_coro()
  {

    profile p;
    bool terminate = false;


    auto y_max_length = global_belt_parameters.Length;

    std::tie(p,terminate) = co_yield {p,0.0,false}; 

    y_type y_offset = p.y;
    while (true)
    {
      
      if(terminate) break;

      y_type y = p.y;

      if (y >= y_max_length + y_offset)
      {
        y_offset = y;
      }
      p = {p.y - y_offset, p.x_off, p.z};

      std::tie(p,terminate) = co_yield {p,y,true};  
    }
  }

  coro<std::tuple<profile,bool>,profile,1> identity_coro()
  {

    profile p;
    bool terminate = false;

    std::tie(p,terminate) = co_yield {p,false};  

    while (true)
    {
      
      if(terminate) break;

      std::tie(p,terminate) = co_yield {p,true};  
    }
  }


  void bypass_fiducial_detection_thread(BlockingReaderWriterQueue<msg> &profile_fifo, BlockingReaderWriterQueue<msg> &next_fifo)
  {

    cads::msg m;
    auto start = std::chrono::high_resolution_clock::now();
    int64_t cnt = 0;
    auto buffer_size_warning = buffer_warning_increment;

    auto origin_detection = maxlength_coro();

    long origin_sequence_cnt = 0;

    for (auto loop = true;loop;++cnt)
    {
      
      profile_fifo.wait_dequeue(m);

      switch(std::get<0>(m)) {

        case msgid::scan: {
          auto p = get<profile>(get<1>(m));
          auto [coro_end,result] = origin_detection.resume(p);
          
          if(!coro_end) {
            auto [op,estimated_belt_length,valid] = result;

            if(valid) {

              if(op.y == 0) {
                
                if(origin_sequence_cnt == 0) {
                  next_fifo.enqueue({msgid::begin_sequence, origin_sequence_cnt});
                }

                if(origin_sequence_cnt > 0) {
                  spdlog::get("cads")->info("Estimated Belt Length(m): {}", estimated_belt_length / 1000);
                  publish_CurrentLength(estimated_belt_length);
                  next_fifo.enqueue({msgid::complete_belt, estimated_belt_length});
                }
                
                origin_sequence_cnt++;
              }
              
              next_fifo.enqueue({msgid::scan, op});
            }else{
              next_fifo.enqueue({msgid::end_sequence, origin_sequence_cnt});
              origin_sequence_cnt = 0;
            }
            
          }else {
            loop = false;
            spdlog::get("cads")->error("Origin dectored stopped");
          }
          break;
        }
        case msgid::finished:
          next_fifo.enqueue(m);
          loop = false;
          break;
        default:
          next_fifo.enqueue(m);
      }

      if (profile_fifo.size_approx() > buffer_size_warning)
      {
        spdlog::get("cads")->warn("Cads Origin Detection showing signs of not being able to keep up with data source. Size {}", buffer_size_warning);
        buffer_size_warning += buffer_warning_increment;
      }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto rate = duration != 0 ? (double)cnt / duration : 0;
    spdlog::get("cads")->info("ORIGIN DETECTION - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, rate);

    next_fifo.enqueue({msgid::finished, 0});
    spdlog::get("cads")->info("window_processing_thread");
  }
}
