
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow="
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

#include <spdlog/spdlog.h>

#pragma GCC diagnostic pop

#include <origin_detection_thread.h>
#include <window.hpp>
#include <fiducial.h>
#include <constants.h>
#include <coro.hpp>
#include <intermessage.h>

using namespace moodycamel;

namespace cads
{

  void shift_Mat(cv::Mat &m)
  {
    for (int i = m.rows - 1; i > 0; --i)
    {
      memcpy(m.ptr<float>(i), m.ptr<float>(i - 1), (size_t)m.cols * sizeof(float));
    }
  }

  void prepend_Mat(cv::Mat &m, const z_type &z)
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

  coro<std::tuple<profile,bool>,profile,1> origin_detection_coro(double x_resolution, double y_resolution, int width_n)
  {
     auto fiducial = make_fiducial(x_resolution, y_resolution);
    window profile_buffer;

    auto fdepth = global_config["fiducial_depth"].get<double>();
    double lowest_correlation = std::numeric_limits<double>::max();
    const double belt_crosscorr_threshold = global_config["belt_cross_correlation_threshold"].get<double>();

    cads::msg m;
    profile p;
    bool terminate = false;

    // Fill buffer
    std::tie(p,terminate) = co_yield {null_profile,false};
    for (int j = 0; j < fiducial.rows; j++)
    {
      std::tie(p,terminate) = co_yield {p,false};
      profile_buffer.push_back(p);
    }

    cv::Mat belt = window_to_mat_fixed(profile_buffer, width_n);
    if (!belt.isContinuous())
    {
      std::throw_with_nested(std::runtime_error("window_processing:OpenCV matrix must be continuous for row shifting using memcpy"));
    }

    cv::Mat m1(fiducial.rows, int(fiducial.cols * 1.5), CV_32F, cv::Scalar::all(0.0f));
    cv::Mat out(m1.rows - fiducial.rows + 1, m1.cols - fiducial.cols + 1, CV_32F, cv::Scalar::all(0.0f));

    auto y_max_length = global_config["y_max_length"].get<double>();
    auto trigger_length = std::numeric_limits<y_type>::lowest();
    y_type y_offset = 0;
    double y_lowest_correlation = 0;
    double cv_threshold_correleation = 0;
    cv::Mat matrix_correlation = belt.clone();
    long sequence_cnt = 0;
    auto valid = false;
    long cnt = 1;

    auto start = std::chrono::high_resolution_clock::now();

    while (true)
    {
      y_type y = profile_buffer.front().y;
      
      if(sequence_cnt > 0 && (cnt++ % 10000) == 0) {
        publish_CadsToOrigin(y);
      }


      if (y >= trigger_length)
      {
        const auto cv_threshhold = left_edge_avg_height(belt, fiducial) - fdepth;
        auto correlation = search_for_fiducial(belt, fiducial, m1, out, cv_threshhold);

        if(correlation <= lowest_correlation) {
          lowest_correlation = correlation;
          y_lowest_correlation = y;
          cv_threshold_correleation = cv_threshhold;
          matrix_correlation = belt.clone();
        }

        if (correlation < belt_crosscorr_threshold)
        {
          ++sequence_cnt;
          spdlog::get("cads")->info("Correlation : {} at y : {} with threshold: {}", correlation, y, cv_threshhold);
          auto now = std::chrono::high_resolution_clock::now();
          auto period = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
          start = now;

          if(sequence_cnt > 1) {
            publish_RotationPeriod(period);
            y_max_length =  y * 1.05;
            trigger_length = y * 0.95;
          }
          
          if(sequence_cnt == 1) {
            trigger_length = y_max_length * 0.90;
          }

          
           // fiducial_as_image(belt);

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
          //fiducial_as_image(matrix_correlation,"best-failed-match");

          y_offset += y;

          // Reset buffer y values to origin
          for (auto off = y; auto &pro : profile_buffer)
          {
            pro.y -= off;
          }

          valid = true;
          y_max_length = global_config["y_max_length"].get<double>();
          trigger_length = std::numeric_limits<y_type>::lowest();
          lowest_correlation = std::numeric_limits<double>::max();
        }
      }

      std::tie(p,terminate) = co_yield {profile_buffer.front(),valid};

      if(terminate) break;

      shift_Mat(belt);
      prepend_Mat(belt, p.z);

      profile_buffer.pop_front();
      profile_buffer.push_back({p.y - y_offset, p.x_off, p.z});

    }
  }

