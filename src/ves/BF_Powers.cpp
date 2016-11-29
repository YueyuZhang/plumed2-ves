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

#include "BasisFunctions.h"

#include "core/ActionRegister.h"


namespace PLMD{
namespace ves{

class BF_Powers : public BasisFunctions {
  double inv_normfactor_;
  virtual void setupLabels();
public:
  static void registerKeywords( Keywords&);
  explicit BF_Powers(const ActionOptions&);
  double getValue(const double, const unsigned int, double&, bool&) const;
  void getAllValues(const double, double&, bool&, std::vector<double>&, std::vector<double>&) const;
};


PLUMED_REGISTER_ACTION(BF_Powers,"BF_POWERS")

// See DOI 10.1007/s10614-007-9092-4 for more information;


void BF_Powers::registerKeywords(Keywords& keys){
  BasisFunctions::registerKeywords(keys);
  keys.add("optional","NORMALIZATION","the normalization factor that is used to normalize the basis functions by dividing the values. By default it is 1.0.");
}

BF_Powers::BF_Powers(const ActionOptions&ao):
PLUMED_BASISFUNCTIONS_INIT(ao)
{
  setNumberOfBasisFunctions(getOrder()+1);
  setIntrinsicInterval(intervalMin(),intervalMax());
  double normfactor_=1.0;
  parse("NORMALIZATION",normfactor_);
  inv_normfactor_=1.0/normfactor_;
  setNonPeriodic();
  setIntervalBounded();
  setType("polynom_powers");
  setDescription("Polynomial Powers");
  setupBF();
  log.printf("   normalization factor: %f\n",normfactor_);
  checkRead();
}


double BF_Powers::getValue(const double arg, const unsigned int n, double& argT, bool& inside_range) const {
  plumed_massert(n<numberOfBasisFunctions(),"getValue: n is outside range of the defined order of the basis set");
  inside_range=true;
  argT=checkIfArgumentInsideInterval(arg,inside_range);
  //
  if(n==0){
    return 1.0;
  }
  else{
    return pow(argT,static_cast<double>(n));
  }
}


void BF_Powers::getAllValues(const double arg, double& argT, bool& inside_range, std::vector<double>& values, std::vector<double>& derivs) const {
  inside_range=true;
  argT=checkIfArgumentInsideInterval(arg,inside_range);
  //
  values[0]=1.0;
  derivs[0]=0.0;
  //
  for(unsigned int i=1; i < getNumberOfBasisFunctions(); i++){
    double io = static_cast<double>(i);
    values[i] = pow(argT,io);
    derivs[i] = io*pow(argT,io-1.0);
  }
  if(!inside_range){for(unsigned int i=0;i<derivs.size();i++){derivs[i]=0.0;}}
}


void BF_Powers::setupLabels() {
  setLabel(0,"1");
  for(unsigned int i=1; i < getOrder()+1;i++){
    std::string is; Tools::convert(i,is);
    setLabel(i,"s^"+is);
  }
}

}
}