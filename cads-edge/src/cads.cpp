#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>

#include <cads.h>
#include <regression.h>

#include <db.h>
#include <coms.h>
#include <constants.h>
#include <fiducial.h>
#include <window.hpp>
#include <readerwriterqueue.h>
#include <gocator_reader.h>
#include <sqlite_gocator_reader.h>

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
#include <fstream>
#include <iostream>

#include <spdlog/spdlog.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"

#include <date/date.h>
#include <date/tz.h>
#include <fmt/core.h>
#include <fmt/chrono.h>

#pragma GCC diagnostic pop

#include <filters.h>
#include <edge_detection.h>
#include <coro.hpp>
#include <dynamic_processing.h>
#include <save_send_thread.h>
#include <origin_detection_thread.h>
#include <intermessage.h>
#include <belt.h>

using namespace std;
using namespace moodycamel;
using namespace std::chrono;
using CadsMat = cv::UMat; // cv::cuda::GpuMat

namespace cads
{

  unique_ptr<GocatorReaderBase> mk_gocator(BlockingReaderWriterQueue<msg> &gocatorFifo)
  {
    auto data_src = global_config["data_source"].get<std::string>();

    if (data_src == "gocator"s)
    {
      bool connect_via_ip = global_config.contains("gocator_ip");
      spdlog::get("cads")->debug("Using gocator as data source");
      return connect_via_ip ? make_unique<GocatorReader>(gocatorFifo, global_config.at("gocator_ip"s).get<std::string>()) : make_unique<GocatorReader>(gocatorFifo);
    }
    else
    {
      spdlog::get("cads")->debug("Using sqlite as data source");
      return make_unique<SqliteGocatorReader>(gocatorFifo);
    }
  }

  void upload_profile_only()
  {
    http_post_whole_belt(0, std::numeric_limits<int>::max(), 0);
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

    if (get<0>(m) != cads::msgid::resolutions)
    {
      std::throw_with_nested(std::runtime_error("First message must be resolutions"));
    }

    auto [y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution] = get<resolutions_t>(get<1>(m));
    store_profile_parameters({y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution, global_config["z_height"].get<double>()});

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
        store_profile.resume({0, idx++, p});
        break;
      }
      case cads::msgid::resolutions:
      {
        break;
      }

