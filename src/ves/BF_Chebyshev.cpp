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

//+PLUMEDOC VES_BASISF BF_CHEBYSHEV
/*
Chebyshev basis functions

\par Examples

*/
//+ENDPLUMEDOC


class BF_Chebyshev : public BasisFunctions {
  virtual void setupUniformIntegrals();
public:
  static void registerKeywords(Keywords&);
  explicit BF_Chebyshev(const ActionOptions&);
  void getAllValues(const double, double&, bool&, std::vector<double>&, std::vector<double>&) const;
};


PLUMED_REGISTER_ACTION(BF_Chebyshev,"BF_CHEBYSHEV")


void BF_Chebyshev::registerKeywords(Keywords& keys){
 BasisFunctions::registerKeywords(keys);
}

BF_Chebyshev::BF_Chebyshev(const ActionOptions&ao):
 PLUMED_BASISFUNCTIONS_INIT(ao)
{
  setNumberOfBasisFunctions(getOrder()+1);
  setIntrinsicInterval("-1.0","+1.0");
  setNonPeriodic();
  setIntervalBounded();
  setType("chebyshev-1st-kind");
  setDescription("Chebyshev polynomials of the first kind");
  setLabelPrefix("T");
  setupBF();
  checkRead();
}


void BF_Chebyshev::getAllValues(const double arg, double& argT, bool& inside_range, std::vector<double>& values, std::vector<double>& derivs) const {
  // plumed_assert(values.size()==numberOfBasisFunctions());
  // plumed_assert(derivs.size()==numberOfBasisFunctions());
  inside_range=true;
  argT=translateArgument(arg, inside_range);
  std::vector<double> derivsT(derivs.size());
  //
  values[0]=1.0;
  derivsT[0]=0.0;
  derivs[0]=0.0;
  values[1]=argT;
  derivsT[1]=1.0;
  derivs[1]=intervalDerivf();
  for(unsigned int i=1; i < getOrder();i++){
    values[i+1]  = 2.0*argT*values[i]-values[i-1];
    derivsT[i+1] = 2.0*values[i]+2.0*argT*derivsT[i]-derivsT[i-1];
    derivs[i+1]  = intervalDerivf()*derivsT[i+1];
  }
  if(!inside_range){for(unsigned int i=0;i<derivs.size();i++){derivs[i]=0.0;}}
}


void BF_Chebyshev::setupUniformIntegrals() {
  for(unsigned int i=0; i<numberOfBasisFunctions(); i++){
    double io = i;
    double value = 0.0;
    if(i % 2 == 0){
      value = -2.0/( pow(io,2.0)-1.0)*0.5;
    }
    setUniformIntegral(i,value);
  }
}


}
}