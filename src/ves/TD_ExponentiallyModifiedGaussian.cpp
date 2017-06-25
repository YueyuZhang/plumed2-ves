/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2016-2017 The ves-code team
   (see the PEOPLE-VES file at the root of this folder for a list of names)

   See http://www.ves-code.org for more information.

   This file is part of ves-code, version 1.

   ves-code is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ves-code is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with ves-code.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#include "TargetDistribution.h"

#include "core/ActionRegister.h"


namespace PLMD {
namespace ves {

//+PLUMEDOC VES_TARGETDIST_HIDDEN TD_EXPONENTIALLY_MODIFIED_GAUSSIAN
/*
Target distribution given by a sum of exponentially modified Gaussian distributions (static).

\par Examples

*/
//+ENDPLUMEDOC

class TD_ExponentiallyModifiedGaussian: public TargetDistribution {
  std::vector< std::vector<double> > centers_;
  std::vector< std::vector<double> > sigmas_;
  std::vector< std::vector<double> > lambdas_;
  std::vector<double> weights_;
  unsigned int ncenters_;
  double ExponentiallyModifiedGaussianDiagonal(const std::vector<double>&, const std::vector<double>&, const std::vector<double>&, const std::vector<double>&) const;
public:
  static void registerKeywords(Keywords&);
  explicit TD_ExponentiallyModifiedGaussian(const ActionOptions& ao);
  double getValue(const std::vector<double>&) const;
};


PLUMED_REGISTER_ACTION(TD_ExponentiallyModifiedGaussian,"TD_EXPONENTIALLY_MODIFIED_GAUSSIAN")


void TD_ExponentiallyModifiedGaussian::registerKeywords(Keywords& keys) {
  TargetDistribution::registerKeywords(keys);
  keys.add("numbered","CENTER","The center of each exponentially modified Gaussian distributions.");
  keys.add("numbered","SIGMA","The sigma parameters for each exponentially modified Gaussian distributions.");
  keys.add("numbered","LAMBDA","The lambda parameters for each exponentially modified Gaussian distributions");
  keys.add("optional","WEIGHTS","The weights of the distributions. By default all are weighted equally.");
  keys.use("WELLTEMPERED_FACTOR");
  keys.use("SHIFT_TO_ZERO");
  keys.use("NORMALIZE");
}


TD_ExponentiallyModifiedGaussian::TD_ExponentiallyModifiedGaussian(const ActionOptions& ao):
  PLUMED_VES_TARGETDISTRIBUTION_INIT(ao),
  centers_(0),
  sigmas_(0),
  lambdas_(0),
  weights_(0),
  ncenters_(0)
{
  for(unsigned int i=1;; i++) {
    std::vector<double> tmp_center;
    if(!parseNumberedVector("CENTER",i,tmp_center) ) {break;}
    centers_.push_back(tmp_center);
  }
  for(unsigned int i=1;; i++) {
    std::vector<double> tmp_sigma;
    if(!parseNumberedVector("SIGMA",i,tmp_sigma) ) {break;}
    for(unsigned int k=0; k<tmp_sigma.size(); k++) {
      if(tmp_sigma[k]<=0.0) {plumed_merror(getName()+": the values given in SIGMA should be postive");}
    }
    sigmas_.push_back(tmp_sigma);
  }
  for(unsigned int i=1;; i++) {
    std::vector<double> tmp_lambda;
    if(!parseNumberedVector("LAMBDA",i,tmp_lambda) ) {break;}
    for(unsigned int k=0; k<tmp_lambda.size(); k++) {
      if(tmp_lambda[k]<=0.0) {plumed_merror(getName()+": the values given in LAMBDA should be postive");}
    }
    lambdas_.push_back(tmp_lambda);
  }
  //
  if(centers_.size()==0) {
    plumed_merror(getName()+": CENTER keywords seem to be missing. Note that numbered keywords start at CENTER1.");
  }
  //
  if(centers_.size()!=sigmas_.size() || centers_.size()!=lambdas_.size() ) {
    plumed_merror(getName()+": there has to be an equal amount of CENTER, SIGMA, and LAMBDA keywords");
  }
  //
  setDimension(centers_[0].size());
  ncenters_ = centers_.size();
  //
  // check centers and sigmas
  for(unsigned int i=0; i<ncenters_; i++) {
    if(centers_[i].size()!=getDimension()) {
      plumed_merror(getName()+": one of the CENTER keyword does not match the given dimension");
    }
    if(sigmas_[i].size()!=getDimension()) {
      plumed_merror(getName()+": one of the SIGMA keyword does not match the given dimension");
    }
    if(lambdas_[i].size()!=getDimension()) {
      plumed_merror(getName()+": one of the LAMBDA keyword does not match the given dimension");
    }
  }
  //
  parseVector("WEIGHTS",weights_);
  if(weights_.size()==0) {weights_.assign(centers_.size(),1.0);}
  if(centers_.size()!=weights_.size()) {
    plumed_merror(getName()+": there has to be as many weights given in WEIGHTS as numbered CENTER keywords");
  }
  //
  double sum_weights=0.0;
  for(unsigned int i=0; i<weights_.size(); i++) {sum_weights+=weights_[i];}
  for(unsigned int i=0; i<weights_.size(); i++) {weights_[i]/=sum_weights;}
  //
  checkRead();
}


double TD_ExponentiallyModifiedGaussian::getValue(const std::vector<double>& argument) const {
  double value=0.0;
  for(unsigned int i=0; i<ncenters_; i++) {
    value+=weights_[i]*ExponentiallyModifiedGaussianDiagonal(argument,centers_[i],sigmas_[i],lambdas_[i]);
  }
  return value;
}


double TD_ExponentiallyModifiedGaussian::ExponentiallyModifiedGaussianDiagonal(const std::vector<double>& argument, const std::vector<double>& center, const std::vector<double>& sigma, const std::vector<double>& lambda) const {
  double value = 1.0;
  for(unsigned int k=0; k<argument.size(); k++) {
    double arg1 = 0.5*lambda[k]*(2.0*center[k]+lambda[k]*sigma[k]*sigma[k]-2.0*argument[k]);
    double arg2 = (center[k]+lambda[k]*sigma[k]*sigma[k]-argument[k])/(sqrt(2.0)*sigma[k]);
    value *= 0.5*lambda[k]*exp(arg1)*erfc(arg2);
  }
  return value;
}



}
}