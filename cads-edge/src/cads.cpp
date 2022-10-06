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
#include <coms.h>

using namespace std;
using namespace moodycamel;
using namespace std::chrono;
using CadsMat = cv::UMat; // cv::cuda::GpuMat

namespace cads
{

  tuple<z_element, z_element> barrel_offset(int cnt, BlockingReaderWriterQueue<msg> &ps)
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
    const auto thickness = z_height_mm;

    // Remove z values greater than the peak minus approx belt thickness.
    // Assumes the next peak will be the barrel values
    auto f = hist | views::filter([thickness, peak](tuple<double, z_element> a)
                                  { return peak - get<0>(a) > thickness; });
    vector<tuple<double, z_element>> barrel(f.begin(), f.end());

    return {get<0>(barrel[0]), peak};
  }

  auto belt_width_n(int cnt, BlockingReaderWriterQueue<msg> &ps)
  {

    const int nan_num = global_config["left_edge_nan"].get<int>();
    const int spike_window_size = nan_num * 2;
    int width = 0, left_edge = 0;
    const int n = cnt - 1;
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

        if (right_edge_index <= left_edge_index)
        {
          std::throw_with_nested(std::runtime_error("Right edge index less than left edge index"));
        }

        width += right_edge_index - left_edge_index;
        left_edge += left_edge_index;
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
    left_edge /= n;
    return std::tuple{left_edge, width};
  }

  double belt_regression(int cnt, BlockingReaderWriterQueue<msg> &ps)
  {

    const int nan_num = global_config["left_edge_nan"].get<int>();

    const int spike_window_size = nan_num * 2;
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
      bool connect_via_ip = global_config.contains("gocator_ip");
      spdlog::get("cads")->debug("Using gocator as data source");
      return connect_via_ip ?  make_unique<GocatorReader>(gocatorFifo,global_config.at("gocator_ip"s).get<std::string>()) : make_unique<GocatorReader>(gocatorFifo);
    }
    else
    {
      spdlog::get("cads")->debug("Using sqlite as data source");
      return make_unique<SqliteGocatorReader>(gocatorFifo);
    }
  }

  void upload_profile_only()
  {
    http_post_whole_belt(0, std::numeric_limits<int>::max(),0);
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

  
  auto preprocessing(BlockingReaderWriterQueue<msg> &gocatorFifo)
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

    if (std::floor(y_resolution / encoder_resolution) != (y_resolution / encoder_resolution))
    {
      spdlog::get("cads")->error("Danger! Y resolution not Integer multiple of Encoder Resolution");
    }

    auto [bottom, top] = barrel_offset(1024, gocatorFifo);
    auto [left_edge_index, width_n] = belt_width_n(1024, gocatorFifo);
    spdlog::get("cads")->info("Belt properties - botton:{}, top:{}, height(mm):{}, width:{}, width_n:{}", bottom, top, top - bottom, width_n * x_resolution, width_n);
    store_profile_parameters({y_resolution, x_resolution, z_resolution, -(double)bottom, encoder_resolution, top - bottom});

    return std::tuple{x_resolution, y_resolution, z_resolution, bottom, top, width_n, left_edge_index};
  }

  void process()
  {
    create_db(global_config["db_name"].get<std::string>().c_str());

    BlockingReaderWriterQueue<msg> gocatorFifo(4096 * 1024);
    BlockingReaderWriterQueue<msg> winFifo(4096 * 1024);

    auto gocator = mk_gocator(gocatorFifo);
    gocator->Start();

    auto [x_resolution, y_resolution, z_resolution, bottom, belt_top, width_n, left_edge_index_init] = preprocessing(gocatorFifo);
    int fiducial_x = (int) (double(make_fiducial(x_resolution, y_resolution).cols) * 1.5);

    const auto x_width = global_config["x_width"].get<int>();
    const auto z_height_mm = global_config["z_height"].get<double>();
    const int nan_num = global_config["left_edge_nan"].get<int>();
    const int spike_window_size = nan_num * 2;
    auto pully_circumfrence = global_config["pully_circumfrence"].get<double>(); // In mm

    BlockingReaderWriterQueue<msg> db_fifo;
    BlockingReaderWriterQueue<msg> dynamic_processing_fifo;

    bool terminate_publish = false;
    std::jthread realtime_publish(realtime_publish_thread,std::ref(terminate_publish));
    std::jthread save_send(save_send_thread, std::ref(db_fifo));
    std::jthread dynamic_processing(dynamic_processing_thread, std::ref(dynamic_processing_fifo), std::ref(db_fifo), width_n);
    std::jthread origin_dectection(window_processing_thread, x_resolution, y_resolution, width_n, std::ref(winFifo), std::ref(dynamic_processing_fifo));

    auto gradient = belt_regression(64, gocatorFifo);

    auto iirfilter = mk_iirfilterSoS();
    auto delay = mk_delay(global_config["iirfilter"]["delay"]);

    int64_t cnt = 0;
    z_element clip_height = belt_top + 3.0f;

    auto start = std::chrono::high_resolution_clock::now();
    auto barrel_origin_time = std::chrono::high_resolution_clock::now();

    cads::msg m;
    cads::z_element bottom_filtered0 = bottom; // For differential calculation
    long barrel_cnt = 0;
    auto schmitt_trigger = mk_schmitt_trigger(0.001f);
    auto edge_adjust = mk_edge_adjust(left_edge_index_init, width_n);
    auto amplitude_extraction = mk_amplitude_extraction();
    z_element schmitt1 = 1.0, schmitt0 = -1.0;
    z_type prev_z;

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

      if (iz.size() < size_t(x_width * 0.75))
      {
        spdlog::get("cads")->error("Gocator sending profiles with widths less than 0.75 of expected width");
        continue;
      }

      if (iz.size() < (size_t)width_n)
      {
        spdlog::get("cads")->error("Gocator sending profiles with sample number {} less than {}", iz.size(), width_n);
        iz.insert(iz.end(), width_n - iz.size(), bottom);
      }

      ++cnt;

      spike_filter(iz, spike_window_size);
      auto [bottom_avg, top_avg, invalid] = barrel_offset(iz, z_height_mm);

      if (invalid)
      {
        // spdlog::get("cads")->error("Barrel not detected, profile is invalid");
      }

      auto bottom_filtered = (z_element)iirfilter(bottom_avg);

      auto dbottom1 = bottom_filtered - bottom_filtered0;
      schmitt1 = schmitt_trigger(dbottom1);
      amplitude_extraction(bottom_filtered,false);

      if ((std::signbit(schmitt1) == false && std::signbit(schmitt0) == true) || (std::signbit(schmitt1) == true && std::signbit(schmitt0) == false) )
      {
        auto now = std::chrono::high_resolution_clock::now();
        auto period = std::chrono::duration_cast<std::chrono::milliseconds>(now - barrel_origin_time).count();
        if (barrel_cnt % 100 == 0)
        {
          spdlog::get("cads")->info("Barrel Frequency(Hz): {}", 1000.0 / ((double)period) *2 );
          publish_meta_realtime("PullyOscillation",amplitude_extraction(bottom_filtered,true));
          publish_meta_realtime("SurfaceSpeed",pully_circumfrence / (2 * period));
        }
        winFifo.enqueue({msgid::barrel_rotation_cnt, barrel_cnt});
        barrel_cnt++;
        barrel_origin_time = now;
      }

      bottom_filtered0 = bottom_filtered;
      schmitt0 = schmitt1;

      auto [delayed, dd] = delay({iy, ix, iz});
      
      if (!delayed)
        continue;

      auto [y, x, z] = dd;

      auto [left_edge_index, right_edge_index] = find_profile_edges_nans_outer(z, nan_num);

      nan_filter(z);
      regression_compensate(z, left_edge_index, right_edge_index, gradient);

      if(prev_z.size() != 0) {
        auto dbg = correlation_lowest(prev_z, z | views::take(right_edge_index) | views::drop(left_edge_index));
        left_edge_index -= dbg;
        auto f = z | views::take(left_edge_index + width_n) | views::drop(left_edge_index);
        prev_z = {f.begin(), f.end()};
      }

      //left_edge_index = edge_adjust(left_edge_index, right_edge_index);


      std::tie(bottom_avg, top_avg, invalid) = barrel_offset(z, z_height_mm);

      auto avg = z | views::take(left_edge_index + fiducial_x) | views::drop(left_edge_index);
      float  avg2 = (float)std::accumulate(avg.begin(), avg.end(), 0.0) / (float)fiducial_x;
      clip_height = (clip_height + avg2) / 2.0f;
      barrel_height_compensate(z, -bottom_filtered, clip_height + 3.0f);

      auto f = z | views::take(left_edge_index + width_n) | views::drop(left_edge_index);
      
      winFifo.enqueue({msgid::scan, cads::profile{y, x + left_edge_index * x_resolution, {f.begin(), f.end()}}});

    } while (std::get<0>(m) != msgid::finished);

    winFifo.enqueue({msgid::finished, 0});

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    origin_dectection.join();
    spdlog::get("cads")->info("CADS - CNT: {}, DUR: {}, RATE(ms):{} ", cnt, duration, cnt / duration);

    gocator->Stop();
    spdlog::get("cads")->info("Gocator Stopped");

    save_send.join();
    spdlog::get("cads")->info("Upload Thread Stopped");

    terminate_publish = true;
    realtime_publish.join();
    spdlog::get("cads")->info("Realtime publishing Thread Stopped");
  }

  void generate_signal()
  {

    BlockingReaderWriterQueue<msg> gocatorFifo(4096 * 1024);

    auto gocator = mk_gocator(gocatorFifo);
    gocator->Start();

    const auto z_height_mm = global_config["z_height"].get<double>();
    const int nan_num = global_config["left_edge_nan"].get<int>();
    const int spike_window_size = nan_num * 2;

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
