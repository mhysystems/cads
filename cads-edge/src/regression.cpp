#include <vector>
#include <numeric>
#include <ranges>
#include <cmath>
#include <limits>

#include <Eigen/Dense>
#include <unsupported/Eigen/NonLinearOptimization>

#include <regression.h>
#include <constants.h>

namespace cads
{

  std::tuple<double, double> linear_regression(const z_type &yy, int left_edge_index, int right_edge_index) {
    
    namespace sr = std::ranges;
    using namespace Eigen;

    auto r = yy | sr::views::take(right_edge_index) | sr::views::drop(left_edge_index) | sr::views::filter([](float a)
                                                                                                           { return !std::isnan(a); });

    std::vector<double> yr(r.begin(), r.end());
    auto y = Map<VectorXd>(yr.data(), yr.size());

    MatrixXd A(y.size(),2);
    A.setOnes();
    A.col(1) = ArrayXd::LinSpaced(y.size(), 0, y.size()-1);
    MatrixXd s = A.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(y); 
    
    return {s(0,0),s(1,0)};
  }

  std::tuple<double, double> linear_regression(const z_type &yy) {
    return linear_regression(yy,0,yy.size());
  }

  void regression_compensate(z_type &z, int left_edge_index, int right_edge_index, double gradient)
  {
    namespace sr = std::ranges;

    auto f = z | sr::views::take(right_edge_index) | sr::views::drop(left_edge_index);
    std::transform(f.begin(), f.end(), f.begin(), [gradient, i = 0](z_element v) mutable -> z_element
                   { return v != std::numeric_limits<z_element>::quiet_NaN() ? v - (gradient * i++) : std::numeric_limits<z_element>::quiet_NaN(); });
  }


  int correlation_lowest(float *a, size_t al, float *b, size_t bl)
  {

    using namespace Eigen;

    auto va = Map<VectorXf>(a, al);
    auto vb = Map<VectorXf>(b, bl);

    auto bs = bl - 1;
    auto ds = al - bl;
    auto c = 0;
    auto r = std::numeric_limits<float>::max();

    for (size_t i = 0; i <= ds; ++i)
    {
      auto cr = std::abs((va(seq(i, i + bs)) - vb).sum());
      if (cr < r)
      {
        r = cr;
        c = i;
      }
    }

    return c;
  }


void belt_model(Eigen::VectorXf &z, float height, float x_offset, float z_offset,const long width_n, const float x_res) {
    
    z.fill(z_offset);
    
    auto offset_n =  (z.rows() - width_n) / 2 + (long)std::round(x_offset / x_res);

    if(height < 0) height = 0;
    if(offset_n < 0) offset_n = 0;
    if(offset_n > (z.rows() - width_n)) offset_n = (z.rows() - width_n);

    for(auto i = offset_n; i < offset_n + width_n; ++i) {
      z(i) = height + z_offset;
    } 
  }

  
  std::function<std::tuple<double,double,double>(z_type)>  mk_curvefitter(float init_height, float init_x_offset, float init_z_offset, const int width_n, const double x_res, const double z_res) {
    
    const auto belt_height = 0;
    const auto x_offset = 1;
    const auto z_offset = 2;


    struct LMFunctor
    {

      const int n = 3;

      float m_x_res;
      long m_belt_width_n;

      Eigen::VectorXf belt_z;
      Eigen::VectorXf dp;
      
      LMFunctor() = delete;

      LMFunctor(float x_res, float z_res, long belt_width) : 
        m_x_res(x_res),
        m_belt_width_n(belt_width),
        dp(n)
      {
        dp(x_offset) = x_res;
        dp(belt_height) = z_res;
        dp(z_offset) = z_res;
      }

      // Difference between belt and belt model. Ie errors
      int operator()(const Eigen::VectorXf &parameters, Eigen::VectorXf &model_z)
      {

        belt_model(model_z,parameters(belt_height),parameters(x_offset),parameters(z_offset),m_belt_width_n,m_x_res);

        model_z = belt_z - model_z;

        return 0;
      }

      int df(const Eigen::VectorXf &parameters, Eigen::MatrixXf &jac)
      {
        Eigen::VectorXf belt_model_positive(values()), belt_model_negative(values());
        for (int i = 0; i < n; i++) {
          Eigen::VectorXf postive(parameters), negative(parameters);
          
          postive(i) += dp(i);
          negative(i) -= dp(i);

          operator()(postive, belt_model_positive);
          operator()(negative, belt_model_negative);

          jac.block(0, i, values(), 1) = (belt_model_positive - belt_model_negative) / (2.0f * dp(i));
        }

        return 0;
      }

      int values() const { return (int)belt_z.rows(); }

    };


    Eigen::VectorXf parameters(3);
    parameters(belt_height) = init_height;     
    parameters(x_offset) = init_x_offset;  
    parameters(z_offset) = init_z_offset;

  LMFunctor functor((float)x_res,(float)z_res,width_n);
  
  return [=](std::vector<float> z) mutable -> std::tuple<double,double,double> {
    auto f = z | std::views::filter([](float a) { return !std::isnan(a);});
    z = decltype(z)(f.begin(),f.end());
    functor.belt_z = Eigen::Map<Eigen::VectorXf>(z.data(), z.size());

    Eigen::LevenbergMarquardt<LMFunctor, float> lm(functor);
    lm.minimize(parameters);
    return {(double)parameters(belt_height),(double)parameters(x_offset),(double)parameters(z_offset)};
  };

  }



std::function<double(std::vector<float>)>  mk_pulleyfitter(float init_z, float z_res) {
    
    const auto z_index = 0;


    struct LMFunctor
    {

      const int n = 1;

      Eigen::VectorXf belt_z;
      Eigen::VectorXf dp;
     
			LMFunctor() = delete;
 
      LMFunctor(float z_res) : 
        dp(n)
      {
        dp(z_index) = z_res;
      }

      // Difference between belt and belt model. Ie errors
      int operator()(const Eigen::VectorXf &parameters, Eigen::VectorXf &model_z)
      {

        model_z.fill(parameters(z_index));

        model_z = (belt_z - model_z).array().abs().sqrt();

        return 0;
      }

      int df(const Eigen::VectorXf &parameters, Eigen::MatrixXf &jac)
      {
        
				Eigen::VectorXf belt_model_positive(values()), belt_model_negative(values());
        
				for (int i = 0; i < n; i++) {
          Eigen::VectorXf postive(parameters), negative(parameters);
          
          postive(i) += dp(i);
          negative(i) -= dp(i);

          operator()(postive, belt_model_positive);
          operator()(negative, belt_model_negative);

          jac.block(0, i, values(), 1) = (belt_model_positive - belt_model_negative) / (2.0f * dp(i));
        }

        return 0;
      }

      int values() const { return (int)belt_z.rows(); }

    };


    Eigen::VectorXf parameters(1);
    parameters(z_index) = init_z;

  LMFunctor functor(z_res);
  
  return [=](std::vector<float> z) mutable -> double {
    auto f = z | std::views::filter([](float a) { return !std::isnan(a);});
    z = decltype(z)(f.begin(),f.end());
    functor.belt_z = Eigen::Map<Eigen::VectorXf>(z.data(), z.size());

    Eigen::LevenbergMarquardt<LMFunctor, float> lm(functor);
    lm.minimize(parameters);
    return (double)parameters(z_index);
  };

}



}
