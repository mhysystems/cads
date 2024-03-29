
#include <future>
#include <chrono>
#include <cassert>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow="
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wuseless-cast"

#include <spdlog/spdlog.h>
#include <common/scamp_interface.h>
#include <common/scamp_args.h>
#include <common/scamp_utils.h>

#pragma GCC diagnostic pop

#include <origin_detection_thread.h>
#include <coro.hpp>
#include <filters.h>
#include <opencv2/core.hpp>
#include <utils.hpp>
#include <db.h>




using namespace moodycamel;

namespace 
{
    auto scamp_impl(std::vector<double> timeseries_a, size_t window_size, std::vector<double> timeseries_b) {
      using namespace cads;
      using namespace SCAMP;

      SCAMPArgs args;
      args.window = window_size;
      args.max_tile_size = 1 << 17;
      args.has_b = timeseries_b.size() > 0;
      args.distributed_start_row = -1;
      args.distributed_start_col = -1;
      args.distance_threshold = 0.0;
      args.computing_columns = true;
      args.computing_rows = true;
      args.profile_a.type = SCAMP::PROFILE_TYPE_1NN_INDEX;
      args.profile_b.type = SCAMP::PROFILE_TYPE_1NN_INDEX;
      args.precision_type = SCAMP::PRECISION_DOUBLE;
      args.profile_type = SCAMP::PROFILE_TYPE_1NN_INDEX;
      args.keep_rows_separate = false;
      args.is_aligned = false;
      
      args.timeseries_b = timeseries_b;
      args.silent_mode = true;
      args.max_matches_per_column = 5;
      args.matrix_height = 50;
      args.matrix_width = 50;

      args.timeseries_a = std::move(timeseries_a);
      
      do_SCAMP(&args);

      return args;

    };


    auto scamp_dischord(std::vector<double> timeseries_a, size_t window_size) {
      auto args = scamp_impl(timeseries_a,window_size,{});
      auto [mp,mi] = ProfileToVector(args.profile_a, false, window_size);

      for(auto &e : mp) {
        e = std::isnan(e) ? std::numeric_limits<double>::lowest() : e;
      }
      
      auto dischord = std::max_element(mp.begin(),mp.end());

      auto d = std::distance(mp.begin(),dischord);
      std::vector<double> motif(args.timeseries_a.begin()+d,args.timeseries_a.begin() + d + window_size);
      return std::make_tuple(d,*dischord,motif);
    }

    auto scamp_match(std::vector<double> timeseries_a, std::vector<double> timeseries_b, size_t begin, size_t end) {
      using namespace std;

      assert(begin <= end);
      assert(timeseries_b.size() < timeseries_a.size());
      assert(timeseries_a.size() <= end);

      auto args = scamp_impl(timeseries_a,timeseries_b.size(),timeseries_b);
      auto [mp,mi] = ProfileToVector(args.profile_a, false, timeseries_b.size());
      
      for(auto &e : mp) {
        e = std::isnan(e) ? std::numeric_limits<double>::max() : e;
      }
      
      auto min = min_element(mp.begin()+begin,mp.end());
      auto d = distance(mp.begin(),min);
      return make_tuple(d,*min); 

    }
  
  cv::Mat window_to_mat(const std::deque<cads::profile>& win) {

    if(win.size() < 1) return cv::Mat(0,0,CV_32F);

    const int n_cols = (int)win[0].z.size();
    const int n_rows = (int)win.size();
    
    cv::Mat mat(n_rows,n_cols,CV_32F,cv::Scalar::all(0.0f));
    
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

}



namespace cads
{
  using window = std::deque<profile>;

