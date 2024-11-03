//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "ACOrientation.h"
#include "RankTwoTensor.h"
registerMooseObject("PhaseFieldApp", ACOrientation);

InputParameters
ACOrientation::validParams()
{
  InputParameters params = ACInterface::validParams();
  params.addClassDescription("Gradient energy Allen-Cahn Kernel where crack propagation along weak"
                             "cleavage plane is preferred");
  params.addRequiredParam<Real>(
      "beta_penalty",
      "penalty to penalize fracture on planes not normal to one cleavage plane normal which is "
      "normal to weak cleavage plane. Setting beta=0 results in isotropic damage.");
  // params.addRequiredParam<RealVectorValue>("cleavage_plane_normal",
  //                                          "Normal to the weak cleavage plane");
  params.addRequiredParam<std::string>("uncracked_base_name","uncracked base name of damage model");
  return params;
}

ACOrientation::ACOrientation(const InputParameters & parameters)
  : ACInterface(parameters),
     _uncracked_base_name(getParam<std::string>("uncracked_base_name") + "_"),
    _beta_penalty(getParam<Real>("beta_penalty")),
    _crysrot(getMaterialPropertyByName<RankTwoTensor>(_uncracked_base_name + "crysrot"))
{
}

Real
ACOrientation::betaNablaPsi()
{
  return _beta_penalty * _L[_qp] * _kappa[_qp] * 
        ((_grad_u[_qp] * _crysrot[_qp].column(0)) * (_grad_test[_i][_qp] * _crysrot[_qp].column(0))); 
        // + 
        //  (_grad_u[_qp] * _crysrot[_qp].column(1)) * (_grad_test[_i][_qp] * _crysrot[_qp].column(1)) +
        //  (_grad_u[_qp] * _crysrot[_qp].column(2)) * (_grad_test[_i][_qp] * _crysrot[_qp].column(2)));

}

RealVectorValue
ACOrientation::computeDamageProjection(){
  return _crysrot[_qp]*_grad_u[_qp];
}

RealVectorValue
ACOrientation::computeDamageRotation(const RealVectorValue &pro_vec, Real &p_tot){
  Real p1 = pro_vec(0);
  Real p2 = pro_vec(1);
  Real p3 = pro_vec(2);
  p_tot = p1*p2 + p1*p3 + p2*p3;
  //compute rotationof damage projection
  Real rot1 = p3 * (_crysrot[_qp](0,0)+_crysrot[_qp](1,0)) + 
              p2 * (_crysrot[_qp](0,0)+_crysrot[_qp](2,0)) +
              p1 * (_crysrot[_qp](1,0)+_crysrot[_qp](2,0));

  Real rot2 = p3 * (_crysrot[_qp](0,1)+_crysrot[_qp](1,1)) + 
              p2 * (_crysrot[_qp](0,1)+_crysrot[_qp](2,1)) +
              p1 * (_crysrot[_qp](1,1)+_crysrot[_qp](2,1));

  Real rot3 = p3 * (_crysrot[_qp](0,2)+_crysrot[_qp](1,2)) + 
              p2 * (_crysrot[_qp](0,2)+_crysrot[_qp](2,2)) +
              p1 * (_crysrot[_qp](1,2)+_crysrot[_qp](2,2));

  RealVectorValue rot_vec(rot1, rot2, rot3);
  return rot_vec;
}

void 
ACOrientation::computeDamageVec(RealVectorValue &pro_vec, RealVectorValue &rot_vec, Real &p_tot){
  pro_vec = computeDamageProjection();
  rot_vec = computeDamageRotation(pro_vec, p_tot);
}

Real
ACOrientation::computeQpResidual()
{
  RealVectorValue pro_vec(0,0,0);
  RealVectorValue rot_vec(0,0,0);
  Real p_tot = 0.0;
  computeDamageVec(pro_vec,rot_vec, p_tot);
  return _grad_u[_qp] * kappaNablaLPsi() + _beta_penalty * kappaNablaLPsi()*p_tot*rot_vec;
}

Real
ACOrientation::computeQpJacobian()
{
  /// dsum is the derivative \f$ \frac\partial{\partial \eta} \left( \nabla
  /// (L\psi) \right) \f$
  /// isotropic jacobian
  RealGradient dsum =
      (_dkappadop[_qp] * _L[_qp] + _kappa[_qp] * _dLdop[_qp]) * _phi[_j][_qp] * _grad_test[_i][_qp];

  /// compute the derivative of the gradient of the mobility
  if (_variable_L)
  {
    RealGradient dgradL =
        _grad_phi[_j][_qp] * _dLdop[_qp] + _grad_u[_qp] * _phi[_j][_qp] * _d2Ldop2[_qp];

    for (unsigned int i = 0; i < _n_args; ++i)
      dgradL += (*_gradarg[i])[_qp] * _phi[_j][_qp] * (*_d2Ldargdop[i])[_qp];

    dsum += (_kappa[_qp] * dgradL + _dkappadop[_qp] * _phi[_j][_qp] * gradL()) * _test[_i][_qp];
  }
  /// orientant jacobian
  RealVectorValue pro_vec(0,0,0);
  RealVectorValue rot_vec(0,0,0);
  Real p_tot = 0.0;
  computeDamageVec(pro_vec,rot_vec, p_tot);
  // Real sigma1 = _beta_penalty * (rot_vec(1)*rot_vec(2) +  p_tot*)
  return _grad_phi[_j][_qp] * kappaNablaLPsi() + _grad_u[_qp] * dsum;
  // return (1 + _beta_penalty) * _grad_phi[_j][_qp] * kappaNablaLPsi() + _grad_u[_qp] * dsum -
  //        _beta_penalty * _L[_qp] * _kappa[_qp] * 
  //        ((_grad_u[_qp] * _crysrot[_qp].column(0)) *(_grad_phi[_j][_qp] * _crysrot[_qp].column(0))); 

}

