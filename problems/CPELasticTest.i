[GlobalParams]
  displacements = 'disp_x disp_y disp_z'
[]

[Mesh]
  [gen]
    type = GeneratedMeshGenerator
    dim = 3
    xmax = 0.01
    ymax = 0.01
    zmax = 0.01
    nx = 20
    ny = 20
    nz = 20
  []
  [./noncrack]
    type = BoundingBoxNodeSetGenerator
    new_boundary = noncrack
    bottom_left = '0.005 0 0'
    top_right = '0.01 0 0.01'
    input = gen
  [../]
[]

[AuxVariables]
  [euler_angle_1]
    order = CONSTANT
    family = MONOMIAL
  []
  [euler_angle_2]
    order = CONSTANT
    family = MONOMIAL
  []
  [euler_angle_3]
    order = CONSTANT
    family = MONOMIAL
  []
  [./e_yy]
    order = CONSTANT
    family = MONOMIAL
  [../]
[]

[Physics/SolidMechanics/QuasiStatic/all]
  strain = FINITE
  add_variables = true
  #generate_output = stress_zz
  additional_generate_output = 'stress_yy'
  strain_base_name = uncracked
[]
[Modules]
  [./PhaseField]
    [./Nonconserved]
      [./c]
        free_energy = E_el
        kappa = kappa_op
        mobility = L
      [../]
    [../]
  [../]
[]
[AuxKernels]
  [e_yy]
    type = RankTwoAux
    variable = e_yy
    rank_two_tensor = uncracked_elastic_strain
    index_j = 1
    index_i = 1
    execute_on = timestep_end
  []
[]

[Kernels]
  [./solid_x]
    type = PhaseFieldFractureMechanicsOffDiag
    variable = disp_x
    component = 0
    c = c
  [../]
  [./solid_y]
    type = PhaseFieldFractureMechanicsOffDiag
    variable = disp_y
    component = 1
    c = c
  [../]
  [./solid_z]
    type = PhaseFieldFractureMechanicsOffDiag
    variable = disp_z
    component = 1
    c = c
  [../]
  [./off_disp]
    type = AllenCahnElasticEnergyOffDiag
    variable = c
    displacements = 'disp_x disp_y disp_z'
    mob_name = L
  [../]
  [./ACbulk]
    type = AllenCahn
    variable = c
    f_name = E_el
  [../]
  [./ACInterfaceCleavageFracture]
    type = ACInterfaceCleavageFracture
    variable = c
    beta_penalty = 10
    cleavage_plane_normal = '-0.707 0.707 0.0'
  [../]
  [./dcdt]
    type = TimeDerivative
    variable = c
  [../]
[]

[BCs]
  [fix_x]
    type = DirichletBC
    variable = disp_x
    boundary = 'left'
    value = 0
  []
  [fix_y]
    type = DirichletBC
    variable = disp_y
    boundary = 'noncrack'
    value = 0
  []
  [fix_z]
    type = DirichletBC
    variable = disp_z
    boundary = 'back'
    value = 0
  []
  [tdisp]
    type = FunctionDirichletBC
    variable = disp_y
    boundary = 'top'
    function = 't'
  []
[]

[Materials]
  [./pfbulkmat]
    type = GenericConstantMaterial
    prop_names = 'gc_prop l visco'
    prop_values = '1e-3 1e-4 5e-3'
  [../]
  [elasticity_tensor]
    type = ComputeElasticityTensorCP
    #C_ijkl = '1.684e5 1.214e5 1.214e5 1.684e5 1.214e5 1.684e5 0.754e5 0.754e5 0.754e5'
    C_ijkl = '1.2e5 0.8e5'
    fill_method = symmetric_isotropic
    base_name = uncracked
    euler_angle_1 = 0
    euler_angle_2 = 0
    euler_angle_3 = 0
  []
  [./stress]
    type = ComputeFiniteStrainElasticStress
    base_name = uncracked
  [../]
  [./cracked_stress]
    type = ComputeCrackedStress
    c = c
    F_name = E_el
    use_current_history_variable = true
    uncracked_base_name = uncracked
    finite_strain_model = true
  [../]
[]

[Postprocessors]
  [./stress_yy]
    type = ElementAverageValue
    variable = stress_yy
  [../]
  [./e_yy]
    type = ElementAverageValue
    variable = e_yy
  [../]
[]

[Preconditioning]
  [smp]
    type = SMP
    full = true
  []
[]

[Executioner]
  type = Transient
  solve_type = 'PJFNK'

  petsc_options_iname = '-pc_type -pc_factor_mat_solving_package'
  petsc_options_value = 'lu superlu_dist'
  nl_abs_tol = 1e-10
  nl_rel_tol = 1e-10
  nl_abs_step_tol = 1e-10
  [Predictor]
    type = SimplePredictor
    scale = 1.0
  []
  dt = 5e-7
  #dtmin = 1e-3
  end_time = 2e-5
[]

[Outputs]
  csv = true
  exodus = true
[]
