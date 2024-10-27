//*Linghao cleavage fracture stress due to volumetric strain energy

#include "ComputeVolCrackedStress.h"

registerMooseObject("SolidMechanicsApp", ComputeVolCrackedStress);

InputParameters
ComputeVolCrackedStress::validParams()
{
  InputParameters params = Material::validParams();
  params.addClassDescription("Computes energy and modifies the stress for phase field fracture");
  params.addRequiredCoupledVar("c", "Order parameter for damage");
  params.addParam<Real>("kdamage", 1e-9, "Stiffness of damaged matrix");
  params.addParam<bool>("finite_strain_model", false, "The model is using finite strain");
  params.addParam<bool>(
      "use_current_history_variable", false, "Use the current value of the history variable.");
  params.addParam<MaterialPropertyName>(
      "F_name", "E_el", "Name of material property storing the elastic energy");
  params.addParam<MaterialPropertyName>(
      "kappa_name",
      "kappa_op",
      "Name of material property being created to store the interfacial parameter kappa");
  params.addParam<MaterialPropertyName>(
      "mobility_name", "L", "Name of material property being created to store the mobility L");
  params.addParam<std::string>("base_name", "The base name used to save the cracked stress");
  params.addRequiredParam<std::string>("uncracked_base_name",
                                       "The base name used to calculate the original stress");
  return params;
}

ComputeVolCrackedStress::ComputeVolCrackedStress(const InputParameters & parameters)
  : DerivativeMaterialInterface<Material>(parameters),
    _base_name(isParamValid("base_name") ? getParam<std::string>("base_name") + "_" : ""),
    _uncracked_base_name(getParam<std::string>("uncracked_base_name") + "_"),
    _elasticity_tensor_name(_uncracked_base_name + "elasticity_tensor"),
    _elasticity_tensor(getMaterialPropertyByName<RankFourTensor>(_elasticity_tensor_name)),
    _finite_strain_model(getParam<bool>("finite_strain_model")),
    _use_current_hist(getParam<bool>("use_current_history_variable")),
    _strain(
        _finite_strain_model
            ? getMaterialPropertyByName<RankTwoTensor>(_uncracked_base_name + "elastic_strain")
            : getMaterialPropertyByName<RankTwoTensor>(_uncracked_base_name + "mechanical_strain")),
    _uncracked_stress(getMaterialPropertyByName<RankTwoTensor>(_uncracked_base_name + "stress")),
    _uncracked_Jacobian_mult(
        getMaterialPropertyByName<RankFourTensor>(_uncracked_base_name + "Jacobian_mult")),
    _c(coupledValue("c")),
    _gc_prop(getMaterialProperty<Real>("gc_prop")),
    _l(getMaterialProperty<Real>("l")),
    _visco(getMaterialProperty<Real>("visco")),
    _kdamage(getParam<Real>("kdamage")),
    _stress(declareProperty<RankTwoTensor>(_base_name + "stress")),
    _F(declareProperty<Real>(getParam<MaterialPropertyName>("F_name"))),
    _dFdc(declarePropertyDerivative<Real>(getParam<MaterialPropertyName>("F_name"),
                                          coupledName("c", 0))),
    _d2Fdc2(declarePropertyDerivative<Real>(
        getParam<MaterialPropertyName>("F_name"), coupledName("c", 0), coupledName("c", 0))),
    _d2Fdcdstrain(declareProperty<RankTwoTensor>("d2Fdcdstrain")),
    _dstress_dc(declarePropertyDerivative<RankTwoTensor>("stress", coupledName("c", 0))),
    _hist(declareProperty<Real>("hist")),
    _hist_old(getMaterialPropertyOld<Real>("hist")),
    _Jacobian_mult(declareProperty<RankFourTensor>(_base_name + "Jacobian_mult")),
    _kappa(declareProperty<Real>(getParam<MaterialPropertyName>("kappa_name"))),
    _L(declareProperty<Real>(getParam<MaterialPropertyName>("mobility_name")))
{
}

void
ComputeVolCrackedStress::initQpStatefulProperties()
{
  _stress[_qp].zero();
  _hist[_qp] = 0.0;
}

void
ComputeVolCrackedStress::computeQpProperties()
{
  const Real c = _c[_qp];

  // Zero out values when c > 1
  Real cfactor = 1.0;
  if (c > 1.0)
    cfactor = 0.0;

  // Create the positive and negative projection tensors
  RankFourTensor I4sym(RankFourTensor::initIdentitySymmetricFour);
  RankTwoTensor stress_dev = _uncracked_stress[_qp].deviatoric();
  RankTwoTensor stress_vol = _uncracked_stress[_qp] - stress_dev;
  // compute volumetric strain energy
  Real G0 = stress_vol.doubleContraction(_strain[_qp]) / 2.0;

  // Update the history variable
  if (G0 > _hist_old[_qp])
    _hist[_qp] = G0;
  else
    _hist[_qp] = _hist_old[_qp];

  Real hist_variable = _hist_old[_qp];
  if (_use_current_hist)
    hist_variable = _hist[_qp];

  // Compute degredation function and derivatives
  Real h = cfactor * (1.0 - c) * (1.0 - c) * (1.0 - _kdamage) + _kdamage;
  Real dhdc = -2.0 * cfactor * (1.0 - c) * (1.0 - _kdamage);
  Real d2hdc2 = 2.0 * cfactor * (1.0 - _kdamage);

  // Compute stress and its derivatives
  _stress[_qp] = h*stress_vol + stress_dev;
  _dstress_dc[_qp] = stress_vol * dhdc;

  // Compute damaged and undamaged bulk modulus(only valid for isotropic material!)
  Real k = (_elasticity_tensor[_qp](0,0,0,0)+2.0*_elasticity_tensor[_qp](0,0,1,1))/3.0;
  Real del_k = (1.0-h)*k;
  _Jacobian_mult[_qp] = _uncracked_Jacobian_mult[_qp] - del_k*I4sym;

  // Compute energy and its derivatives
  _F[_qp] = hist_variable * h + _gc_prop[_qp] * c * c / (2 * _l[_qp]);
  _dFdc[_qp] = hist_variable * dhdc + _gc_prop[_qp] * c / _l[_qp];
  _d2Fdc2[_qp] = hist_variable * d2hdc2 + _gc_prop[_qp] / _l[_qp];

  // 2nd derivative wrt c and strain = 0.0 if we used the previous step's history varible
  if (_use_current_hist)
    _d2Fdcdstrain[_qp] = stress_vol * dhdc;

  // Assign L and kappa
  _kappa[_qp] = _gc_prop[_qp] * _l[_qp];
  _L[_qp] = 1.0 / (_gc_prop[_qp] * _visco[_qp]);
}
