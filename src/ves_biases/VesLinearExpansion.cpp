/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2011-2015 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed-code.org for more information.

   This file is part of plumed, version 2.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#include "VesBias.h"
#include "LinearBasisSetExpansion.h"
#include "ves_tools/CoeffsVector.h"
#include "ves_tools/CoeffsMatrix.h"
#include "ves_basisfunctions/BasisFunctions.h"

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
  std::vector<BasisFunctions*> basisf_pntrs;
  LinearBasisSetExpansion* bias_expansion_pntr;
  size_t ncoeffs_;
  Value* valueBias;
  Value* valueForce2;
public:
  explicit VesLinearExpansion(const ActionOptions&);
  ~VesLinearExpansion();
  void calculate();
  static void registerKeywords( Keywords& keys );
};

PLUMED_REGISTER_ACTION(VesLinearExpansion,"VES_LINEAR_EXPANSION")

void VesLinearExpansion::registerKeywords( Keywords& keys ){
  VesBias::registerKeywords(keys);
  keys.use("ARG");
  keys.add("compulsory","BASIS_FUNCTIONS","the label of the basis sets that you want to use");
  keys.use("COEFFS");
}

VesLinearExpansion::VesLinearExpansion(const ActionOptions&ao):
PLUMED_VESBIAS_INIT(ao),
nargs_(getNumberOfArguments()),
basisf_pntrs(getNumberOfArguments(),NULL),
bias_expansion_pntr(NULL)
{
  std::vector<std::string> basisf_labels;
  parseMultipleValues("BASIS_FUNCTIONS",basisf_labels,nargs_);
  checkRead();

  for(unsigned int i=0; i<basisf_labels.size(); i++){
    basisf_pntrs[i] = plumed.getActionSet().selectWithLabel<BasisFunctions*>(basisf_labels[i]);
    plumed_massert(basisf_pntrs[i]!=NULL,"basis function "+basisf_labels[i]+" does not exist. NOTE: the basis functions should always be defined BEFORE the VES bias.");
  }

  //
  std::vector<Value*> args_pntrs = getArguments();
  addCoeffsSet(args_pntrs,basisf_pntrs);
  ncoeffs_ = numberOfCoeffs();



  bias_expansion_pntr = new LinearBasisSetExpansion(getLabel(),comm,args_pntrs,basisf_pntrs,getCoeffsPntr());
  bias_expansion_pntr->linkVesBias(this);

  //
  bias_expansion_pntr->setupUniformTargetDistribution();
  setCoeffsDerivsOverTargetDist(bias_expansion_pntr->CoeffDerivsAverTargetDist());
  //
  readCoeffsFromFiles();
  //
  addComponent("bias"); componentIsNotPeriodic("bias");
  valueBias=getPntrToComponent("bias");
  addComponent("force2"); componentIsNotPeriodic("force2");
  valueForce2=getPntrToComponent("force2");
}


VesLinearExpansion::~VesLinearExpansion() {
  if(bias_expansion_pntr!=NULL){
    delete bias_expansion_pntr;
  }
}


void VesLinearExpansion::calculate() {

  std::vector<double> cv_values(nargs_);
  std::vector<double> forces(nargs_);
  std::vector<double> coeffsderivs_values(ncoeffs_);

  for(unsigned int k=0; k<nargs_; k++){
    cv_values[k]=getArgument(k);
  }

  double bias = bias_expansion_pntr->getBiasAndForces(cv_values,forces,coeffsderivs_values);
  valueBias->set(bias);

  double totalForce2 = 0.0;
  for(unsigned int k=0; k<nargs_; k++){
    setOutputForce(k,forces[k]);
    totalForce2 += forces[k]*forces[k];
  }

  valueForce2->set(totalForce2);
  setCoeffsDerivs(coeffsderivs_values);
}

}
}