  void window_processing_thread(double x_resolution, double y_resolution, int width_n, BlockingReaderWriterQueue<msg> &profile_fifo, BlockingReaderWriterQueue<msg> &next_fifo)
  {

    cads::msg m;
    auto pully_circumfrence = global_conveyor_parameters.PulleyCircumference;
    auto start = std::chrono::high_resolution_clock::now();
    int64_t cnt = 0;
    auto buffer_size_warning = buffer_warning_increment;

    long barrel_rotation_cnt = 0;
    long barrel_rotation_offset = 0;

    auto origin_detection = origin_detection_coro(x_resolution,y_resolution,width_n);

    long origin_sequence_cnt = 0;

    for (auto loop = true;loop;++cnt)
    {
      
      profile_fifo.wait_dequeue(m);

      switch(std::get<0>(m)) {
        case msgid::barrel_rotation_cnt:
        barrel_rotation_cnt = get<long>(get<1>(m));
        break;

        case msgid::scan: {
          auto p = get<profile>(get<1>(m));
          auto [coro_end,result] = origin_detection.resume(p);
          
          if(!coro_end) {
            auto [op,valid] = result;

            if(valid) {

              if(op.y == 0) {
                
                if(origin_sequence_cnt == 0) {
                  next_fifo.enqueue({msgid::begin_sequence, origin_sequence_cnt});
                }

                if(origin_sequence_cnt > 0) {
                  auto estimated_belt_length = pully_circumfrence * (double(barrel_rotation_cnt - barrel_rotation_offset) / 2.0);
                  spdlog::get("cads")->info("Barrel rotation count : {} Estimated Belt Length: {}",barrel_rotation_cnt - barrel_rotation_offset, estimated_belt_length / 1000);
                  publish_CurrentLength(estimated_belt_length);
                  next_fifo.enqueue({msgid::complete_belt, estimated_belt_length});
                }
                
                barrel_rotation_offset = barrel_rotation_cnt;
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
        default:
          loop = false;
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

  void bypass_fiducial_detection_thread(BlockingReaderWriterQueue<msg> &profile_fifo, BlockingReaderWriterQueue<msg> &next_fifo)
  {

    cads::msg m;
    auto pully_circumfrence = global_conveyor_parameters.PulleyCircumference;
    auto start = std::chrono::high_resolution_clock::now();
    int64_t cnt = 0;
    auto buffer_size_warning = buffer_warning_increment;

    long barrel_rotation_cnt = 0;
    long barrel_rotation_offset = 0;

    long origin_sequence_cnt = 0;

    for (auto loop = true;loop;++cnt)
    {
      
      profile_fifo.wait_dequeue(m);

      switch(std::get<0>(m)) {
        case msgid::barrel_rotation_cnt:
        barrel_rotation_cnt = get<long>(get<1>(m));
        break;

        case msgid::scan: {
          auto op = get<profile>(get<1>(m));
          
          if(op.y == 0) {
            
            if(origin_sequence_cnt == 0) {
              next_fifo.enqueue({msgid::begin_sequence, origin_sequence_cnt});
            }

            if(origin_sequence_cnt > 0) {
              auto estimated_belt_length = pully_circumfrence * (double(barrel_rotation_cnt - barrel_rotation_offset) / 2.0);
              spdlog::get("cads")->info("Barrel rotation count : {} Estimated Belt Length: {}",barrel_rotation_cnt - barrel_rotation_offset, estimated_belt_length / 1000);
              publish_CurrentLength(estimated_belt_length);
              next_fifo.enqueue({msgid::complete_belt, estimated_belt_length});
            }
            
            barrel_rotation_offset = barrel_rotation_cnt;
            origin_sequence_cnt++;
          }
          
          next_fifo.enqueue({msgid::scan, op});
        
          break;
        }

        default:
          loop = false;
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
    spdlog::get("cads")->info("bypass_fiducial_detection_thread");
  }
}