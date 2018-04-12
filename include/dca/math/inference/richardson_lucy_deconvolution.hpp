// Copyright (C) 2009-2016 ETH Zurich
// Copyright (C) 2007?-2016 Center for Nanophase Materials Sciences, ORNL
// All rights reserved.
//
// See LICENSE.txt for terms of usage.
// See CITATION.txt for citation guidelines if you use this code for scientific publications.
//
// Author: Peter Staar (taa@zurich.ibm.com)
//         Urs R. Haehner (haehneru@itp.phys.ethz.ch)
//
// This class implements the Richardson Lucy deconvolution algorithm.

#ifndef DCA_MATH_INFERENCE_RICHARDSON_LUCY_DECONVOLUTION_HPP
#define DCA_MATH_INFERENCE_RICHARDSON_LUCY_DECONVOLUTION_HPP

#include <cassert>
#include <cmath>
#include <iostream>
#include <utility>

#include "dca/function/domains.hpp"
#include "dca/function/function.hpp"
#include "dca/linalg/matrix.hpp"
#include "dca/linalg/matrixop.hpp"

namespace dca {
namespace math {
namespace inference {
// dca::math::inference::

template <typename ClusterDmn, typename HostDmn, typename OtherDmn>
class RichardsonLucyDeconvolution {
public:
  RichardsonLucyDeconvolution(const linalg::Matrix<double, linalg::CPU>& p_cluster,
                              const linalg::Matrix<double, linalg::CPU>& p_host,
                              const double tolerance, const int max_iterations);

  // Returns the number of iterations executed.
  int findTargetFunction(
      const func::function<double, func::dmn_variadic<ClusterDmn, OtherDmn>>& source,
      const func::function<double, func::dmn_variadic<HostDmn, OtherDmn>>& source_interpolated,
      func::function<double, func::dmn_variadic<HostDmn, OtherDmn>>& target);

  // Returns the number of iterations executed.
  int findTargetFunction(
      const func::function<double, func::dmn_variadic<ClusterDmn, OtherDmn>>& source,
      const func::function<double, func::dmn_variadic<HostDmn, OtherDmn>>& source_interpolated,
      func::function<double, func::dmn_variadic<HostDmn, OtherDmn>>& target,
      func::function<double, func::dmn_variadic<HostDmn, OtherDmn>>& target_convoluted);

private:
  void initializeMatrices(
      const func::function<double, func::dmn_variadic<HostDmn, OtherDmn>>& source_interpolated);

  bool finished(const func::function<double, func::dmn_variadic<ClusterDmn, OtherDmn>>& source,
                func::function<double, func::dmn_variadic<HostDmn, OtherDmn>>& target);

private:
  const double tolerance_;
  const int max_iterations_;

  const linalg::Matrix<double, linalg::CPU>& p_cluster_;
  const linalg::Matrix<double, linalg::CPU>& p_host_;

  linalg::Matrix<double, linalg::CPU> c_cluster_;

  linalg::Matrix<double, linalg::CPU> c_;
  linalg::Matrix<double, linalg::CPU> d_;
  linalg::Matrix<double, linalg::CPU> d_over_c_;
  linalg::Matrix<double, linalg::CPU> u_t_;
  linalg::Matrix<double, linalg::CPU> u_t_plus_1_;

  func::function<bool, OtherDmn> is_finished_;
};

template <typename ClusterDmn, typename HostDmn, typename OtherDmn>
RichardsonLucyDeconvolution<ClusterDmn, HostDmn, OtherDmn>::RichardsonLucyDeconvolution(
    const linalg::Matrix<double, linalg::CPU>& p_cluster,
    const linalg::Matrix<double, linalg::CPU>& p_host, const double tolerance,
    const int max_iterations)
    : tolerance_(tolerance),
      max_iterations_(max_iterations),

      p_cluster_(p_cluster),
      p_host_(p_host),

      c_cluster_("c-cluster (Richardson-Lucy-deconvolution)"),

      c_("c (Richardson-Lucy-deconvolution)"),
      d_("d (Richardson-Lucy-deconvolution)"),
      d_over_c_("d/c (Richardson-Lucy-deconvolution)"),
      u_t_("u_t (Richardson-Lucy-deconvolution)"),
      u_t_plus_1_("u_{t+1} (Richardson-Lucy-deconvolution)"),

