#---------------------------------------------------------
#  This runs a Little Washita test problem with variable dz
#  in spinup mode with overland dampening turned on
#---------------------------------------------------------

from parflow import Run
from parflow.tools.fs import cp, mkdir, chdir, get_absolute_path

LWvdz = Run("LW_var_dz_spinup", __file__)

#---------------------------------------------------------
# Copying slope files
#---------------------------------------------------------

dir_name = get_absolute_path('test_output/LWvdz_s')
mkdir(dir_name)
chdir(dir_name)

cp('$PF_SRC/test/input/lw.1km.slope_x.10x.pfb')
cp('$PF_SRC/test/input/lw.1km.slope_y.10x.pfb')

#---------------------------------------------------------

LWvdz.FileVersion = 4

LWvdz.Process.Topology.P = 1
LWvdz.Process.Topology.Q = 1
LWvdz.Process.Topology.R = 1

#---------------------------------------------------------
# Computational Grid
#---------------------------------------------------------

LWvdz.ComputationalGrid.Lower.X = 0.0
LWvdz.ComputationalGrid.Lower.Y = 0.0
LWvdz.ComputationalGrid.Lower.Z = 0.0

LWvdz.ComputationalGrid.NX = 45
LWvdz.ComputationalGrid.NY = 32
LWvdz.ComputationalGrid.NZ = 25
LWvdz.ComputationalGrid.NZ = 10
LWvdz.ComputationalGrid.NZ = 6

LWvdz.ComputationalGrid.DX = 1000.0
LWvdz.ComputationalGrid.DY = 1000.0
#"native" grid resolution is 2m everywhere X NZ=25 for 50m
#computational domain.
LWvdz.ComputationalGrid.DZ = 2.0

#---------------------------------------------------------
# The Names of the GeomInputs
#---------------------------------------------------------

LWvdz.GeomInput.Names = 'domaininput'

LWvdz.GeomInput.domaininput.GeomName = 'domain'
LWvdz.GeomInput.domaininput.InputType = 'Box'

#---------------------------------------------------------
# Domain Geometry
#---------------------------------------------------------

LWvdz.Geom.domain.Lower.X = 0.0
LWvdz.Geom.domain.Lower.Y = 0.0
LWvdz.Geom.domain.Lower.Z = 0.0
#
LWvdz.Geom.domain.Upper.X = 45000.0
LWvdz.Geom.domain.Upper.Y = 32000.0
# this upper is synched to computational grid, not linked w/ Z multipliers
LWvdz.Geom.domain.Upper.Z = 12.0
LWvdz.Geom.domain.Patches = 'x_lower x_upper y_lower y_upper z_lower z_upper'

#--------------------------------------------
# variable dz assignments
#--------------------------------------------

LWvdz.Solver.Nonlinear.VariableDz = True
LWvdz.dzScale.GeomNames = 'domain'
LWvdz.dzScale.Type = 'nzList'
LWvdz.dzScale.nzListNumber = 6

LWvdz.Cell._0.dzScale.Value = 1.0
LWvdz.Cell._1.dzScale.Value = 1.00
LWvdz.Cell._2.dzScale.Value = 1.000
LWvdz.Cell._3.dzScale.Value = 1.000
LWvdz.Cell._4.dzScale.Value = 1.000
LWvdz.Cell._5.dzScale.Value = 0.05

#-----------------------------------------------------------------------------
# Perm
#-----------------------------------------------------------------------------

LWvdz.Geom.Perm.Names = 'domain'

# Values in m/hour

LWvdz.Geom.domain.Perm.Type = 'Constant'
LWvdz.Geom.domain.Perm.Value = 0.1

LWvdz.Geom.domain.Perm.LambdaX = 5000.0
LWvdz.Geom.domain.Perm.LambdaY = 5000.0
LWvdz.Geom.domain.Perm.LambdaZ = 50.0
LWvdz.Geom.domain.Perm.GeomMean = 0.0001427686

LWvdz.Geom.domain.Perm.Sigma = 0.20
LWvdz.Geom.domain.Perm.Sigma = 1.20
LWvdz.Geom.domain.Perm.NumLines = 150
LWvdz.Geom.domain.Perm.RZeta = 10.0
LWvdz.Geom.domain.Perm.KMax = 100.0000001
LWvdz.Geom.domain.Perm.DelK = 0.2
LWvdz.Geom.domain.Perm.Seed = 33333
LWvdz.Geom.domain.Perm.LogNormal = 'Log'
LWvdz.Geom.domain.Perm.StratType = 'Bottom'

LWvdz.Perm.TensorType = 'TensorByGeom'

LWvdz.Geom.Perm.TensorByGeom.Names = 'domain'

LWvdz.Geom.domain.Perm.TensorValX = 1.0
LWvdz.Geom.domain.Perm.TensorValY = 1.0
LWvdz.Geom.domain.Perm.TensorValZ = 1.0

