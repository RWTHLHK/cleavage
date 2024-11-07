[GlobalParams]
  displacements = 'ux uy'
[]

[Mesh]
  [file]
    type = FileMeshGenerator
    file = n10-id1.msh
  []
  [rename]
    type = RenameBlockGenerator
    input = file
    old_block = '1 2 3 4 5 6 7 8 9 10'
    new_block = '0 1 2 3 4 5 6 7 8 9'
  []
  [./bottom]
    type = BoundingBoxNodeSetGenerator
    new_boundary = bottom
    bottom_left = '5 0 0'
    top_right = '10 0 0'
    input = rename
  [../]
  [./top]
    type = BoundingBoxNodeSetGenerator
    new_boundary = top
    bottom_left = '0 10 0'
    top_right = '10 10 0'
    input = bottom
  [../]
  [./right]
    type = BoundingBoxNodeSetGenerator
    new_boundary = right
    bottom_left = '10 0 0'
    top_right = '10 10 0'
    input = top
  [../]
[]

[AuxVariables]
  [e_yy]
    order = CONSTANT
    family = MONOMIAL
  []
[]

[Physics/SolidMechanics/QuasiStatic/all]
  strain = FINITE
  add_variables = true
  additional_generate_output = 'vonmises_stress stress_yy'
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

[Kernels]
  [./solid_x]
    type = PhaseFieldFractureMechanicsOffDiag
    variable = ux
    component = 0
    c = c
  [../]
  [./solid_y]
    type = PhaseFieldFractureMechanicsOffDiag
    variable = uy
    component = 1
    c = c
  [../]
  [./off_disp]
    type = AllenCahnElasticEnergyOffDiag
    variable = c
    displacements = 'ux uy'
    mob_name = L
  [../]
[]

[AuxKernels]
  [e_yy]
    type = RankTwoAux
    variable = e_yy
    rank_two_tensor = total_lagrangian_strain
    index_j = 1
    index_i = 1
    execute_on = timestep_end
  []
[]

[BCs]
  [symmy]
    type = DirichletBC
    variable = uy
    boundary = bottom
    value = 0
  []
  [symmx]
    type = DirichletBC
    variable = ux
    boundary = right
    value = 0
  []
  [tdisp]
    type = FunctionDirichletBC
    variable = uy
    boundary = top
    function = t
  []
[]

[UserObjects]
  [./prop_read]
    type = PropertyReadFile
    prop_file_name = 'euler_ang_file.txt'
    # Enter file data as prop#1, prop#2, .., prop#nprop
    nprop = 3
    read_type = block
    nblock= 10
  [../]
[]

[Materials]
[./pfbulkmat]
    type = GenericConstantMaterial
    prop_names = 'gc_prop l visco'
    prop_values = '1e-4 0.05 0.1'
  [../]
  [elasticity_tensor]
    type = ComputeElasticityTensorCP
    #C_ijkl = '1.684e5 1.214e5 1.214e5 1.684e5 1.214e5 1.684e5 0.754e5 0.754e5 0.754e5'
    C_ijkl = '120 80'
    fill_method = symmetric_isotropic
    base_name = uncracked
  []
  [stress]
    type = ComputeMultipleCrystalPlasticityStress
    crystal_plasticity_models = 'trial_xtalpl'
    tan_mod_type = exact
    base_name = uncracked
  []
  [trial_xtalpl]
    type = CrystalPlasticityKalidindiUpdate
    number_slip_systems = 12
    slip_sys_file_name = input_slip_sys.txt
    base_name = uncracked
  []
  [updated_euler_angle]
    type = ComputeUpdatedEulerAngle
    radian_to_degree = true
  []
  [./cracked_stress]
    type = ComputeCrackedStressCP
    c = c
    F_name = E_el
    use_current_history_variable = true
    uncracked_base_name = uncracked
  [../]
[]

[Postprocessors]
  [stress_yy]
    type = ElementAverageValue
    variable = stress_yy
  []
  #[fp_zz]
    #type = ElementAverageValue
    #variable = fp_zz
  #[]
  [e_yy]
    type = ElementAverageValue
    variable = e_yy
  []
  #[gss]
    #type = ElementAverageValue
    #variable = gss
  #[]
  [./max_c]
    type = ElementExtremeValue
    variable = c
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
  petsc_options_iname = '-pc_type -pc_asm_overlap -sub_pc_type -ksp_type -ksp_gmres_restart'
  petsc_options_value = ' asm      2              lu            gmres     200'
  nl_abs_tol = 1e-10
  nl_rel_step_tol = 1e-10
  dtmax = 5.0
  nl_rel_tol = 1e-10
  dtmin = 1e-9
  end_time = 1
  #num_steps = 2
  nl_abs_step_tol = 1e-8
  dt = 1e-4
  [Predictor]
    type = SimplePredictor
    scale = 1.0
  []
[]

[Outputs]
  exodus = true
  csv = true
  gnuplot = true
[]