      is_finished_("is_finished") {
  if (p_host_.size().first != HostDmn::dmn_size() || p_host_.size().second != HostDmn::dmn_size() ||
      p_cluster_.size().first != ClusterDmn::dmn_size() ||
      p_cluster_.size().second != HostDmn::dmn_size())
    std::logic_error("Projection operator dimensions do not match domain sizes.");
}

template <typename ClusterDmn, typename HostDmn, typename OtherDmn>
int RichardsonLucyDeconvolution<ClusterDmn, HostDmn, OtherDmn>::findTargetFunction(
    const func::function<double, func::dmn_variadic<ClusterDmn, OtherDmn>>& source,
    const func::function<double, func::dmn_variadic<HostDmn, OtherDmn>>& source_interpolated,
    func::function<double, func::dmn_variadic<HostDmn, OtherDmn>>& target) {
  for (int i = 0; i < OtherDmn::dmn_size(); ++i)
    is_finished_(i) = false;

  initializeMatrices(source_interpolated);

  int iterations = 0;
  while (!finished(source, target) && iterations < max_iterations_) {
    // Compute c.
    linalg::matrixop::gemm(p_host_, u_t_, c_);

    // Compute d_over_c.
    for (int j = 0; j < OtherDmn::dmn_size(); ++j)
      for (int i = 0; i < HostDmn::dmn_size(); ++i)
        d_over_c_(i, j) = d_(i, j) / c_(i, j);

    // Compute u_{t+1}.
    linalg::matrixop::gemm('T', 'N', p_host_, d_over_c_, u_t_plus_1_);

    for (int j = 0; j < OtherDmn::dmn_size(); ++j)
      for (int i = 0; i < HostDmn::dmn_size(); ++i)
        u_t_(i, j) = u_t_plus_1_(i, j) * u_t_(i, j);

    ++iterations;
  }

  // Copy iterative solution into returned target function for all OtherDmn indices that have not
  // finished.
  for (int j = 0; j < OtherDmn::dmn_size(); ++j)
    if (!is_finished_(j))
      for (int i = 0; i < HostDmn::dmn_size(); ++i)
        target(i, j) = u_t_(i, j);

  return iterations;
}

template <typename ClusterDmn, typename HostDmn, typename OtherDmn>
int RichardsonLucyDeconvolution<ClusterDmn, HostDmn, OtherDmn>::findTargetFunction(
    const func::function<double, func::dmn_variadic<ClusterDmn, OtherDmn>>& source,
    const func::function<double, func::dmn_variadic<HostDmn, OtherDmn>>& source_interpolated,
    func::function<double, func::dmn_variadic<HostDmn, OtherDmn>>& target,
    func::function<double, func::dmn_variadic<HostDmn, OtherDmn>>& target_convoluted) {
  const int iterations = findTargetFunction(source, source_interpolated, target);

  // Compute the convolution of the target function, which should resemble the interpolated source
  // function.
  for (int j = 0; j < OtherDmn::dmn_size(); j++)
    for (int i = 0; i < HostDmn::dmn_size(); i++)
      u_t_(i, j) = target(i, j);

  linalg::matrixop::gemm(p_host_, u_t_, c_);

  for (int j = 0; j < OtherDmn::dmn_size(); j++)
    for (int i = 0; i < HostDmn::dmn_size(); i++)
      target_convoluted(i, j) = c_(i, j);

  return iterations;
}

template <typename ClusterDmn, typename HostDmn, typename OtherDmn>
void RichardsonLucyDeconvolution<ClusterDmn, HostDmn, OtherDmn>::initializeMatrices(
    const func::function<double, func::dmn_variadic<HostDmn, OtherDmn>>& source_interpolated) {
  const int num_rows_host = HostDmn::dmn_size();
  const int num_rows_cluster = ClusterDmn::dmn_size();
  const int num_cols = OtherDmn::dmn_size();

  c_cluster_.resizeNoCopy(std::make_pair(num_rows_cluster, num_cols));

  c_.resizeNoCopy(std::make_pair(num_rows_host, num_cols));
  d_.resizeNoCopy(std::make_pair(num_rows_host, num_cols));
  d_over_c_.resizeNoCopy(std::make_pair(num_rows_host, num_cols));
  u_t_.resizeNoCopy(std::make_pair(num_rows_host, num_cols));
  u_t_plus_1_.resizeNoCopy(std::make_pair(num_rows_host, num_cols));

  // Initialize d matrix ("observed image") with interpolated source function.
  for (int j = 0; j < num_cols; ++j)
    for (int i = 0; i < num_rows_host; ++i)
      d_(i, j) = source_interpolated(i, j);

  // Initialize iterative solution u_t with signs of column means of d.
  for (int j = 0; j < num_cols; ++j) {
    double mean = 0.;
    for (int i = 0; i < num_rows_host; ++i)
      mean += d_(i, j);
    mean /= num_rows_host;

    for (int i = 0; i < num_rows_host; ++i)
      u_t_(i, j) = mean / std::abs(mean);  // Used to be: u_t_(i,j) = mean.
  }

  // Initialize the other matrices with zero.
  for (int j = 0; j < num_cols; ++j)
    for (int i = 0; i < num_rows_host; ++i)
      c_(i, j) = d_over_c_(i, j) = u_t_plus_1_(i, j) = 0.;

  for (int j = 0; j < num_cols; ++j)
    for (int i = 0; i < num_rows_cluster; ++i)
      c_cluster_(i, j) = 0.;
}

template <typename ClusterDmn, typename HostDmn, typename OtherDmn>
bool RichardsonLucyDeconvolution<ClusterDmn, HostDmn, OtherDmn>::finished(
    const func::function<double, func::dmn_variadic<ClusterDmn, OtherDmn>>& source,
    func::function<double, func::dmn_variadic<HostDmn, OtherDmn>>& target) {
  bool all_finished = true;

  // Convolute iterative solution to cluster domain and compare with original source.
  linalg::matrixop::gemm(p_cluster_, u_t_, c_cluster_);

  for (int j = 0; j < OtherDmn::dmn_size(); ++j) {
    if (!is_finished_(j)) {
      // Compute relative L2 error.
      double diff_squared = 0.;
      double norm_source_squared = 0.;

      for (int i = 0; i < ClusterDmn::dmn_size(); ++i) {
        diff_squared += std::pow(c_cluster_(i, j) - source(i, j), 2);
        norm_source_squared += std::pow(source(i, j), 2);
      }

      const double error = std::sqrt(diff_squared / norm_source_squared);

      if (error < tolerance_) {
        // Copy iterative solution into returned target function.
        for (int i = 0; i < HostDmn::dmn_size(); ++i)
          target(i, j) = u_t_(i, j);

        is_finished_(j) = true;
      }

      else
        all_finished = false;
    }
  }

  return all_finished;
}

}  // inference
}  // math
}  // dca

#endif  // DCA_MATH_INFERENCE_RICHARDSON_LUCY_DECONVOLUTION_HPP
