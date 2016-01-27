/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2011-2014 The plumed team
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
#ifndef __PLUMED_ves_biases_LinearBasisSetExpansion_h
#define __PLUMED_ves_biases_LinearBasisSetExpansion_h

#include <vector>
#include <string>

namespace PLMD {

class Action;
class Keywords;
class Value;
class Communicator;
class Grid;
class CoeffsVector;
class BasisFunctions;
namespace bias{
  class VesBias;


class LinearBasisSetExpansion{
private:
  std::string label_;
  //
  Action* action_pntr;
  bias::VesBias* vesbias_pntr;
  Communicator& mycomm;
  bool serial_;
  //
  std::vector<Value*> args_pntrs;
  unsigned int nargs_;
  //
  std::vector<BasisFunctions*> basisf_pntrs;
  std::vector<unsigned int> nbasisf_;
  //
  CoeffsVector* bias_coeffs_pntr;
  size_t ncoeffs_;
  CoeffsVector* coeffderivs_aver_ps_pntr;
  CoeffsVector* fes_wt_coeffs_pntr;
  //
  double biasf_;
  double invbiasf_;
  //
  Grid* bias_grid_pntr;
  Grid* fes_grid_pntr;
  Grid* ps_grid_pntr;
  //
 public:
  static void registerKeywords( Keywords& keys );
  // Constructor
  explicit LinearBasisSetExpansion(
    const std::string&,
    Communicator&,
    std::vector<Value*>,
    std::vector<BasisFunctions*>,
    CoeffsVector* bias_coeffs_pntr_in=NULL);
  //
  ~LinearBasisSetExpansion();
  //
  std::vector<Value*> getPntrsToArguments() const ;
  std::vector<BasisFunctions*> getPntrsToBasisFunctions() const ;
  CoeffsVector* getPntrToBiasCoeffs() const ;
  Grid* getPntrToBiasGrid() const ;
  //
  unsigned int getNumberOfArguments() const ;
  std::vector<unsigned int> getNumberOfBasisFunctions() const ;
  size_t getNumberOfCoeffs() const ;
  //
  CoeffsVector& BiasCoeffs() const;
  CoeffsVector& FesWTCoeffs() const;
  CoeffsVector& CoeffDerivsAverTargetDist() const;
  //
  void setSerial() {serial_=true;}
  void setParallel() {serial_=false;}
  //
  void linkVesBias(bias::VesBias*);
  void linkAction(Action*);
  // calculate bias and derivatives
  static double getBiasAndForces(const std::vector<double>&, std::vector<double>&, std::vector<double>&, std::vector<BasisFunctions*>&, CoeffsVector*, Communicator* comm_in=NULL);
  double getBiasAndForces(const std::vector<double>&, std::vector<double>&, std::vector<double>&);
  // Grid stuff
  void setupBiasGrid(const std::vector<unsigned int>&, const bool usederiv=false);
  void updateBiasGrid();
  void writeBiasGridToFile(const std::string&, const bool);
  //
  void setupUniformTargetDistribution();
  // Well-Tempered p(s) stuff
  void setupWellTemperedTargetDistribution(const double, const std::vector<unsigned int>&);
  void updateWellTemperedFESCoeffs();
};

inline
std::vector<Value*> LinearBasisSetExpansion::getPntrsToArguments() const {return args_pntrs;}

inline
std::vector<BasisFunctions*> LinearBasisSetExpansion::getPntrsToBasisFunctions() const {return basisf_pntrs;}

inline
CoeffsVector* LinearBasisSetExpansion::getPntrToBiasCoeffs() const {return bias_coeffs_pntr;}

inline
Grid* LinearBasisSetExpansion::getPntrToBiasGrid() const {return bias_grid_pntr;}

inline
unsigned int LinearBasisSetExpansion::getNumberOfArguments() const {return nargs_;}

inline
std::vector<unsigned int> LinearBasisSetExpansion::getNumberOfBasisFunctions() const {return nbasisf_;}

inline
size_t LinearBasisSetExpansion::getNumberOfCoeffs() const {return ncoeffs_;}

inline
CoeffsVector& LinearBasisSetExpansion::BiasCoeffs() const {return *bias_coeffs_pntr;}

inline
CoeffsVector& LinearBasisSetExpansion::FesWTCoeffs() const {return *fes_wt_coeffs_pntr;}

inline
CoeffsVector& LinearBasisSetExpansion::CoeffDerivsAverTargetDist() const {return *coeffderivs_aver_ps_pntr;}


}

}

#endif