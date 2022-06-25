#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>

#include <cads.h>
#include <regression.h>

#include <db.h>
#include <upload.h>
#include <constants.h>
#include <fiducial.h>
#include <window.hpp>
#include <readerwriterqueue.h>
#include <gocator_reader.h>
#include <sqlite_gocator_reader.h>

#include <z_data_generated.h>
#include <p_config_generated.h>

#include <exception>
#include <limits>
#include <string>
#include <vector>
#include <tuple>
#include <algorithm>
#include <thread>
#include <memory>
#include <ranges>
#include <chrono>
#include <sstream>
#include <future>
#include <fstream>

#include <spdlog/spdlog.h>

#include <filters.h>
#include <edge_detection.h>
#include <date/date.h>
#include <date/tz.h>
#include <fmt/core.h>
#include <fmt/chrono.h>

#include <coro.hpp>
#include <dynamic_processing.h>

using namespace std;
using namespace moodycamel;
using namespace std::chrono;
using CadsMat = cv::UMat; // cv::cuda::GpuMat

namespace cads
{

  void dynamic_processing_thread(BlockingReaderWriterQueue<msg> &profile_fifo, BlockingReaderWriterQueue<msg> &next_fifo, int width)
  {

    auto realtime_processing = lua_processing_coro(width);
    uint64_t cnt = 0;
    profile p;
    cads::msg m;
    int buffer_size_warning = 1024;
    int buffer_size_lower_bounds = buffer_size_warning - 1024;

    auto start = std::chrono::high_resolution_clock::now();

    while (true)
    {
      ++cnt;
      profile_fifo.wait_dequeue(m);

      if (get<0>(m) == msgid::scan)
      {
        p = get<profile>(get<1>(m));
      }
      else
      {
        break;
      }

      auto [err, rslt] = realtime_processing(m);
      if (rslt > 0)
      {
        spdlog::get("cads")->info("Belt damage found around y: {}", p.y);
      }

      if (profile_fifo.size_approx() > buffer_size_warning)
      {
        spdlog::get("cads")->warn("Cads Dynamic Processing showing signs of not being able to keep up with data source. Size {}", buffer_size_warning);
        buffer_size_warning += 1024;
        buffer_size_lower_bounds = buffer_size_warning - 1024;
      }

      if (profile_fifo.size_approx() < buffer_size_lower_bounds)
      {
        spdlog::get("cads")->warn("Cads Dynamic Processing showing signs of catching up with data source. Size {}", buffer_size_lower_bounds);
        buffer_size_warning -= 1024;
        buffer_size_lower_bounds = buffer_size_warning - 1024;
      }

      next_fifo.enqueue(m);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    next_fifo.enqueue({msgid::finished, 0});

    spdlog::get("cads")->info("DYNAMIC PROCESSING - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, (double)cnt / duration);
  }

  void save_send_thread(BlockingReaderWriterQueue<msg> &profile_fifo, int width)
  {
    using namespace date;
    using namespace chrono;

    hours trigger_hour;
    auto sts = global_config["daily_start_time"].get<std::string>();

    if (sts != "now"s)
    {
      std::stringstream st{sts};

      system_clock::duration run_in;
      st >> parse("%T", run_in);

      trigger_hour = chrono::floor<chrono::hours>(run_in);
    }
    else
    {
      auto now = current_zone()->to_local(system_clock::now());
      auto today = chrono::floor<chrono::days>(now);
      auto daily_time = duration_cast<seconds>(now - today);
      trigger_hour = chrono::floor<chrono::hours>(daily_time);
    }

    cads::msg m;

    enum state
    {
      waiting,
      processing,
      waitthread,
      finished
    };

    state s = waiting;
    auto store_profile = store_profile_coro();
    std::chrono::time_point<date::local_t, std::chrono::days> today;
    std::future<int> fut;
    profile p;
    int revid = 0, idx = 0, buffer_size_warning = 1024;
    int buffer_size_lower_bounds = buffer_size_warning - 1024;

    auto start = std::chrono::high_resolution_clock::now();
    uint64_t cnt = 0;

    while (true)
    {
      ++cnt;
      profile_fifo.wait_dequeue(m);

      if (get<0>(m) == msgid::scan)
      {
        p = get<profile>(get<1>(m));
      }
      else
      {
        break;
      }

      switch (s)
      {
      case waiting:
      {
        auto now = current_zone()->to_local(system_clock::now());
        today = chrono::floor<chrono::days>(now);
        auto daily_time = duration_cast<seconds>(now - today);
        auto current_hour = chrono::floor<chrono::hours>(daily_time);

        if (p.y == 0 && current_hour >= trigger_hour)
        {
          s = processing;
        }
        break;
      }
      case processing:
      {
        if (p.y == 0)
        {
          fut = std::async(http_post_whole_belt, revid++, idx);
          spdlog::get("cads")->info("Finished Processing");
          s = waitthread;
        }
        break;
      }
      case waitthread:
        if (fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready && p.y == 0.0)
        {
          fut.get();
          revid = 0;
          s = finished;
        }
        break;
      case finished:
      {
        auto now = current_zone()->to_local(system_clock::now());
        if (today != chrono::floor<chrono::days>(now))
        {
          spdlog::get("cads")->info("Switch to waiting");
          s = waiting;
        }
      }
      }

      if (p.y == 0.0)
        idx = 0;
      
      auto [invalid, dberr] = store_profile({revid, idx++, p});

      if (profile_fifo.size_approx() > buffer_size_warning)
      {
        spdlog::get("cads")->warn("Saving to DB showing signs of not being able to keep up with data source. Size {}", buffer_size_warning);
        buffer_size_warning += 1024;
        buffer_size_lower_bounds = buffer_size_warning - 1024;
      }

      if (profile_fifo.size_approx() < buffer_size_lower_bounds)
      {
        spdlog::get("cads")->warn("Saving to DB showing signs of catching up with data source. Size {}", buffer_size_lower_bounds);
        buffer_size_warning -= 1024;
        buffer_size_lower_bounds = buffer_size_warning - 1024;
      }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    spdlog::get("cads")->info("DB PROCESSING - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, (double)cnt / duration);

    spdlog::get("cads")->info("Final Upload");
    http_post_whole_belt(revid, idx); // For replay and not having a complete belt, so something is uploaded
    spdlog::get("cads")->info("Stopping save_send_thread");
  }

  tuple<z_element, z_element> barrel_offset(int cnt, double z_resolution, BlockingReaderWriterQueue<msg> &ps)
  {

    window win;
    const auto z_height_mm = global_config["z_height"].get<double>();
    cads::msg m;

    do
    {
      ps.wait_dequeue(m);
      auto m_id = get<0>(m);

      if (m_id == msgid::scan)
      {
        win.push_back(get<profile>(get<1>(m)));
      }
      else
      {
        break;
      }

    } while (--cnt > 0);

    if (cnt != 0)
    {
      std::throw_with_nested(std::runtime_error("barrel_offset:Less profiles in fifo than required by argument cnt"));
    }

    auto [z_min, z_max] = find_minmax_z(win);

    // Histogram, first is z value, second is count
    auto hist = histogram(win, z_min, z_max);

    const auto peak = get<0>(hist[0]);
    const auto thickness = z_height_mm; // / z_resolution;

    // Remove z values greater than the peak minus approx belt thickness.
    // Assumes the next peak will be the barrel values
    auto f = hist | views::filter([thickness, peak](tuple<double, z_element> a)
                                  { return peak - get<0>(a) > thickness; });
    vector<tuple<double, z_element>> barrel(f.begin(), f.end());

    return {get<0>(barrel[0]), peak};
  }

  int belt_width_n(int cnt, BlockingReaderWriterQueue<msg> &ps)
  {

    const int nan_num = global_config["left_edge_nan"].get<int>();
    const auto z_height_mm = global_config["z_height"].get<double>();
    const int spike_window_size = nan_num / 3;
    int width = 0;
    const double n = cnt - 1;
    cads::msg m;

    do
    {
      ps.wait_dequeue(m);
      auto m_id = get<0>(m);

      if (m_id == msgid::scan)
      {
        auto p = get<profile>(get<1>(m));
        spike_filter(p.z, spike_window_size);
        auto [left_edge_index, right_edge_index] = find_profile_edges_nans_outer(p.z, nan_num);
        width += right_edge_index - left_edge_index;
      }
      else
      {
        break;
      }

    } while (--cnt > 0);

    if (cnt != 0)
    {
      std::throw_with_nested(std::runtime_error("belt_width:Less profiles in fifo than required by argument cnt"));
    }

    width /= n;
    width += (width & 1);
    return width;
  }

  double belt_regression(int cnt, BlockingReaderWriterQueue<msg> &ps)
  {

    const int nan_num = global_config["left_edge_nan"].get<int>();
    const auto z_height_mm = global_config["z_height"].get<double>();
    const int spike_window_size = nan_num / 3;
    double gradient = 0.0, intercept = 0.0;
    const double n = cnt - 1;
    cads::msg m;

    do
    {
      ps.wait_dequeue(m);
      auto m_id = get<0>(m);

      if (m_id == msgid::scan)
      {
        auto p = get<profile>(get<1>(m));
        spike_filter(p.z, spike_window_size);
        auto [left_edge_index, right_edge_index] = find_profile_edges_nans_outer(p.z, nan_num);
        nan_filter(p.z);
        auto [i, g] = linear_regression(p.z, left_edge_index, right_edge_index);
        intercept += i;
        gradient += g;
      }
      else
      {
        break;
      }

    } while (--cnt > 0);

    if (cnt != 0)
    {
      std::throw_with_nested(std::runtime_error("belt_regression:Less profiles in fifo than required by argument cnt"));
    }

    return gradient / n;
  }

  unique_ptr<GocatorReaderBase> mk_gocator(BlockingReaderWriterQueue<msg> &gocatorFifo)
  {
    auto data_src = global_config["data_source"].get<std::string>();

    if (data_src == "gocator"s)
    {
      spdlog::get("cads")->debug("Using gocator as data source");
      return make_unique<GocatorReader>(gocatorFifo);
    }
    else
    {
      spdlog::get("cads")->debug("Using sqlite as data source");
      return make_unique<SqliteGocatorReader>(gocatorFifo);
    }
  }

  void store_profile_only()
  {

    auto db_name = global_config["db_name"].get<std::string>();
    create_db(db_name);

    BlockingReaderWriterQueue<msg> gocatorFifo;

    auto gocator = mk_gocator(gocatorFifo);

    gocator->Start();

    cads::msg m;

    gocatorFifo.wait_dequeue(m);
    auto m_id = get<0>(m);

    if (m_id != cads::msgid::resolutions)
    {
      std::throw_with_nested(std::runtime_error("First message must be resolutions"));
    }

    auto [y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution] = get<resolutions_t>(get<1>(m));
    store_profile_parameters(y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution);

    auto store_profile = store_profile_coro();

    auto y_max_length = global_config["y_max_length"].get<double>();
    double Y = 0.0;
    int idx = 0;

    do
    {

      gocatorFifo.wait_dequeue(m);
      auto m_id = get<0>(m);

      switch (m_id)
      {
      case cads::msgid::scan:
      {
        auto p = get<profile>(get<1>(m));
        Y = p.y;
        store_profile({0, idx++, p});
        break;
      }
      case cads::msgid::resolutions:
      {
        break;
      }
      }

    } while (get<0>(m) != msgid::finished && Y < 2 * y_max_length);

    store_profile({0, idx++, null_profile});
    
    gocator->Stop();

  }

  void shift_Mat(cv::Mat &m)
  {
    for (int i = m.rows - 1; i > 0; --i)
    {
      memcpy(m.ptr<float>(i), m.ptr<float>(i - 1), m.cols * sizeof(float));
    }
  }

  void prepend_Mat(cv::Mat &m, const z_type &z)
  {
    if constexpr (std::is_same_v<z_element, float>)
    {
      memcpy(m.ptr<float>(0), z.data(), m.cols * sizeof(float));
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

  void window_processing_thread(double x_resolution, double y_resolution, double z_resolution, int width_n, BlockingReaderWriterQueue<msg> &profile_fifo, BlockingReaderWriterQueue<msg> &next_fifo)
  {

    auto fiducial = make_fiducial(x_resolution, y_resolution);
    window profile_buffer;

    auto fdepth = global_config["fiducial_depth"].get<double>(); // / z_resolution;
    double lowest_correlation = std::numeric_limits<double>::max();
    const double belt_crosscorr_threshold = global_config["belt_cross_correlation_threshold"].get<double>();

    cads::msg m;

    // Fill buffer
    for (int j = 0; j < fiducial.rows; j++)
    {
      profile_fifo.wait_dequeue(m);

      if (std::get<0>(m) != msgid::scan)
        break;
      auto profile = get<cads::profile>(get<1>(m));

      profile_buffer.push_back(profile);
    }

    cv::Mat belt = window_to_mat_fixed(profile_buffer, width_n);
    if (!belt.isContinuous())
    {
      std::throw_with_nested(std::runtime_error("window_processing:OpenCV matrix must be continuous for row shifting using memcpy"));
    }

    cv::Mat m1(fiducial.rows, fiducial.cols * 1.5, CV_32F, cv::Scalar::all(0.0f));
    cv::Mat out(m1.rows - fiducial.rows + 1, m1.cols - fiducial.cols + 1, CV_32F, cv::Scalar::all(0.0f));

    auto y_max_length = global_config["y_max_length"].get<double>();
    auto trigger_length = std::numeric_limits<y_type>::lowest();
    y_type y_offset = 0;
    auto start = std::chrono::high_resolution_clock::now();
    uint64_t cnt = 0;
    int buffer_size_warning = 1024;
    int buffer_size_lower_bounds = buffer_size_warning - 1024;

    while (true)
    {
      ++cnt;
      y_type y = profile_buffer.front().y;
      if (y >= trigger_length)
      {
        const auto cv_threshhold = left_edge_avg_height(belt, fiducial) - fdepth;
        auto correlation = search_for_fiducial(belt, fiducial, m1, out, cv_threshhold);
        // correlation += 1.0;
        lowest_correlation = std::min(lowest_correlation, correlation);

        if (correlation < belt_crosscorr_threshold)
        {
          spdlog::get("cads")->info("Correlation : {} at y : {}", correlation, y);

          trigger_length = y_max_length * 0.95;

          fiducial_as_image(belt);

          y_offset += y;

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

          y_offset += y;

          // Reset buffer y values to origin
          for (auto off = y; auto &p : profile_buffer)
          {
            p.y -= off;
          }

          lowest_correlation = std::numeric_limits<double>::max();
        }
      }

      next_fifo.enqueue({msgid::scan, profile_buffer.front()});

      profile_fifo.wait_dequeue(m);

      if (std::get<0>(m) != msgid::scan)
      {
        break;
      }

      auto profile = get<cads::profile>(get<1>(m));

      shift_Mat(belt);
      prepend_Mat(belt, profile.z);

      profile_buffer.pop_front();
      profile_buffer.push_back({profile.y - y_offset, profile.x_off, profile.z});

      if (profile_fifo.size_approx() > buffer_size_warning)
      {
        spdlog::get("cads")->warn("Cads Origin Detection showing signs of not being able to keep up with data source. Size {}", buffer_size_warning);
        buffer_size_warning += 1024;
        buffer_size_lower_bounds = buffer_size_warning - 1024;
      }

      if (profile_fifo.size_approx() < buffer_size_lower_bounds)
      {
        spdlog::get("cads")->warn("Cads Origin Detection showing signs of catching up with data source. Size {}", buffer_size_lower_bounds);
        buffer_size_warning -= 1024;
        buffer_size_lower_bounds = buffer_size_warning - 1024;
      }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    spdlog::get("cads")->info("ORIGIN DETECTION - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, (double)cnt / duration);

    next_fifo.enqueue({msgid::finished, 0});
    spdlog::get("cads")->info("window_processing_thread");
  }

  std::tuple<double, double, double, z_element, int> preprocessing(BlockingReaderWriterQueue<msg> &gocatorFifo)
  {
    cads::msg m;

    gocatorFifo.wait_dequeue(m);
    auto m_id = get<0>(m);

    if (m_id != cads::msgid::resolutions)
    {
      std::throw_with_nested(std::runtime_error("preprocessing:First message must be resolutions"));
    }

    auto [y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution] = get<resolutions_t>(get<1>(m));
    spdlog::get("cads")->info("Gocator contants - y_res:{}, x_res:{}, z_res:{}, z_off:{}, encoder_res:{}", y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution);
    auto [bottom, top] = barrel_offset(1024, z_resolution, gocatorFifo);
    auto width_n = belt_width_n(1024, gocatorFifo);
    spdlog::get("cads")->info("Belt properties - botton:{}, top:{}, height(mm):{}, width:{}, width_n:{}", bottom, top, top - bottom, width_n * x_resolution, width_n);
    store_profile_parameters(y_resolution, x_resolution, z_resolution, -bottom * z_resolution, encoder_resolution);

    return {x_resolution, y_resolution, z_resolution, bottom, width_n};
  }

  void process()
  {
    create_db(global_config["db_name"].get<std::string>().c_str());

    BlockingReaderWriterQueue<msg> gocatorFifo(4096 * 1024);
    BlockingReaderWriterQueue<msg> winFifo(4096 * 1024);

    auto gocator = mk_gocator(gocatorFifo);
    gocator->Start();

    auto [x_resolution, y_resolution, z_resolution, bottom, width_n] = preprocessing(gocatorFifo);

    const auto x_width = global_config["x_width"].get<int>();
    const auto z_height_mm = global_config["z_height"].get<double>();
    const int nan_num = global_config["left_edge_nan"].get<int>();
    const int spike_window_size = nan_num / 4;

    BlockingReaderWriterQueue<msg> db_fifo;
    BlockingReaderWriterQueue<msg> dynamic_processing_fifo;
    std::jthread save_send(save_send_thread, std::ref(db_fifo), width_n);
    std::jthread dynamic_processing(dynamic_processing_thread, std::ref(dynamic_processing_fifo),std::ref(db_fifo), width_n);
    std::jthread origin_dectection(window_processing_thread, x_resolution, y_resolution, z_resolution, width_n, std::ref(winFifo), std::ref(dynamic_processing_fifo));

    auto gradient = belt_regression(64, gocatorFifo);

    auto iirfilter = mk_iirfilterSoS();
    auto delay = mk_delay(global_config["iirfilter"]["delay"]);

    uint64_t cnt = 0;

    double lowest_correlation = std::numeric_limits<double>::max();

    auto start = std::chrono::high_resolution_clock::now();

    int buffer_size_warning = 1024;

    cads::msg m;

    do
    {

      gocatorFifo.wait_dequeue(m);
      auto m_id = get<0>(m);

      if (m_id != cads::msgid::scan)
      {
        break;
      }

      auto p = get<profile>(get<1>(m));
      auto iy = p.y;
      auto ix = p.x_off;
      auto iz = p.z;

      if(p.z.size() < x_width * 0.75) {
        spdlog::get("cads")->error("Gocator sending profiles with widths less than 0.75 of expected width");
        continue;
      }


      ++cnt;

      spike_filter(iz, spike_window_size);
      auto [bottom_avg, top_avg, invalid] = barrel_offset(iz, z_height_mm);

      if (invalid)
      {
        // spdlog::get("cads")->error("Barrel not detected, profile is invalid");
      }

      auto bottom_filtered = iirfilter(bottom_avg);

      auto [delayed, dd] = delay({iy, ix, iz});
      if (!delayed)
        continue;

      auto [y, x, z] = dd;

      auto [left_edge_index, right_edge_index] = find_profile_edges_nans_outer(z, nan_num);

      nan_filter(z);
      regression_compensate(z, left_edge_index, right_edge_index, gradient);
      double edge_adjust = right_edge_index - left_edge_index - width_n;
      right_edge_index += -edge_adjust;

      std::tie(bottom_avg, top_avg, invalid) = barrel_offset(z, z_height_mm);
      barrel_height_compensate(z, -bottom_filtered);

      auto f = z | views::take(right_edge_index) | views::drop(left_edge_index);

      winFifo.enqueue({msgid::scan, cads::profile{y, x + left_edge_index * x_resolution, {f.begin(), f.end()}}});

    } while (std::get<0>(m) != msgid::finished);

    winFifo.enqueue({msgid::finished, 0});

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    origin_dectection.join();
    spdlog::get("cads")->info("CADS - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, (double)cnt / duration);

    gocator->Stop();
    spdlog::get("cads")->info("Gocator Stopped");

    save_send.join();
    spdlog::get("cads")->info("Upload Thread Stopped");
  }

  void generate_signal()
  {

    BlockingReaderWriterQueue<msg> gocatorFifo(4096 * 1024);

    auto gocator = mk_gocator(gocatorFifo);
    gocator->Start();

    const auto z_height_mm = global_config["z_height"].get<double>();
    const int nan_num = global_config["left_edge_nan"].get<int>();
    const int spike_window_size = nan_num / 4;

    auto iirfilter = mk_iirfilterSoS();
    auto delay = mk_delay(global_config["iirfilter"]["delay"]);

    cads::msg m;

    std::ofstream filt("filt.txt");

    do
    {

      gocatorFifo.wait_dequeue(m);
      auto m_id = get<0>(m);

      if (m_id == cads::msgid::scan)
      {

        auto p = get<profile>(get<1>(m));
        auto iy = p.y;
        auto ix = p.x_off;
        auto iz = p.z;

        spike_filter(iz, spike_window_size);
        auto [bottom_avg, top_avg, invalid] = barrel_offset(iz, z_height_mm);

        auto bottom_filtered = iirfilter(bottom_avg);

        filt << bottom_avg << "," << bottom_filtered << '\n';
        filt.flush();

        auto [delayed, dd] = delay({iy, ix, iz});
        if (!delayed)
          continue;

        auto [y, x, z] = dd;
      }

    } while (std::get<0>(m) != msgid::finished);

    gocator->Stop();
    spdlog::get("cads")->info("Gocator Stopped");
  }

  void stop_gocator()
  {

    BlockingReaderWriterQueue<msg> f;
    GocatorReader gocator(f);
    gocator.Stop();
  }

}
