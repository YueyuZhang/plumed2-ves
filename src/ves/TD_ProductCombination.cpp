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
#include "core/ActionSet.h"
#include "core/PlumedMain.h"
#include "tools/Grid.h"

#include "GridIntegrationWeights.h"


namespace PLMD {

// class Grid;
class Action;

namespace ves {

//+PLUMEDOC VES_TARGETDIST PRODUCT_COMBINATION
/*
Target distribution given by product combination of distributions (static or dynamic).

Employ a target distribution that is a product combination of the other
distributions, defined as
\f[
p(\mathbf{s}) =
\frac{\prod_{i} p_{i}(\mathbf{s})}
{\int d \mathbf{s} \prod_{i} p_{i}(\mathbf{s})}
\f]
where the distributions \f$p_{i}(\mathbf{s})\f$ are in full dimensional space
of the arguments used.

Note the difference between this target distribution and the one defined in
\ref PRODUCT_DISTRIBUTION. Here we have a non-separable distribution given
as a product of distribution \f$p_{i}(\mathbf{s})\f$ which are in full dimensional
space of the arguments used.

The distributions \f$p_{i}(\mathbf{s})\f$ are given by using a separate numbered
DISTRIBUTION keyword for each distribution. The keywords for each distribution
should be enclosed within curly brackets.

The target distribution resulting from the product combination will be
automatically normalized. Therefore, the product combination needs to
be a proper distribution that is non-negative and normalizable. The
code will perform checks to make sure that this is indeed the case.

The product combination will be a dynamic target distribution if one or more
of the distributions used is a dynamic distribution. Otherwise it will be a
static distribution.

\par Examples

In the following example the overall interval on which the
target distribution is defined is from -4 to 4.
We employ a product of a Gaussian distribution with two centers
and distribution that is uniform on the interval -3 to 3 and
then smoothly decays to zero outside that interval.
The overall effect will then be to cut off the tails of the
Gaussian distribution
\plumedfile
TARGET_DISTRIBUTION={PRODUCT_COMBINATION
                     DISTRIBUTION1={GAUSSIAN
                                   CENTER1=-2.9 SIGMA1=1.0
                                   CENTER2=+2.9 SIGMA2=0.4}
                     DISTRIBUTION2={UNIFORM
                                    MINIMA=-3.0 SIGMA_MINIMA=0.20
                                    MAXIMA=+3.0 SIGMA_MAXIMA=0.15}}
\endplumedfile

*/
//+ENDPLUMEDOC

class VesBias;

class TD_ProductCombination: public TargetDistribution {
private:
  std::vector<TargetDistribution*> distribution_pntrs_;
  std::vector<Grid*> grid_pntrs_;
  unsigned int ndist_;
  void setupAdditionalGrids(const std::vector<Value*>&, const std::vector<std::string>&, const std::vector<std::string>&, const std::vector<unsigned int>&);
public:
  static void registerKeywords(Keywords&);
  explicit TD_ProductCombination(const ActionOptions& ao);
  void updateGrid();
  double getValue(const std::vector<double>&) const;
  //
  void linkVesBias(VesBias*);
  void linkAction(Action*);
  //
  void linkBiasGrid(Grid*);
  void linkBiasWithoutCutoffGrid(Grid*);
  void linkFesGrid(Grid*);
  //
};


PLUMED_REGISTER_ACTION(TD_ProductCombination,"PRODUCT_COMBINATION")


void TD_ProductCombination::registerKeywords(Keywords& keys) {
  TargetDistribution::registerKeywords(keys);
  keys.add("numbered","DISTRIBUTION","The target distributions to be used in the product combination, each given within a separate numbered DISTRIBUTION keyword and enclosed in curly brackets {}.");
  keys.use("BIAS_CUTOFF");
  keys.use("WELLTEMPERED_FACTOR");
  keys.use("SHIFT_TO_ZERO");
}


TD_ProductCombination::TD_ProductCombination(const ActionOptions& ao):
  PLUMED_VES_TARGETDISTRIBUTION_INIT(ao),
  distribution_pntrs_(0),
  grid_pntrs_(0),
  ndist_(0)
{
  for(unsigned int i=1;; i++) {
    std::string targetdist_label;
    if(!parseNumbered("DISTRIBUTION",i,targetdist_label) ) {break;}
    TargetDistribution* dist_pntr_tmp = plumed.getActionSet().selectWithLabel<TargetDistribution*>(targetdist_label);
    plumed_massert(dist_pntr_tmp!=NULL,"target distribution "+targetdist_label+" does not exist. NOTE: the target distribution should always be defined BEFORE the " + getName() + " action.");
    //
    if(dist_pntr_tmp->isDynamic()) {setDynamic();}
    if(dist_pntr_tmp->fesGridNeeded()) {setFesGridNeeded();}
    if(dist_pntr_tmp->biasGridNeeded()) {setBiasGridNeeded();}
    distribution_pntrs_.push_back(dist_pntr_tmp);
  }
  ndist_ = distribution_pntrs_.size();
  grid_pntrs_.assign(ndist_,NULL);
  //
  checkRead();
}


double TD_ProductCombination::getValue(const std::vector<double>& argument) const {
  plumed_merror("getValue not implemented for TD_ProductCombination");
  return 0.0;
}


void TD_ProductCombination::setupAdditionalGrids(const std::vector<Value*>& arguments, const std::vector<std::string>& min, const std::vector<std::string>& max, const std::vector<unsigned int>& nbins) {
  for(unsigned int i=0; i<ndist_; i++) {
    distribution_pntrs_[i]->setupGrids(arguments,min,max,nbins);
    if(distribution_pntrs_[i]->getDimension()!=this->getDimension()) {
      plumed_merror(getName() + ": all target distribution must have the same dimension");
    }
    grid_pntrs_[i]=distribution_pntrs_[i]->getTargetDistGridPntr();
  }
}


void TD_ProductCombination::updateGrid() {
  for(unsigned int i=0; i<ndist_; i++) {
    distribution_pntrs_[i]->updateTargetDist();
  }
  std::vector<double> integration_weights = GridIntegrationWeights::getIntegrationWeights(getTargetDistGridPntr());
  double norm = 0.0;
  for(Grid::index_t l=0; l<targetDistGrid().getSize(); l++) {
    double value = 1.0;
    for(unsigned int i=0; i<ndist_; i++) {
      value *= grid_pntrs_[i]->getValue(l);
    }
    if(value<0.0 && !isTargetDistGridShiftedToZero()) {plumed_merror(getName()+": The target distribution function gives negative values. You should change the definition of the target distribution to avoid this. You can also use the SHIFT_TO_ZERO keyword to avoid this problem.");}
    norm += integration_weights[l]*value;
    targetDistGrid().setValue(l,value);
    logTargetDistGrid().setValue(l,-std::log(value));
  }

  if(norm>0.0) {
    targetDistGrid().scaleAllValuesAndDerivatives(1.0/norm);
  }
  else if(!isTargetDistGridShiftedToZero()) {
    plumed_merror(getName()+": The target distribution function cannot be normalized proberly. You should change the definition of the target distribution to avoid this. You can also use the SHIFT_TO_ZERO keyword to avoid this problem.");
  }
  logTargetDistGrid().setMinToZero();
}


void TD_ProductCombination::linkVesBias(VesBias* vesbias_pntr_in) {
  TargetDistribution::linkVesBias(vesbias_pntr_in);
  for(unsigned int i=0; i<ndist_; i++) {
    distribution_pntrs_[i]->linkVesBias(vesbias_pntr_in);
  }
}


void TD_ProductCombination::linkAction(Action* action_pntr_in) {
  TargetDistribution::linkAction(action_pntr_in);
  for(unsigned int i=0; i<ndist_; i++) {
    distribution_pntrs_[i]->linkAction(action_pntr_in);
  }
}


void TD_ProductCombination::linkBiasGrid(Grid* bias_grid_pntr_in) {
  TargetDistribution::linkBiasGrid(bias_grid_pntr_in);
  for(unsigned int i=0; i<ndist_; i++) {
    distribution_pntrs_[i]->linkBiasGrid(bias_grid_pntr_in);
  }
}


void TD_ProductCombination::linkBiasWithoutCutoffGrid(Grid* bias_withoutcutoff_grid_pntr_in) {
  TargetDistribution::linkBiasWithoutCutoffGrid(bias_withoutcutoff_grid_pntr_in);
  for(unsigned int i=0; i<ndist_; i++) {
    distribution_pntrs_[i]->linkBiasWithoutCutoffGrid(bias_withoutcutoff_grid_pntr_in);
  }
}


void TD_ProductCombination::linkFesGrid(Grid* fes_grid_pntr_in) {
  TargetDistribution::linkFesGrid(fes_grid_pntr_in);
  for(unsigned int i=0; i<ndist_; i++) {
    distribution_pntrs_[i]->linkFesGrid(fes_grid_pntr_in);
  }
}


}
}
