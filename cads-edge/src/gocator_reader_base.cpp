#include <algorithm>

#include <spdlog/spdlog.h>

#include <gocator_reader_base.h>



namespace cads
{
  GocatorReaderBase::GocatorReaderBase(Io<msg>& fifo) : m_gocatorFifo(fifo)
  {
  }

  bool GocatorReaderBase::Start_impl() {
    return false;
  }

  void GocatorReaderBase::Stop_impl(bool) {
  }

  bool GocatorReaderBase::Align_impl() {
    return false;
  }

  bool GocatorReaderBase::SetFrameRate(double) {
    return false;
  }

  bool GocatorReaderBase::SetFoV_impl(double) {
    return false;
  }

  bool GocatorReaderBase::ResetAlign_impl() {
    return false;
  }


  bool GocatorReaderBase::Start(double fps) {
    spdlog::get("cads")->debug(R"({{func = '{}', msg = 'Framerate {}'}})", __func__,fps);
    SetFrameRate(fps);
    return Start_impl();
  }

  void GocatorReaderBase::Stop(bool signal_finished) {
    Stop_impl(signal_finished);
  }

  bool GocatorReaderBase::Align() {
    return Align_impl();
  }

  bool GocatorReaderBase::SetFoV(double len) {
    return SetFoV_impl(len);
  }

  bool GocatorReaderBase::ResetAlign() {
    return ResetAlign_impl();
  }

  z_type GocatorReaderBase::k16sToFloat(k16s *z_start, k16s *z_end, double z_resolution, double z_offset)
  {
    z_type rtn;
    std::transform(z_start, z_end, std::back_inserter(rtn), [=](k16s e)
                   { return e != k16S_NULL ? e * z_resolution + z_offset : std::numeric_limits<z_element>::quiet_NaN(); });

    return rtn;
  }

  std::tuple<std::string,std::tuple<std::tuple<std::string,double>
  ,std::tuple<std::string,double>
  ,std::tuple<std::string,double>
  ,std::tuple<std::string,double>
  ,std::tuple<std::string,double>
  ,std::tuple<std::string,double>
  ,std::tuple<std::string,double> >> GocatorProperties::decompose()
  {
    using namespace std::literals;
    return {"Gocator",{{"xResolution"s,xResolution}, {"zResolution"s,zResolution}, {"zOffset"s,zOffset}, {"xOrigin"s,xOrigin}, {"width"s,width}, {"zOrigin"s,zOrigin}, {"height"s,height}}};
  }   

}