#-----------------------------------------------------------------------------
# Specific Storage
#-----------------------------------------------------------------------------

LWvdz.SpecificStorage.Type = 'Constant'
LWvdz.SpecificStorage.GeomNames = 'domain'
LWvdz.Geom.domain.SpecificStorage.Value = 1.0e-5

#-----------------------------------------------------------------------------
# Phases
#-----------------------------------------------------------------------------

LWvdz.Phase.Names = 'water'

LWvdz.Phase.water.Density.Type = 'Constant'
LWvdz.Phase.water.Density.Value = 1.0

LWvdz.Phase.water.Viscosity.Type = 'Constant'
LWvdz.Phase.water.Viscosity.Value = 1.0

#-----------------------------------------------------------------------------
# Contaminants
#-----------------------------------------------------------------------------

LWvdz.Contaminants.Names = ''

#-----------------------------------------------------------------------------
# Retardation
#-----------------------------------------------------------------------------

LWvdz.Geom.Retardation.GeomNames = ''

#-----------------------------------------------------------------------------
# Gravity
#-----------------------------------------------------------------------------

LWvdz.Gravity = 1.0

#-----------------------------------------------------------------------------
# Setup timing info
#-----------------------------------------------------------------------------

LWvdz.TimingInfo.BaseUnit = 10.0
LWvdz.TimingInfo.StartCount = 0
LWvdz.TimingInfo.StartTime = 0.0
LWvdz.TimingInfo.StopTime = 7000.0
LWvdz.TimingInfo.DumpInterval = 1000.0
LWvdz.TimeStep.Type = 'Constant'
LWvdz.TimeStep.Value = 1000.0

#-----------------------------------------------------------------------------
# Porosity
#-----------------------------------------------------------------------------

LWvdz.Geom.Porosity.GeomNames = 'domain'
LWvdz.Geom.domain.Porosity.Type = 'Constant'
LWvdz.Geom.domain.Porosity.Value = 0.25

#-----------------------------------------------------------------------------
# Domain
#-----------------------------------------------------------------------------

LWvdz.Domain.GeomName = 'domain'

#-----------------------------------------------------------------------------
# Relative Permeability
#-----------------------------------------------------------------------------

LWvdz.Phase.RelPerm.Type = 'VanGenuchten'
LWvdz.Phase.RelPerm.GeomNames = 'domain'

LWvdz.Geom.domain.RelPerm.Alpha = 1.
LWvdz.Geom.domain.RelPerm.Alpha = 1.0
LWvdz.Geom.domain.RelPerm.N = 3.

#---------------------------------------------------------
# Saturation
#---------------------------------------------------------

LWvdz.Phase.Saturation.Type = 'VanGenuchten'
LWvdz.Phase.Saturation.GeomNames = 'domain'

LWvdz.Geom.domain.Saturation.Alpha = 1.0
LWvdz.Geom.domain.Saturation.Alpha = 1.0
LWvdz.Geom.domain.Saturation.N = 3.
LWvdz.Geom.domain.Saturation.SRes = 0.1
LWvdz.Geom.domain.Saturation.SSat = 1.0

#-----------------------------------------------------------------------------
# Wells
#-----------------------------------------------------------------------------

LWvdz.Wells.Names = ''

#-----------------------------------------------------------------------------
# Time Cycles
#-----------------------------------------------------------------------------

LWvdz.Cycle.Names = 'constant rainrec'
LWvdz.Cycle.Names = 'constant'
LWvdz.Cycle.constant.Names = 'alltime'
LWvdz.Cycle.constant.alltime.Length = 10000000
LWvdz.Cycle.constant.Repeat = -1

# rainfall and recession time periods are defined here
# rain for 1 hour, recession for 2 hours

LWvdz.Cycle.rainrec.Names = 'rain rec'
LWvdz.Cycle.rainrec.rain.Length = 10
LWvdz.Cycle.rainrec.rec.Length = 20
LWvdz.Cycle.rainrec.Repeat = 14
#
#-----------------------------------------------------------------------------
# Boundary Conditions: Pressure
#-----------------------------------------------------------------------------

LWvdz.BCPressure.PatchNames = LWvdz.Geom.domain.Patches

LWvdz.Patch.x_lower.BCPressure.Type = 'FluxConst'
LWvdz.Patch.x_lower.BCPressure.Cycle = 'constant'
LWvdz.Patch.x_lower.BCPressure.alltime.Value = 0.0

LWvdz.Patch.y_lower.BCPressure.Type = 'FluxConst'
LWvdz.Patch.y_lower.BCPressure.Cycle = 'constant'
LWvdz.Patch.y_lower.BCPressure.alltime.Value = 0.0

LWvdz.Patch.z_lower.BCPressure.Type = 'FluxConst'
LWvdz.Patch.z_lower.BCPressure.Cycle = 'constant'
LWvdz.Patch.z_lower.BCPressure.alltime.Value = 0.0

