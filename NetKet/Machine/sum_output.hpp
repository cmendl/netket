// Copyright 2018 The Simons Foundation, Inc. - All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef NETKET_SUMOUTPUT_HH
#define NETKET_SUMOUTPUT_HH

#include <time.h>
#include <Eigen/Dense>
#include <algorithm>
#include <complex>
#include <fstream>
#include <random>
#include <vector>
#include "Lookup/lookup.hpp"
#include "Utils/all_utils.hpp"
#include "abstract_layer.hpp"

namespace netket {

template <typename T>
class SumOutput : public AbstractLayer<T> {
  using VectorType = typename AbstractLayer<T>::VectorType;
  using MatrixType = typename AbstractLayer<T>::MatrixType;

  int in_size_;   // input size: should be multiple of no. of sites
  int out_size_;  // output size: should be multiple of no. of sites

  VectorType z_;  // Linear term, z = W' * in + b

  // Note that input of this layer is also the output of
  // previous layer

 public:
  using StateType = typename AbstractLayer<T>::StateType;
  using LookupType = typename AbstractLayer<T>::LookupType;

  /// Constructor
  explicit SumOutput(const json &pars) {
    in_size_ = FieldVal(pars, "Inputs");

    out_size_ = 1;

    Init();
  }

  void Init() {
    z_.resize(out_size_);

    std::string buffer = "";
    InfoMessage(buffer) << "Sum Output Layer: " << in_size_ << " --> "
                        << out_size_ << std::endl;
  }

  void InitRandomPars(int /*seed*/, double /*sigma*/) override {}

  int Npar() const override { return 0; }

  int Ninput() const override { return in_size_; }

  int Noutput() const override { return out_size_; }

  void GetParameters(VectorType & /*pars*/, int /*start_idx*/) const override {}

  void SetParameters(const VectorType & /*pars*/, int /*start_idx*/) override {}

  void UpdateLookup(VectorType &oldconf, const std::vector<int> &tochange,
                    const VectorType &newconf, VectorType &theta) override {
    if (int(tochange.size()) == in_size_) {
      LinearTransformation(newconf, theta);
    } else {
      UpdateTheta(oldconf, tochange, newconf, theta);
    }
    UpdateConf(tochange, newconf, oldconf);
  }

  void UpdateLookup(const Eigen::VectorXd &v, const std::vector<int> &tochange,
                    const std::vector<double> &newconf,
                    VectorType &theta) override {
    UpdateTheta(v, tochange, newconf, theta);
  }

  void NextConf(const VectorType &theta, const std::vector<int> & /*tochange*/,
                std::vector<int> & /*tochange1*/,
                VectorType &newconf1) override {
    newconf1.noalias() = theta;
  }

  void UpdateConf(const std::vector<int> &tochange, const VectorType &newconf,
                  VectorType &v) override {
    const int num_of_changes = tochange.size();

    if (num_of_changes == in_size_) {
      v.noalias() = newconf;
    } else {
      for (int s = 0; s < num_of_changes; s++) {
        const int sf = tochange[s];
        v(sf) = newconf(s);
      }
    }
  }

  void Forward(const VectorType &prev_layer_output, VectorType &theta,
               VectorType &output) override {
    LinearTransformation(prev_layer_output, theta);
    NonLinearTransformation(theta, output);
  }

  // Using lookup
  void Forward(const VectorType &theta, VectorType &output) override {
    // Apply activation function
    NonLinearTransformation(theta, output);
  }

  inline void LinearTransformation(const VectorType &input, VectorType &theta) {
    theta(0) = input.sum();
  }

  inline void NonLinearTransformation(const VectorType &theta,
                                      VectorType &output) {
    output(0) = theta(0);
  }

  inline void UpdateTheta(VectorType &oldconf, const std::vector<int> &tochange,
                          const VectorType &newconf, VectorType &theta) {
    const int num_of_changes = tochange.size();
    for (int s = 0; s < num_of_changes; s++) {
      const int sf = tochange[s];
      theta(0) += (newconf(s) - oldconf(sf));
    }
  }

  inline void UpdateTheta(const VectorType &oldconf,
                          const std::vector<int> &tochange,
                          const std::vector<double> &newconf,
                          VectorType &theta) {
    const int num_of_changes = tochange.size();
    for (int s = 0; s < num_of_changes; s++) {
      const int sf = tochange[s];
      theta(0) += (newconf[s] - oldconf(sf));
    }
  }

  void Backprop(const VectorType & /*prev_layer_output*/,
                const VectorType & /*this_layer_output*/,
                const VectorType & /*this_layer_theta*/,
                const VectorType &next_layer_data, VectorType &din,
                VectorType & /*der*/, int /*start_idx*/) override {
    din.resize(in_size_);
    din.setConstant(next_layer_data(0));
  }

  void to_json(json &pars) const override {
    json layerpar;
    layerpar["Name"] = "Sum";
    layerpar["Inputs"] = in_size_;
    layerpar["Outputs"] = out_size_;

    pars["Machine"]["Layers"].push_back(layerpar);
  }

  void from_json(const json & /*j*/) override {}
};
}  // namespace netket

#endif