      case cads::msgid::finished:
      {
        continue;
      }
      default:
        continue;
      }

    } while (get<0>(m) != msgid::finished && Y < 2 * y_max_length);

    store_profile.terminate();

    gocator->Stop();
  }

  bool process_impl()
  {
    create_db(global_config["db_name"].get<std::string>().c_str());

    BlockingReaderWriterQueue<msg> gocatorFifo(4096 * 1024);
    BlockingReaderWriterQueue<msg> winFifo(4096 * 1024);

    const auto x_width = global_config["x_width"].get<int>();
    const auto nan_percentage = global_config["nan_%"].get<double>();
    const auto width_n = global_config["width_n"].get<int>();
    const z_element clip_height = global_config["clip_height"].get<z_element>();

    auto gocator = mk_gocator(gocatorFifo);
    gocator->Start();

    cads::msg m;
    gocatorFifo.wait_dequeue(m);
    auto m_id = get<0>(m);

    if (m_id != cads::msgid::resolutions)
    {
      std::throw_with_nested(std::runtime_error("preprocessing:First message must be resolutions"));
    }

    auto [y_resolution, x_resolution, z_resolution, z_offset, encoder_resolution] = get<resolutions_t>(get<1>(m));
    store_profile_parameters({y_resolution, x_resolution, z_resolution, 33.0, encoder_resolution, clip_height});

    BlockingReaderWriterQueue<msg> db_fifo;
    BlockingReaderWriterQueue<msg> dynamic_processing_fifo;

    bool terminate_publish = false;
    std::jthread realtime_publish(realtime_publish_thread, std::ref(terminate_publish));
    std::jthread save_send(save_send_thread, std::ref(db_fifo));
    std::jthread dynamic_processing(dynamic_processing_thread, std::ref(dynamic_processing_fifo), std::ref(db_fifo), width_n);
    std::jthread origin_dectection(window_processing_thread, x_resolution, y_resolution, width_n, std::ref(winFifo), std::ref(dynamic_processing_fifo));

    auto iirfilter_left = mk_iirfilterSoS();
    auto iirfilter_right = mk_iirfilterSoS();
    auto delay = mk_delay(global_config["iirfilter"]["delay"]);

    int64_t cnt = 0;

    auto start = std::chrono::high_resolution_clock::now();

    auto schmitt_trigger = mk_schmitt_trigger(0.1f);
    auto differentiation = mk_differentiation(-32.5);
    auto pulley_frequency = mk_pulley_frequency();
    auto profiles_align = mk_profiles_align(width_n);

    long drop_profiles = global_config["iirfilter"]["skip"]; // Allow for iir fillter too stablize
    bool error = false;

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
      ++cnt;

      if (iz.size() < size_t(x_width * 0.75))
      {
        spdlog::get("cads")->error("Gocator sending profiles with widths less than 0.75 of expected width");
        error = true;
        break;
      }

      if (iz.size() < (size_t)width_n)
      {
        spdlog::get("cads")->error("Gocator sending profiles with sample number {} less than {}", iz.size(), width_n);
        error = true;
        break;
      }

      double nan_cnt = std::count_if(iz.begin(), iz.end(), [](z_element z)
                                   { return std::isnan(z); });

      if ((nan_cnt / iz.size()) > nan_percentage )
      {
        spdlog::get("cads")->error("Percentage of nan({}) in profile > {}%", nan_cnt,nan_percentage);
        error = true;
        break;
      }

      spike_filter(iz);

      auto [ileft_edge_index, iright_edge_index] = find_profile_edges_sobel(nan_filter_pure(iz));
      auto [pulley_left, pulley_right] = pulley_left_right_mean(iz, ileft_edge_index, iright_edge_index);

      auto pulley_left_filtered = (z_element)iirfilter_left(pulley_left);
      auto pulley_right_filtered = (z_element)iirfilter_right(pulley_right);
      auto bottom_filtered = pulley_left_filtered;

      auto barrel_cnt = pulley_frequency(differentiation(bottom_filtered));
      winFifo.enqueue({msgid::barrel_rotation_cnt, barrel_cnt});

      auto [delayed, dd] = delay({iy, ix, iz, ileft_edge_index, iright_edge_index});

      if (!delayed)
        continue;

      if (drop_profiles > 0)
      {
        --drop_profiles;
        continue;
      }

      auto [y, x, z, left_edge_index, right_edge_index] = dd;

      auto gradient = (pulley_right_filtered - pulley_left_filtered) / (double)z.size();
      regression_compensate(z, 0, z.size(), gradient);

      nan_filter(z);
      left_edge_index = profiles_align(z, left_edge_index, right_edge_index);

      barrel_height_compensate(z, -bottom_filtered, clip_height);

      auto f = z | views::take(left_edge_index + width_n) | views::drop(left_edge_index);

      winFifo.enqueue({msgid::scan, cads::profile{y, x + left_edge_index * x_resolution, {f.begin(), f.end()}}});

    } while (std::get<0>(m) != msgid::finished);

    winFifo.enqueue({msgid::finished, 0});

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    origin_dectection.join();
    auto rate = duration != 0 ? (double)cnt / duration : 0;
    spdlog::get("cads")->info("CADS - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, rate);

    gocator->Stop();
    spdlog::get("cads")->info("Gocator Stopped");

    save_send.join();
    spdlog::get("cads")->info("Upload Thread Stopped");

    terminate_publish = true;
    realtime_publish.join();
    spdlog::get("cads")->info("Realtime publishing Thread Stopped");

    return error;
  }

  void process()
  {
    for (int sleep_wait = 1;;)
    {
      
      auto start = std::chrono::high_resolution_clock::now();

      bool error = process_impl();

      auto now = std::chrono::high_resolution_clock::now();
      auto period = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();

      if (period > sleep_wait * 60)
      {
        sleep_wait = 1;
      }

      if (error)
      {
        spdlog::get("cads")->info("Sleeping for {} minutes",sleep_wait);
        std::this_thread::sleep_for(std::chrono::seconds(sleep_wait * 60));

        if (sleep_wait < 128)
        {
          sleep_wait *= 2;
        }
      }
      else
      {
        break;
      }
    }
  }

  void generate_signal()
  {

    BlockingReaderWriterQueue<msg> gocatorFifo(4096 * 1024);

    auto gocator = mk_gocator(gocatorFifo);
    gocator->Start();

    const int nan_num = global_config["left_edge_nan"].get<int>();
    const int spike_window_size = nan_num * 2;

    auto iirfilter = mk_iirfilterSoS();
    auto delay = mk_delay(global_config["iirfilter"]["delay"]);

    cads::msg m;

    auto differentiation = mk_differentiation(0);

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
        auto [ileft_edge_index, iright_edge_index] = find_profile_edges_nans_outer(iz);
        auto gradient = barrel_gradient(iz, ileft_edge_index, iright_edge_index);
        regression_compensate(iz, 0, iz.size(), gradient);
        auto bottom_avg = barrel_mean(iz, ileft_edge_index, iright_edge_index);

        auto bottom_filtered = iirfilter(bottom_avg);

        filt << bottom_filtered << "," << differentiation(bottom_filtered) << '\n';
        filt.flush();

        auto [delayed, dd] = delay({iy, ix, iz, 0, 0});
        if (!delayed)
          continue;

        auto [y, x, z, ignore_a, ignore_b] = dd;
      }

    } while (std::get<0>(m) != msgid::finished);

    gocator->Stop();
    spdlog::get("cads")->info("Gocator Stopped");
  }

  void generate_belt_parameters(long cnt)
  {
    BlockingReaderWriterQueue<msg> gocatorFifo(4096 * 1024);

    auto gocator = mk_gocator(gocatorFifo);
    gocator->Start();

    const int nan_num = global_config["left_edge_nan"].get<int>();
    const int spike_window_size = nan_num * 2;

    cads::msg m;
    double sum_left_mean = 0, sum_right_mean = 0;
    long sum_width_n = 0;
    long loop_cnt = cnt;

    do
    {

      gocatorFifo.wait_dequeue(m);
      auto m_id = get<0>(m);

      if (m_id == cads::msgid::scan)
      {

        auto p = get<profile>(get<1>(m));
        auto iz = p.z;

        spike_filter(iz, spike_window_size);
        auto [left_edge_index, right_edge_index] = find_profile_edges_nans_outer(iz);

        auto [left_mean, right_mean] = pulley_left_right_mean(iz, left_edge_index, right_edge_index);

        sum_left_mean += left_mean;
        sum_right_mean += right_mean;

        sum_width_n += right_edge_index - left_edge_index;
        if (--loop_cnt == 0)
          break;
      }

    } while (std::get<0>(m) != msgid::finished);

    gocator->Stop();

    long width_n = sum_width_n / (cnt - 1);
    double left_mean = sum_left_mean / (cnt - 1);
    double right_mean = sum_right_mean / (cnt - 1);
    double gradient = (right_mean - left_mean) / width_n;
    std::cout << fmt::format("width_n : {}, regression_gradient : {}, left_mean : {}, right_mean : {}", width_n, gradient, left_mean, right_mean);
  }

  void stop_gocator()
  {

    BlockingReaderWriterQueue<msg> f;
    GocatorReader gocator(f);
    gocator.Stop();
  }

}
