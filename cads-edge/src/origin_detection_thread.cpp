
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



void window_processing_thread(double x_resolution, double y_resolution, int width_n, BlockingReaderWriterQueue<msg> &profile_fifo, BlockingReaderWriterQueue<msg> &next_fifo)
  {

    auto fiducial = make_fiducial(x_resolution, y_resolution);
    window profile_buffer;

    auto fdepth = global_config["fiducial_depth"].get<double>();
    double lowest_correlation = std::numeric_limits<double>::max();
    const double belt_crosscorr_threshold = global_config["belt_cross_correlation_threshold"].get<double>();

    cads::msg m;

    // Fill buffer
    for (int j = 0; j < fiducial.rows; j++)
    {
    hacky:
      profile_fifo.wait_dequeue(m);

      if (std::get<0>(m) == msgid::barrel_rotation_cnt)
        goto hacky;

      if (std::get<0>(m) != msgid::scan)
        break;
      auto p = get<cads::profile>(get<1>(m));

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
    auto start = std::chrono::high_resolution_clock::now();
    int64_t cnt = 0;
    auto buffer_size_warning = buffer_warning_increment;

    int64_t found_origin_sequence_cnt = 0;
    long barrel_rotation_cnt = 0;
    long barrel_rotation_offset = 0;

    while (true)
    {
      ++cnt;
      y_type y = profile_buffer.front().y;
      if (y >= trigger_length)
      {
        const auto cv_threshhold = left_edge_avg_height(belt, fiducial) - fdepth;
        auto correlation = search_for_fiducial(belt, fiducial, m1, out, cv_threshhold);

        lowest_correlation = std::min(lowest_correlation, correlation);

        if (correlation < belt_crosscorr_threshold)
        {
          spdlog::get("cads")->info("Correlation : {} at y : {} with barrel rotation count : {} Estimated Belt Length: {}", correlation, y, barrel_rotation_cnt - barrel_rotation_offset, 2.6138 * double(barrel_rotation_cnt - barrel_rotation_offset));
          found_origin_sequence_cnt++;
          trigger_length = y_max_length * 0.95;

          // fiducial_as_image(belt);

          y_offset += y;
          barrel_rotation_offset = barrel_rotation_cnt;

          // Reset buffer y values to origin
          for (auto off = y; auto &p : profile_buffer)
          {
            p.y -= off;
          }

          lowest_correlation = std::numeric_limits<double>::max();
        }

        if (y > y_max_length)
        {
          spdlog::get("cads")->info("Origin not found before Max samples. Lowest Correlation : {}", lowest_correlation);

          found_origin_sequence_cnt = 0;
          y_offset += y;

          // Reset buffer y values to origin
          for (auto off = y; auto &p : profile_buffer)
          {
            p.y -= off;
          }

          lowest_correlation = std::numeric_limits<double>::max();
        }
      }

      if (found_origin_sequence_cnt > 0 /*FIXME change to 1 for release */)
      {
        next_fifo.enqueue({msgid::scan, profile_buffer.front()});
      }

    wait:
      profile_fifo.wait_dequeue(m);

      if (std::get<0>(m) == msgid::barrel_rotation_cnt)
      {
        barrel_rotation_cnt = get<long>(get<1>(m));
        goto wait;
      }

      if (std::get<0>(m) != msgid::scan)
      {
        break;
      }

      auto p = get<cads::profile>(get<1>(m));

      shift_Mat(belt);
      prepend_Mat(belt, p.z);

      profile_buffer.pop_front();
      profile_buffer.push_back({p.y - y_offset, p.x_off, p.z});

      if (profile_fifo.size_approx() > buffer_size_warning)
      {
        spdlog::get("cads")->warn("Cads Origin Detection showing signs of not being able to keep up with data source. Size {}", buffer_size_warning);
        buffer_size_warning += buffer_warning_increment;
      }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    spdlog::get("cads")->info("ORIGIN DETECTION - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, cnt / duration);

    next_fifo.enqueue({msgid::finished, 0});
    spdlog::get("cads")->info("window_processing_thread");
  }
}