LWvdz.Patch.x_upper.BCPressure.Type = 'FluxConst'
LWvdz.Patch.x_upper.BCPressure.Cycle = 'constant'
LWvdz.Patch.x_upper.BCPressure.alltime.Value = 0.0

LWvdz.Patch.y_upper.BCPressure.Type = 'FluxConst'
LWvdz.Patch.y_upper.BCPressure.Cycle = 'constant'
LWvdz.Patch.y_upper.BCPressure.alltime.Value = 0.0

## overland flow boundary condition with very heavy rainfall then slight ET
LWvdz.Patch.z_upper.BCPressure.Type = 'OverlandFlow'
LWvdz.Patch.z_upper.BCPressure.Cycle = 'constant'
# constant recharge at 100 mm / y
LWvdz.Patch.z_upper.BCPressure.alltime.Value = -0.005
LWvdz.Patch.z_upper.BCPressure.alltime.Value = -0.0001

#---------------------------------------------------------
# Topo slopes in x-direction
#---------------------------------------------------------

LWvdz.TopoSlopesX.Type = 'PFBFile'
LWvdz.TopoSlopesX.GeomNames = 'domain'
LWvdz.TopoSlopesX.FileName = 'lw.1km.slope_x.10x.pfb'

#---------------------------------------------------------
# Topo slopes in y-direction
#---------------------------------------------------------

LWvdz.TopoSlopesY.Type = 'PFBFile'
LWvdz.TopoSlopesY.GeomNames = 'domain'
LWvdz.TopoSlopesY.FileName = 'lw.1km.slope_y.10x.pfb'

#---------------------------------------------------------
#  Distribute slopes
#---------------------------------------------------------

LWvdz.dist('lw.1km.slope_x.10x.pfb')
LWvdz.dist('lw.1km.slope_y.10x.pfb')

#---------------------------------------------------------
# Mannings coefficient
#---------------------------------------------------------

LWvdz.Mannings.Type = 'Constant'
LWvdz.Mannings.GeomNames = 'domain'
LWvdz.Mannings.Geom.domain.Value = 0.00005

#-----------------------------------------------------------------------------
# Phase sources:
#-----------------------------------------------------------------------------

LWvdz.PhaseSources.water.Type = 'Constant'
LWvdz.PhaseSources.water.GeomNames = 'domain'
LWvdz.PhaseSources.water.Geom.domain.Value = 0.0

#-----------------------------------------------------------------------------
# Exact solution specification for error calculations
#-----------------------------------------------------------------------------

LWvdz.KnownSolution = 'NoKnownSolution'

#-----------------------------------------------------------------------------
# Set solver parameters
#-----------------------------------------------------------------------------

LWvdz.Solver = 'Richards'
LWvdz.Solver.MaxIter = 2500

LWvdz.Solver.TerrainFollowingGrid = True

LWvdz.Solver.Nonlinear.MaxIter = 80
LWvdz.Solver.Nonlinear.ResidualTol = 1e-5
LWvdz.Solver.Nonlinear.EtaValue = 0.001

LWvdz.Solver.PrintSubsurf = False
LWvdz.Solver.Drop = 1E-20
LWvdz.Solver.AbsTol = 1E-10

LWvdz.Solver.Nonlinear.EtaChoice = 'EtaConstant'
LWvdz.Solver.Nonlinear.EtaValue = 0.001
LWvdz.Solver.Nonlinear.UseJacobian = True
LWvdz.Solver.Nonlinear.StepTol = 1e-25
LWvdz.Solver.Nonlinear.Globalization = 'LineSearch'
LWvdz.Solver.Linear.KrylovDimension = 80
LWvdz.Solver.Linear.MaxRestarts = 2

LWvdz.Solver.Linear.Preconditioner = 'MGSemi'
LWvdz.Solver.Linear.Preconditioner = 'PFMG'
LWvdz.Solver.Linear.Preconditioner.PCMatrixType = 'FullJacobian'

##---------------------------------------------------------
# Initial conditions: water pressure
#---------------------------------------------------------

# set water table to be at the bottom of the domain, the top layer is initially dry
LWvdz.ICPressure.Type = 'HydroStaticPatch'
LWvdz.ICPressure.GeomNames = 'domain'
LWvdz.Geom.domain.ICPressure.Value = 0.0

LWvdz.Geom.domain.ICPressure.RefGeom = 'domain'
LWvdz.Geom.domain.ICPressure.RefPatch = 'z_upper'

#spinup key
LWvdz.OverlandFlowSpinUp = 1
LWvdz.OverlandSpinupDampP1 = 1.0
LWvdz.OverlandSpinupDampP2 = 0.00001

#-----------------------------------------------------------------------------
# Run and do tests
#-----------------------------------------------------------------------------

LWvdz.run()