  double left_edge_avg_height(const cv::Mat& belt, const cv::Mat& fiducial) {

    cv::Mat mout;
    cv::multiply(belt.rowRange(0,fiducial.rows),fiducial,mout);
    auto avg_val = cv::sum(mout)[0] / cv::countNonZero(mout);
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
  
  coro<std::tuple<profile,double,bool>,profile,1> maxlength_coro(double y_max_length)
  {
    spdlog::get("cads")->debug(R"({{func = '{}', msg = 'Slice data at {}'}})", __func__,y_max_length);
    profile p;
    bool terminate = false;

    std::tie(p,terminate) = co_yield {p,0.0,false}; 

    y_type y_offset = p.y;
    while (true)
    {
      
      if(terminate) break;

      y_type y = p.y - y_offset;

      if (y >= y_max_length)
      {
        y_offset = p.y;
      }
      
      p = {p.time,p.y - y_offset, p.x_off, p.z};

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

  coro<std::tuple<profile,double,bool>,profile,1> fiducial_origin_coro(double x_resolution, double y_resolution, int width_n, FiducialOriginDetection config, cads::Io<msg> &next)
  {    
    auto dump_match = config.dump_match;
    cv::Mat fiducial;
    cv::transpose(make_fiducial(x_resolution, y_resolution,config.fiducial),fiducial);
    
    if(dump_match) {
      mat_as_image(fiducial,"fid");
    }
    
    window profile_buffer;

    auto fdepth = config.fiducial.fiducial_depth;
    double lowest_correlation = std::numeric_limits<double>::max();
    double belt_crosscorr_threshold = config.cross_correlation_threshold;


    auto edge_height = config.fiducial.edge_height;
    auto avg_threshold_fn = mk_online_mean(edge_height - config.fiducial.fiducial_depth);

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

    auto y_max_length = std::get<1>(config.belt_length);
    auto trigger_length = std::numeric_limits<y_type>::lowest();
    y_type y_offset = 0;
    double y_lowest_correlation = 0;
    double cv_threshold_correleation = 0;
    cv::Mat matrix_correlation = belt.clone();
    long sequence_cnt = 0;
    auto valid = false;

    auto start = std::chrono::high_resolution_clock::now();

    while (true)
    {
      y_type y = profile_buffer.front().y;
 
      if(sequence_cnt >= 1) {
        next.enqueue({msgid::measure,Measure::MeasureMsg{"cadstoorigin",0,date::utc_clock::now(),y}});
      }

      if (y >= trigger_length)
      {
        auto l = left_edge_avg_height(belt, fiducial) - fdepth;
        const auto cv_threshhold = l;//avg_threshold_fn(l);
        auto [correlation,loc] = search_for_fiducial(belt, fiducial, m1, out, cv_threshhold);

        if(correlation <= lowest_correlation) {
          lowest_correlation = correlation;
          y_lowest_correlation = y;
          cv_threshold_correleation = cv_threshhold;
          matrix_correlation = belt.clone();
        }

        if (correlation < belt_crosscorr_threshold && (!(sequence_cnt > 0) || between(config.belt_length,y)))
        {
          ++sequence_cnt;
          spdlog::get("cads")->info("Correlation : {} at y : {} with threshold: {}", correlation, y, cv_threshhold);
          auto now = std::chrono::high_resolution_clock::now();
          auto period = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
          start = now;

          if(sequence_cnt > 1) {
            next.enqueue({msgid::measure,Measure::MeasureMsg{"beltrotationperiod",0,date::utc_clock::now(),(double)period}});
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
          trigger_length = std::numeric_limits<y_type>::lowest();
          lowest_correlation = std::numeric_limits<double>::max();
        }
      }

      std::tie(p,terminate) = co_yield {profile_buffer.front(),y,valid};

      if(terminate) break;

      shift_Mat_cols(belt);
      prepend_Mat_cols(belt, p.z);

      profile_buffer.pop_front();
      profile_buffer.push_back({p.time,p.y - y_offset, p.x_off, p.z});

    }
  }

  void loop_beltlength_thread(double length, cads::Io<msg> &profile_fifo, cads::Io<msg> &next_fifo)
  {
    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}'}})", __func__,"Entering Thread");
    cads::msg m;

    auto start = std::chrono::high_resolution_clock::now();
    int64_t cnt = 0;
    auto buffer_size_warning = buffer_warning_increment;

    profile_fifo.wait_dequeue(m);
    auto m_id = get<0>(m);

    if (m_id == cads::msgid::finished)
    {
      return;
    }

    if (m_id != cads::msgid::gocator_properties)
    {
      std::throw_with_nested(std::runtime_error("preprocessing:First message must be gocator_properties"));
    }
    
    next_fifo.enqueue(m);


    auto origin_detection = maxlength_coro(length);

    long origin_sequence_cnt = 0;
    size_t scan_cnt = 0;
    auto scanprogress = -1.0;

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
              

              if(origin_sequence_cnt >= 1) {
                next_fifo.enqueue({msgid::measure,Measure::MeasureMsg{"cadstoorigin",0,date::utc_clock::now(),op.y}});
              }

              auto scanprogress_now = std::floor(100* estimated_belt_length / length);

              if(origin_sequence_cnt > 0 && scanprogress != scanprogress_now) {
                scanprogress = scanprogress_now;
                next_fifo.enqueue({msgid::caas_msg,CaasMsg{"scanprogress", std::to_string(scanprogress_now)}});  
              }

              if(op.y == 0) {
                
                auto now = std::chrono::high_resolution_clock::now();
                auto period = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
                start = now;
                
                if(origin_sequence_cnt == 0) {
                  spdlog::get("cads")->debug(R"({{func = '{}', msg = 'Begin Sequence'}})", __func__);
                  next_fifo.enqueue({msgid::begin_sequence, origin_sequence_cnt});
                }

                if(origin_sequence_cnt > 0) {
                  spdlog::get("cads")->debug(R"({{func = '{}', msg = 'Estimated belt length {}'}})", __func__,estimated_belt_length);  
                  next_fifo.enqueue({msgid::measure,Measure::MeasureMsg{"beltrotationperiod",0,date::utc_clock::now(),(double)period}});
                  next_fifo.enqueue({msgid::measure,Measure::MeasureMsg{"beltlength",0,date::utc_clock::now(),estimated_belt_length}});
                  next_fifo.enqueue({msgid::complete_belt, CompleteBelt{0,scan_cnt}});
                }
                
                scan_cnt = 0;
                origin_sequence_cnt++;
              }
              
              next_fifo.enqueue({msgid::scan, op}); scan_cnt++;
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

    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}'}})", __func__,"Exiting Thread");
  }


  void fiducial_origin_thread(FiducialOriginDetection config,cads::Io<msg> &profile_fifo, cads::Io<msg> &next_fifo)
  {
    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}'}})", __func__,"Entering Thread");
    cads::msg m;

    int64_t cnt = 0;
    auto buffer_size_warning = buffer_warning_increment;
    double x_resolution = 1.0, y_resolution = config.avg_y_res;
    int width_n = (int)config.width_n;

    profile_fifo.wait_dequeue(m);
    auto m_id = get<0>(m);

    if (m_id == cads::msgid::finished)
    {
      return;
    }

    if (m_id != cads::msgid::gocator_properties)
    {
      std::throw_with_nested(std::runtime_error("preprocessing:First message must be gocator_properties"));
    }
    else
    {
      auto p = get<GocatorProperties>(get<1>(m));
      x_resolution = p.xResolution;
    }
    next_fifo.enqueue(m);


    auto origin_detection = fiducial_origin_coro(x_resolution,y_resolution,width_n,config,next_fifo);

    long origin_sequence_cnt = 0;
    size_t scan_cnt = 0;

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
                  next_fifo.enqueue({msgid::measure,Measure::MeasureMsg{"beltlength",0,date::utc_clock::now(),estimated_belt_length}});
                  next_fifo.enqueue({msgid::complete_belt, CompleteBelt{0,scan_cnt}});
                }
                
                scan_cnt = 0;
                origin_sequence_cnt++;
              }
              
