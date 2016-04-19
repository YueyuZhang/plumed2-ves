/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2015-2016 The ves-code team
   (see the PEOPLE-VES file at the root of the distribution for a list of names)

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
#include "VesBias.h"
#include "LinearBasisSetExpansion.h"
#include "ves_tools/CoeffsVector.h"
#include "ves_tools/CoeffsMatrix.h"
#include "ves_basisfunctions/BasisFunctions.h"
#include "ves_optimizers/Optimizer.h"

#include "bias/Bias.h"
#include "core/ActionRegister.h"
#include "core/ActionSet.h"
#include "core/PlumedMain.h"



using namespace std;


namespace PLMD{
namespace bias{

//+PLUMEDOC BIAS MOVINGRESTRAINT
/*

*/
//+ENDPLUMEDOC


class VesLinearExpansion : public VesBias{
private:
  unsigned int nargs_;
  std::vector<BasisFunctions*> basisf_pntrs_;
  LinearBasisSetExpansion* bias_expansion_pntr_;
  size_t ncoeffs_;
  Value* valueBias_;
  Value* valueForce2_;
public:
  explicit VesLinearExpansion(const ActionOptions&);
  ~VesLinearExpansion();
  void calculate();
  void updateTargetDistributions();
  void setupBiasFileOutput();
  void writeBiasToFile();
  void setupFesFileOutput();
  void writeFesToFile();
  void setupTargetDistFileOutput() {};
  void writeTargetDistToFile();
  static void registerKeywords( Keywords& keys );
};

PLUMED_REGISTER_ACTION(VesLinearExpansion,"VES_LINEAR_EXPANSION")

void VesLinearExpansion::registerKeywords( Keywords& keys ){
  VesBias::registerKeywords(keys);
  keys.use("ARG");
  keys.add("compulsory","BASIS_FUNCTIONS","the label of the basis sets that you want to use");
  VesBias::useInitialCoeffsKeywords(keys);
  VesBias::useTargetDistributionKeywords(keys);
  VesBias::useWellTemperdKeywords(keys);
  VesBias::useBiasCutoffKeywords(keys);
  VesBias::useGridBinKeywords(keys);
}

VesLinearExpansion::VesLinearExpansion(const ActionOptions&ao):
PLUMED_VESBIAS_INIT(ao),
nargs_(getNumberOfArguments()),
basisf_pntrs_(getNumberOfArguments(),NULL),
bias_expansion_pntr_(NULL),
valueBias_(NULL),
valueForce2_(NULL)
{
  std::vector<std::string> basisf_labels;
  parseMultipleValues("BASIS_FUNCTIONS",basisf_labels,nargs_);
  checkRead();

  for(unsigned int i=0; i<basisf_labels.size(); i++){
    basisf_pntrs_[i] = plumed.getActionSet().selectWithLabel<BasisFunctions*>(basisf_labels[i]);
    plumed_massert(basisf_pntrs_[i]!=NULL,"basis function "+basisf_labels[i]+" does not exist. NOTE: the basis functions should always be defined BEFORE the VES bias.");
  }

  //
  std::vector<Value*> args_pntrs = getArguments();
  addCoeffsSet(args_pntrs,basisf_pntrs_);
  ncoeffs_ = numberOfCoeffs();

  checkThatTemperatureIsGiven();
  bias_expansion_pntr_ = new LinearBasisSetExpansion(getLabel(),getBeta(),comm,args_pntrs,basisf_pntrs_,getCoeffsPntr());
  bias_expansion_pntr_->linkVesBias(this);
  bias_expansion_pntr_->setGridBins(this->getGridBins());
  //
  if(getNumberOfTargetDistributionKeywords()>0){
    if(getNumberOfTargetDistributionKeywords()!=1 && getNumberOfTargetDistributionKeywords()!=nargs_){
      plumed_merror("the number of target distribution keywords given by the TARGET_DISTRIBUTION keywords needs to be either 1 or equal to the number of arguments");
    }
    bias_expansion_pntr_->setupTargetDistribution(getTargetDistributionKeywords());
  }
  else{
    bias_expansion_pntr_->setupUniformTargetDistribution();
  }
  setCoeffsDerivsOverTargetDist(bias_expansion_pntr_->CoeffDerivsAverTargetDist());

  if(this->wellTemperdTargetDistribution()){
    bias_expansion_pntr_->setupWellTemperedTargetDistribution(this->getWellTemperedBiasFactor());
  }
  else if(this->biasCutoffActive()){
    bias_expansion_pntr_->setupBiasCutoffTargetDistribution();
  }

  writeCoeffDerivsAverTargetDistToFile();
  //
  readCoeffsFromFiles();
  //
  addComponent("bias"); componentIsNotPeriodic("bias");
  valueBias_=getPntrToComponent("bias");
  addComponent("force2"); componentIsNotPeriodic("force2");
  valueForce2_=getPntrToComponent("force2");
}


VesLinearExpansion::~VesLinearExpansion() {
  if(bias_expansion_pntr_!=NULL){
    delete bias_expansion_pntr_;
  }
}


void VesLinearExpansion::calculate() {

  std::vector<double> cv_values(nargs_);
  std::vector<double> forces(nargs_);
  std::vector<double> coeffsderivs_values(ncoeffs_);

  for(unsigned int k=0; k<nargs_; k++){
    cv_values[k]=getArgument(k);
  }

  double bias = bias_expansion_pntr_->getBiasAndForces(cv_values,forces,coeffsderivs_values);
  if(biasCutoffActive()){
    applyBiasCutoff(bias,forces,coeffsderivs_values);
    coeffsderivs_values[0]=1.0;
  }
  double totalForce2 = 0.0;
  for(unsigned int k=0; k<nargs_; k++){
    setOutputForce(k,forces[k]);
    totalForce2 += forces[k]*forces[k];
  }

  valueBias_->set(bias);
  valueForce2_->set(totalForce2);
  setCoeffsDerivs(coeffsderivs_values);
}


void VesLinearExpansion::updateTargetDistributions() {
  if(wellTemperdTargetDistribution()){
    bias_expansion_pntr_->updateWellTemperedTargetDistribution();
  }
  else if(biasCutoffActive()){
    bias_expansion_pntr_->updateBiasCutoffTargetDistribution();
  }
  setCoeffsDerivsOverTargetDist(bias_expansion_pntr_->CoeffDerivsAverTargetDist());
  // this output should be optional or with a stride
  writeCoeffDerivsAverTargetDistToFile(true,getOptimizerPntr()->getIterationCounter());
}


void VesLinearExpansion::setupBiasFileOutput() {
  bias_expansion_pntr_->setupBiasGrid(true);
}


void VesLinearExpansion::writeBiasToFile() {
  bias_expansion_pntr_->updateBiasGrid();
  bias_expansion_pntr_->writeBiasGridToFile(getCurrentBiasOutputFilename());
}


void VesLinearExpansion::setupFesFileOutput() {
  bias_expansion_pntr_->setupFesGrid();
}


void VesLinearExpansion::writeFesToFile() {
  bias_expansion_pntr_->updateFesGrid();
  bias_expansion_pntr_->writeFesGridToFile(getCurrentFesOutputFilename());
}


void VesLinearExpansion::writeTargetDistToFile() {
  bias_expansion_pntr_->writeTargetDistGridToFile(getCurrentTargetDistOutputFilename());
}


}
}