              next_fifo.enqueue({msgid::scan, op});
            }else{
              next_fifo.enqueue({msgid::end_sequence, origin_sequence_cnt});
              origin_sequence_cnt = 0;
            }
            scan_cnt++;
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

    spdlog::get("cads")->debug(R"({{func = '{}', msg = '{}'}})", __func__,"Exiting Thread");
  }


   
coro<std::tuple<size_t,double,std::vector<double>>,double,1> find_dischord_motif_coro(size_t window_size, size_t partition_size)
{
    using namespace std::chrono_literals;  

    std::vector<double> xs;
    double x = 0;
    auto terminate = false;
    auto empty_return = std::make_tuple(size_t(0),0.0,std::vector<double>());
    for (;!terminate && xs.size() < partition_size;)
    {
      std::tie(x,terminate) = co_yield empty_return;
      if(terminate) continue;

      xs.push_back(x);
    }

    auto scamp_fut = std::async(scamp_dischord,xs,window_size);

    for (;!terminate && (scamp_fut.wait_for(0s) != std::future_status::ready);)
    {
        std::tie(x,terminate) = co_yield empty_return;
    }

    co_return scamp_fut.get();

}



coro<std::tuple<bool,size_t,size_t,double>,profile,1> anomaly_detection_coro(AnomalyDetection anomaly)
  {    
    using namespace SCAMP;
    using namespace std::chrono_literals;
    using namespace std;
    
    profile p;
    bool terminate = false;
    
    std::vector<double> belt_thickness_estimates, y_position;
    enum class State {noMotif,findMotif,foundMotif};

    State state = State::findMotif;

    auto find_dischord_motif  = find_dischord_motif_coro(anomaly.WindowSize,anomaly.BeltPartitionSize);

    auto [motif_creation,motif] = fetch_last_motif();

    if(motif.empty()) state = State::noMotif;

    size_t index = 0;
    double distance = 0;
    bool found = false;
    auto y_pos = 0.0;
    
    while (!terminate)
    {
      
      std::tie(p,terminate) = co_yield {found,index,index-anomaly.WindowSize,y_pos};
      if(terminate) break;
      
      found = false;
      y_position.push_back(p.y);
      auto belt_thickness_estimate = interquartile_mean(p.z);
      belt_thickness_estimates.push_back(belt_thickness_estimate);

      for(auto processing = true; processing;) {
        processing = false;
        switch(state) {
          case State::noMotif : {
            auto [done,ret] = find_dischord_motif.resume(belt_thickness_estimate);

            if(done) {
              spdlog::get("cads")->info("{{func = {}, msg = 'Detect first motif'}}");
              std::tie(index,distance,motif) = ret;
              //store_motif_state({date::utc_clock::now(),motif});
              state = State::foundMotif;
              processing = true;
            }

            break;
          }

          case State::findMotif: {
            
            if(belt_thickness_estimates.size() > anomaly.BeltSize){
              tie(index,distance) = scamp_match(belt_thickness_estimates,motif,anomaly.MinPosition,anomaly.MaxPosition);
              spdlog::get("cads")->info("index:{} dis:{} size:{}", index,distance,belt_thickness_estimates.size());
              state = State::foundMotif;
              processing = true;
            }
            break;
          }

          case State::foundMotif: {
            spdlog::get("cads")->info("{{func = {}, msg = 'Found motif'}}");
            y_pos = y_position[index];
            index += anomaly.WindowSize;
            
            if(index > belt_thickness_estimates.size()){
              index = belt_thickness_estimates.size();
            } 
            std::shift_left(belt_thickness_estimates.begin(),belt_thickness_estimates.end(),index);
            belt_thickness_estimates.resize(belt_thickness_estimates.size() - index);
            std::shift_left(y_position.begin(),y_position.end(),index);
            y_position.resize(y_position.size() - index);
            state = State::findMotif;
            found = true;
            break;
          }

          default:
            break;
        }
      }

    }
  }

  void splice_detection_thread(cads::AnomalyDetection anomaly, cads::Io<msg> &profile_fifo,cads::Io<msg> &next_fifo)
  {

    cads::msg m;

    auto start = std::chrono::high_resolution_clock::now();
    int64_t cnt = 0;
    auto buffer_size_warning = buffer_warning_increment;
    double last_splice_position = 0;
    size_t last_splice_index = 0;

    auto origin_detection = anomaly_detection_coro(anomaly);

    long origin_sequence_cnt = 0L;

    for (auto loop = true;loop;++cnt)
    {
      
      profile_fifo.wait_dequeue(m);

      switch(std::get<0>(m)) {

        case msgid::scan: {
          auto p = get<profile>(get<1>(m));
          next_fifo.enqueue({msgid::scan, p});
          
          auto [coro_end,result] = origin_detection.resume(p);
          
          if(!coro_end) {
            auto [valid,index,anomaly_index,pos] = result;
            if(valid) {
              auto estimated_belt_length = pos - last_splice_position;
              if(origin_sequence_cnt > 0) {
                spdlog::get("cads")->info("Estimated Belt Length(m): {}", estimated_belt_length / 1000);
                next_fifo.enqueue({msgid::measure,Measure::MeasureMsg{"beltlength",0,date::utc_clock::now(),estimated_belt_length}});
                next_fifo.enqueue({msgid::complete_belt, CompleteBelt{last_splice_index,index}});
                last_splice_position = pos;
                last_splice_index = 0;
              }else {
                spdlog::get("cads")->info("distance:{}", p.y - pos);
                last_splice_position = pos;
                last_splice_index = index;
              }
              origin_sequence_cnt++;
              std::deque<std::tuple<int, cads::profile>> msg;
              moodycamel::BlockingReaderWriterQueue<decltype(msg)> rows;
              next_fifo.enqueue({msgid::select,Select{&rows,anomaly_index,anomaly.WindowSize}});

              rows.wait_dequeue(msg);

              std::deque<cads::profile> win;
              for(auto e : msg) {
                win.push_back(get<1>(e));
              }
              
              auto mat = window_to_mat(win);
              mat_as_image(mat,fmt::format("splice{}",origin_sequence_cnt));
              
              //testing remove it 
              if(origin_sequence_cnt > 1) {
                next_fifo.enqueue({msgid::stopped,0});
              }
            }
          }else 
          {
            loop = false;
            spdlog::get("cads")->error("Origin detector stopped");
          }
        break;
        }
        case msgid::finished:

          loop = false;
          next_fifo.enqueue(m);
          break;
        case msgid::gocator_properties :
          next_fifo.enqueue(m);
          next_fifo.enqueue({msgid::begin_sequence, 0});
          break;
        default:
          next_fifo.enqueue(m);
          break;
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

}
