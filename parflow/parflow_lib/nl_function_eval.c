/*BHEADER**********************************************************************

  Copyright (c) 1995-2009, Lawrence Livermore National Security,
  LLC. Produced at the Lawrence Livermore National Laboratory. Written
  by the Parflow Team (see the CONTRIBUTORS file)
  <parflow@lists.llnl.gov> CODE-OCEC-08-103. All rights reserved.

  This file is part of Parflow. For details, see
  http://www.llnl.gov/casc/parflow

  Please read the COPYRIGHT file or Our Notice and the LICENSE file
  for the GNU Lesser General Public License.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License (as published
  by the Free Software Foundation) version 2.1 dated February 1999.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms
  and conditions of the GNU General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA
**********************************************************************EHEADER*/

#include "parflow.h"
#include "llnlmath.h"
#include "llnltyps.h"

/*---------------------------------------------------------------------
 * Define module structures
 *---------------------------------------------------------------------*/

typedef struct
{
   int       time_index;

} PublicXtra;

typedef struct
{
   Problem      *problem;

   PFModule     *density_module;
   PFModule     *saturation_module;
   PFModule     *rel_perm_module;
   PFModule     *phase_source;
   PFModule     *bc_pressure;
   PFModule     *bc_internal;
} InstanceXtra;

/*---------------------------------------------------------------------
 * Define macros for function evaluation
 *---------------------------------------------------------------------*/

#define PMean(a, b, c, d)    HarmonicMean(c, d)
#define RPMean(a, b, c, d)   UpstreamMean(a, b, c, d)

/*  This routine provides the interface between KINSOL and ParFlow
    for function evaluations.  */

void     KINSolFunctionEval(size, pressure, fval, current_state)
int      size;
N_Vector pressure;
N_Vector fval;
void    *current_state;
{
   PFModule  *nl_function_eval = StateFunc(        ((State*)current_state) );
   ProblemData *problem_data   = StateProblemData( ((State*)current_state) );
   Vector      *old_pressure   = StateOldPressure(((State*)current_state) );
   Vector      *saturation     = StateSaturation(  ((State*)current_state) );
   Vector      *old_saturation = StateOldSaturation(((State*)current_state) );
   Vector      *density        = StateDensity(     ((State*)current_state) );
   Vector      *old_density    = StateOldDensity(  ((State*)current_state) );
   double       dt             = StateDt(          ((State*)current_state) );
   double       time           = StateTime(        ((State*)current_state) );
   double       *outflow       = StateOutflow(     ((State*)current_state) );
   Vector       *evap_trans    = StateEvapTrans(   ((State*)current_state) );
   Vector       *ovrl_bc_flx   = StateOvrlBcFlx(   ((State*)current_state) );
 
 
   PFModuleInvoke(void, nl_function_eval, 
   (pressure, fval, problem_data, saturation, old_saturation, 
   density, old_density, dt, time, old_pressure, outflow, evap_trans,
   ovrl_bc_flx) );
 
   return;
}


/*  This routine evaluates the nonlinear function based on the current 
    pressure values.  This evaluation is basically an application
    of the stencil to the pressure array. */

void    NlFunctionEval(pressure, fval, problem_data, saturation, 
old_saturation, density, old_density, dt, 
time, old_pressure, outflow, evap_trans, ovrl_bc_flx)

Vector      *pressure;       /* Current pressure values */
Vector      *fval;           /* Return values of the nonlinear function */
ProblemData *problem_data;   /* Geometry data for problem */
Vector      *old_pressure;
Vector      *saturation;     /* Saturation / work vector */
Vector      *old_saturation; /* Saturation values at previous time step */
Vector      *density;        /* Density vector */
Vector      *old_density;    /* Density values at previous time step */
double       dt;             /* Time step size */
double       time;           /* New time value */
double      *outflow;       /*sk Outflow due to overland flow*/
Vector      *evap_trans;     /*sk sink term from land surface model*/
Vector      *ovrl_bc_flx;     /*sk overland flow boundary fluxes*/ 

{
   PFModule      *this_module     = ThisPFModule;
   InstanceXtra  *instance_xtra   = PFModuleInstanceXtra(this_module);
   PublicXtra    *public_xtra     = PFModulePublicXtra(this_module);

   Problem     *problem           = (instance_xtra -> problem);

   PFModule    *density_module    = (instance_xtra -> density_module);
   PFModule    *saturation_module = (instance_xtra -> saturation_module);
   PFModule    *rel_perm_module   = (instance_xtra -> rel_perm_module);
   PFModule    *phase_source      = (instance_xtra -> phase_source);
   PFModule    *bc_pressure       = (instance_xtra -> bc_pressure);
   PFModule    *bc_internal       = (instance_xtra -> bc_internal);

   /* Re-use saturation vector to save memory */
   Vector      *rel_perm          = saturation;
   Vector      *source            = saturation;

   /* Overland flow variables */ //sk
   Vector      *KW, *KE, *KN, *KS;
   Vector      *qx, *qy;
   Subvector   *kw_sub, *ke_sub, *kn_sub, *ks_sub, *qx_sub, *qy_sub;
   Subvector   *x_sl_sub, *y_sl_sub, *mann_sub;
   Subvector   *obf_sub;
   double      *kw_, *ke_, *kn_, *ks_, *qx_, *qy_;
   double      *x_sl_dat, *y_sl_dat, *mann_dat;
   double      *obf_dat;
   double      dir_x, dir_y;
   double      q_overlnd;

   Vector      *porosity          = ProblemDataPorosity(problem_data);
   Vector      *permeability_x    = ProblemDataPermeabilityX(problem_data);
   Vector      *permeability_y    = ProblemDataPermeabilityY(problem_data);
   Vector      *permeability_z    = ProblemDataPermeabilityZ(problem_data);
   Vector      *sstorage          = ProblemDataSpecificStorage(problem_data);  //sk
   Vector      *x_sl              = ProblemDataTSlopeX(problem_data);  //sk
   Vector      *y_sl              = ProblemDataTSlopeY(problem_data);  //sk
   Vector      *man               = ProblemDataMannings(problem_data);  //sk

   double       gravity           = ProblemGravity(problem);
   double       viscosity         = ProblemPhaseViscosity(problem, 0);

   Subgrid     *subgrid;

   Subvector   *p_sub, *d_sub, *od_sub, *s_sub, *os_sub, *po_sub, *op_sub, *ss_sub, *et_sub;
   Subvector   *f_sub, *rp_sub, *permx_sub, *permy_sub, *permz_sub;

   Grid        *grid              = VectorGrid(pressure);
   Grid        *grid2d            = VectorGrid(x_sl);

   double      *pp, *odp, *sp, *osp, *pop, *fp, *dp, *rpp, *opp, *ss, *et;
   double      *permxp, *permyp, *permzp;

   int          i, j, k, r, is;
   int          ix, iy, iz;
   int          nx, ny, nz,gnx,gny;
   int          nx_f, ny_f, nz_f;
   int          nx_p, ny_p, nz_p;
   int          nx_po, ny_po, nz_po;
   int          sy_p, sz_p;
   int          ip, ipo,io;

   double       dtmp, dx, dy, dz, vol, ffx, ffy, ffz;
   double       u_right, u_front, u_upper;
   double       diff = 0.0e0;
   double       lower_cond, upper_cond;
   
   BCStruct    *bc_struct;
   GrGeomSolid *gr_domain         = ProblemDataGrDomain(problem_data);
   double      *bc_patch_values;
   double       u_old = 0.0e0;
   double       u_new = 0.0e0;
   double       value;
   int         *fdir;
   int          ipatch, ival;
   int          dir = 0;
   
   CommHandle  *handle;

   BeginTiming(public_xtra -> time_index);

   /* Initialize function values to zero. */
   PFVConstInit(0.0, fval);

   /* Pass pressure values to neighbors.  */
   handle = InitVectorUpdate(pressure, VectorUpdateAll);
   FinalizeVectorUpdate(handle);
 
   KW = NewVector( grid2d, 1, 1);
   KE = NewVector( grid2d, 1, 1);
   KN = NewVector( grid2d, 1, 1);
   KS = NewVector( grid2d, 1, 1);
   qx = NewVector( grid2d, 1, 1);
   qy = NewVector( grid2d, 1, 1);

   /* Pass permeability values */
   /*
     handle = InitVectorUpdate(permeability_x, VectorUpdateAll);
     FinalizeVectorUpdate(handle);

     handle = InitVectorUpdate(permeability_y, VectorUpdateAll);
     FinalizeVectorUpdate(handle);

     handle = InitVectorUpdate(permeability_z, VectorUpdateAll);
     FinalizeVectorUpdate(handle); */

   /* Calculate pressure dependent properties: density and saturation */

   PFModuleInvoke(void, density_module, (0, pressure, density, &dtmp, &dtmp, 
   CALCFCN));

   PFModuleInvoke(void, saturation_module, (saturation, pressure, density, 
   gravity, problem_data, CALCFCN));

   /* bc_struct = PFModuleInvoke(BCStruct *, bc_pressure, 
      (problem_data, grid, gr_domain, time));*/

   /*@ Why are the above things calculated here again; they were allready
     calculated in the driver solver_richards and passed further @*/

   /* Calculate accumulation terms for the function values */

   ForSubgridI(is, GridSubgrids(grid))
   {
      subgrid = GridSubgrid(grid, is);
	
      d_sub  = VectorSubvector(density, is);
      od_sub = VectorSubvector(old_density, is);
      p_sub = VectorSubvector(pressure, is);
      op_sub = VectorSubvector(old_pressure, is);
      s_sub  = VectorSubvector(saturation, is);
      os_sub = VectorSubvector(old_saturation, is);
      po_sub = VectorSubvector(porosity, is);
      f_sub  = VectorSubvector(fval, is);

      /* RDF: assumes resolutions are the same in all 3 directions */
      r = SubgridRX(subgrid);

      ix = SubgridIX(subgrid);
      iy = SubgridIY(subgrid);
      iz = SubgridIZ(subgrid);
	 
      nx = SubgridNX(subgrid);
      ny = SubgridNY(subgrid);
      nz = SubgridNZ(subgrid);
	 
      dx = SubgridDX(subgrid);
      dy = SubgridDY(subgrid);
      dz = SubgridDZ(subgrid);
	 
      vol = dx*dy*dz;

      nx_f = SubvectorNX(f_sub);
      ny_f = SubvectorNY(f_sub);
      nz_f = SubvectorNZ(f_sub);
	 
      nx_po = SubvectorNX(po_sub);
      ny_po = SubvectorNY(po_sub);
      nz_po = SubvectorNZ(po_sub);

      dp  = SubvectorData(d_sub);
      odp = SubvectorData(od_sub);
      sp  = SubvectorData(s_sub);
      pp = SubvectorData(p_sub);
      opp = SubvectorData(op_sub);
      osp = SubvectorData(os_sub);
      pop = SubvectorData(po_sub);
      fp  = SubvectorData(f_sub);

      GrGeomInLoop(i, j, k, gr_domain, r, ix, iy, iz, nx, ny, nz,
      {
	 ip  = SubvectorEltIndex(f_sub,   i, j, k);
	 ipo = SubvectorEltIndex(po_sub,  i, j, k);
	 fp[ip] = (sp[ip]*dp[ip] - osp[ip]*odp[ip])*pop[ipo]*vol;			 
      });
   }

   /*@ Add in contributions from compressible storage */

   ForSubgridI(is, GridSubgrids(grid))
   {
      subgrid = GridSubgrid(grid, is);
	
      ss_sub  = VectorSubvector(sstorage, is);

      d_sub  = VectorSubvector(density, is);
      od_sub = VectorSubvector(old_density, is);
      p_sub = VectorSubvector(pressure, is);
      op_sub = VectorSubvector(old_pressure, is);
      s_sub  = VectorSubvector(saturation, is);
      os_sub = VectorSubvector(old_saturation, is);
      f_sub  = VectorSubvector(fval, is);

      /* RDF: assumes resolutions are the same in all 3 directions */
      r = SubgridRX(subgrid);
	 
      ix = SubgridIX(subgrid);
      iy = SubgridIY(subgrid);
      iz = SubgridIZ(subgrid);
	 
      nx = SubgridNX(subgrid);
      ny = SubgridNY(subgrid);
      nz = SubgridNZ(subgrid);
	 
      dx = SubgridDX(subgrid);
      dy = SubgridDY(subgrid);
      dz = SubgridDZ(subgrid);
	 
      vol = dx*dy*dz;

      nx_f = SubvectorNX(f_sub);
      ny_f = SubvectorNY(f_sub);
      nz_f = SubvectorNZ(f_sub);
	 
      ss = SubvectorData(ss_sub);

      dp  = SubvectorData(d_sub);
      odp = SubvectorData(od_sub);
      sp  = SubvectorData(s_sub);
      pp = SubvectorData(p_sub);
      opp = SubvectorData(op_sub);
      osp = SubvectorData(os_sub);
      fp  = SubvectorData(f_sub);


      GrGeomInLoop(i, j, k, gr_domain, r, ix, iy, iz, nx, ny, nz,
      {
	 ip = SubvectorEltIndex(f_sub, i, j, k);
	 fp[ip] += ss[ip]*vol*(pp[ip]*sp[ip]*dp[ip] - opp[ip]*osp[ip]*odp[ip]);
      });
   }

   /* Add in contributions from source terms - user specified sources and
      flux wells.  Calculate phase source values overwriting current 
      saturation vector */
   PFModuleInvoke(void, phase_source, (source, problem, problem_data,
   time));

   ForSubgridI(is, GridSubgrids(grid))
   {
      subgrid = GridSubgrid(grid, is);
	
      s_sub  = VectorSubvector(source, is);
      f_sub  = VectorSubvector(fval, is);
      et_sub = VectorSubvector(evap_trans, is);
	 
      /* RDF: assumes resolutions are the same in all 3 directions */
      r = SubgridRX(subgrid);

      ix = SubgridIX(subgrid);
      iy = SubgridIY(subgrid);
      iz = SubgridIZ(subgrid);
	 
      nx = SubgridNX(subgrid);
      ny = SubgridNY(subgrid);
      nz = SubgridNZ(subgrid);
	 
      dx = SubgridDX(subgrid);
      dy = SubgridDY(subgrid);
      dz = SubgridDZ(subgrid);
	 
      vol = dx*dy*dz;

      nx_f = SubvectorNX(f_sub);
      ny_f = SubvectorNY(f_sub);
      nz_f = SubvectorNZ(f_sub);
	 
      sp = SubvectorData(s_sub);
      fp = SubvectorData(f_sub);
      et = SubvectorData(et_sub);

      GrGeomInLoop(i, j, k, gr_domain, r, ix, iy, iz, nx, ny, nz,
      {

	 ip = SubvectorEltIndex(f_sub, i, j, k);
	 fp[ip] -= vol * dt * (sp[ip] + et[ip]);
      });
   }

   bc_struct = PFModuleInvoke(BCStruct *, bc_pressure, 
   (problem_data, grid, gr_domain, time));


   /* Get boundary pressure values for Dirichlet boundaries.   */
   /* These are needed for upstream weighting in mobilities - need boundary */
   /* values for rel perms and densities. */

   ForSubgridI(is, GridSubgrids(grid))
   {
      subgrid = GridSubgrid(grid, is);
	 
      p_sub   = VectorSubvector(pressure, is);

      nx_p = SubvectorNX(p_sub);
      ny_p = SubvectorNY(p_sub);
      nz_p = SubvectorNZ(p_sub);
	 
      sy_p = nx_p;
      sz_p = ny_p * nx_p;

      pp = SubvectorData(p_sub);

      for (ipatch = 0; ipatch < BCStructNumPatches(bc_struct); ipatch++)
      {
	 bc_patch_values = BCStructPatchValues(bc_struct, ipatch, is);

	 switch(BCStructBCType(bc_struct, ipatch))
	 {

	    case DirichletBC:
	    {
	       BCStructPatchLoop(i, j, k, fdir, ival, bc_struct, ipatch, is,
	       {
		  ip   = SubvectorEltIndex(p_sub, i, j, k);
		  value =  bc_patch_values[ival];
		  pp[ip + fdir[0]*1 + fdir[1]*sy_p + fdir[2]*sz_p] = value;
	       
	       });
	       break;
	    }

	 }     /* End switch BCtype */
      }        /* End ipatch loop */
   }           /* End subgrid loop */

   /* Calculate relative permeability values overwriting current 
      phase source values */

   PFModuleInvoke(void, rel_perm_module, 
   (rel_perm, pressure, density, gravity, problem_data, 
   CALCFCN));

   /* Calculate contributions from second order derivatives and gravity */
   ForSubgridI(is, GridSubgrids(grid))
   {
      subgrid = GridSubgrid(grid, is);
	
      p_sub     = VectorSubvector(pressure, is);
      d_sub     = VectorSubvector(density, is);
      rp_sub    = VectorSubvector(rel_perm, is);
      f_sub     = VectorSubvector(fval, is);
      permx_sub = VectorSubvector(permeability_x, is);
      permy_sub = VectorSubvector(permeability_y, is);
      permz_sub = VectorSubvector(permeability_z, is);

      /* RDF: assumes resolutions are the same in all 3 directions */
      r = SubgridRX(subgrid);
	 
      ix = SubgridIX(subgrid) - 1;
      iy = SubgridIY(subgrid) - 1;
      iz = SubgridIZ(subgrid) - 1;
	 
      nx = SubgridNX(subgrid) + 1;
      ny = SubgridNY(subgrid) + 1;
      nz = SubgridNZ(subgrid) + 1;
	 
      dx = SubgridDX(subgrid);
      dy = SubgridDY(subgrid);
      dz = SubgridDZ(subgrid);
	 
      ffx = dy * dz;
      ffy = dx * dz;
      ffz = dx * dy;

      nx_p = SubvectorNX(p_sub);
      ny_p = SubvectorNY(p_sub);
      nz_p = SubvectorNZ(p_sub);
	 
      sy_p = nx_p;
      sz_p = ny_p * nx_p;

      pp    = SubvectorData(p_sub);
      dp    = SubvectorData(d_sub);
      rpp   = SubvectorData(rp_sub);
      fp    = SubvectorData(f_sub);
      permxp = SubvectorData(permx_sub);
      permyp = SubvectorData(permy_sub);
      permzp = SubvectorData(permz_sub);

      GrGeomInLoop(i, j, k, gr_domain, r, ix, iy, iz, nx, ny, nz,
      {
	 ip = SubvectorEltIndex(p_sub, i, j, k);

	 /* Calculate right face velocity.
	    diff >= 0 implies flow goes left to right */
	 diff    = pp[ip] - pp[ip+1];
	 u_right = ffx * PMean(pp[ip], pp[ip+1], 
	 permxp[ip], permxp[ip+1])
	    * (diff / dx )
	    * RPMean(pp[ip], pp[ip+1], rpp[ip]*dp[ip],
	    rpp[ip+1]*dp[ip+1])
	    / viscosity;

	 /* Calculate front face velocity.
	    diff >= 0 implies flow goes back to front */
	 diff    = pp[ip] - pp[ip+sy_p];
	 u_front = ffy * PMean(pp[ip], pp[ip+sy_p], permyp[ip], 
	 permyp[ip+sy_p])
	    * (diff / dy )
	    * RPMean(pp[ip], pp[ip+sy_p], rpp[ip]*dp[ip],
	    rpp[ip+sy_p]*dp[ip+sy_p])
	    / viscosity;
		
	 /* Calculate upper face velocity.
	    diff >= 0 implies flow goes lower to upper */
	 lower_cond = (pp[ip] / dz) - 0.5 * dp[ip] * gravity;
	 upper_cond = (pp[ip+sz_p] / dz) + 0.5 * dp[ip+sz_p] * gravity;
	 diff = lower_cond - upper_cond;
	 u_upper = ffz * PMean(pp[ip], pp[ip+sz_p], 
	 permzp[ip], permzp[ip+sz_p])
	    * diff
	    * RPMean(lower_cond, upper_cond, rpp[ip]*dp[ip], 
	    rpp[ip+sz_p]*dp[ip+sz_p])
	    / viscosity;

	 fp[ip]      += dt * ( u_right + u_front + u_upper );
	 fp[ip+1]    -= dt * u_right;
	 fp[ip+sy_p] -= dt * u_front;
	 fp[ip+sz_p] -= dt * u_upper;
      });
   }

   /*  Calculate correction for boundary conditions */

   ForSubgridI(is, GridSubgrids(grid))
   {
      subgrid = GridSubgrid(grid, is);
	 
      d_sub     = VectorSubvector(density, is);
      rp_sub    = VectorSubvector(rel_perm, is);
      f_sub     = VectorSubvector(fval, is);
      permx_sub = VectorSubvector(permeability_x, is);
      permy_sub = VectorSubvector(permeability_y, is);
      permz_sub = VectorSubvector(permeability_z, is);

      p_sub     = VectorSubvector(pressure, is);
      op_sub = VectorSubvector(old_pressure, is);
      os_sub = VectorSubvector(old_saturation, is);

      // sk Overland flow
      kw_sub = VectorSubvector(KW, is);
      ke_sub = VectorSubvector(KE, is);
      kn_sub = VectorSubvector(KN, is);
      ks_sub = VectorSubvector(KS, is);
      qx_sub = VectorSubvector(qx, is);
      qy_sub = VectorSubvector(qy, is);
      x_sl_sub = VectorSubvector(x_sl, is);
      y_sl_sub = VectorSubvector(y_sl, is);
      mann_sub = VectorSubvector(man, is);
      // SGS TODO This looks very wrong, why going to DB here, should come from DS 
      gnx = GetInt("ComputationalGrid.NX");
      gny = GetInt("ComputationalGrid.NY");
      obf_sub = VectorSubvector(ovrl_bc_flx,is);


      dx = SubgridDX(subgrid);
      dy = SubgridDY(subgrid);
      dz = SubgridDZ(subgrid);
      
      nx = SubgridNX(subgrid);
      ny = SubgridNY(subgrid);
      
      ix = SubgridIX(subgrid);
      iy = SubgridIY(subgrid); 
      
      ffx = dy * dz;
      ffy = dx * dz;
      ffz = dx * dy;

      vol = dx * dy * dz;
	 
      nx_p = SubvectorNX(p_sub);
      ny_p = SubvectorNY(p_sub);
      nz_p = SubvectorNZ(p_sub);
	 
      sy_p = nx_p;
      sz_p = ny_p * nx_p;

      dp     = SubvectorData(d_sub);
      rpp    = SubvectorData(rp_sub);
      fp     = SubvectorData(f_sub);
      permxp = SubvectorData(permx_sub);
      permyp = SubvectorData(permy_sub);
      permzp = SubvectorData(permz_sub);

      kw_ = SubvectorData(kw_sub);
      ke_ = SubvectorData(ke_sub);
      kn_ = SubvectorData(kn_sub);
      ks_ = SubvectorData(ks_sub);
      qx_ = SubvectorData(qx_sub);
      qy_ = SubvectorData(qy_sub);
      x_sl_dat = SubvectorData(x_sl_sub);
      y_sl_dat = SubvectorData(y_sl_sub);
      mann_dat = SubvectorData(mann_sub);
      obf_dat  = SubvectorData(obf_sub);

      pp = SubvectorData(p_sub);
      opp = SubvectorData(op_sub);
      osp = SubvectorData(os_sub);

      for (ipatch = 0; ipatch < BCStructNumPatches(bc_struct); ipatch++)
      {
	 bc_patch_values = BCStructPatchValues(bc_struct, ipatch, is);

	 switch(BCStructBCType(bc_struct, ipatch))
	 {

	    case DirichletBC:
	    {
	       BCStructPatchLoop(i, j, k, fdir, ival, bc_struct, ipatch, is,
	       {
		  ip   = SubvectorEltIndex(p_sub, i, j, k);

		  value =  bc_patch_values[ival];

		  /* Don't currently do upstream weighting on boundaries */

		  if (fdir[0])
		  {
		     switch(fdir[0])
		     {
			case -1:
			   dir = -1;
			   diff  = pp[ip-1] - pp[ip];
			   u_old = ffx 
			      * PMean(pp[ip-1], pp[ip], permxp[ip-1], permxp[ip])
			      * (diff / dx )
			      * RPMean(pp[ip-1], pp[ip], 
			      rpp[ip-1]*dp[ip-1], rpp[ip]*dp[ip]) 
			      / viscosity;
			   diff = value - pp[ip];
			   u_new = RPMean(value, pp[ip], 
			   rpp[ip-1]*dp[ip-1], rpp[ip]*dp[ip]);
			   break;
			case  1:
			   dir = 1;
			   diff  = pp[ip] - pp[ip+1];
			   u_old = ffx 
			      * PMean(pp[ip], pp[ip+1], permxp[ip], permxp[ip+1])
			      * (diff / dx )
			      * RPMean(pp[ip], pp[ip+1],
			      rpp[ip]*dp[ip], rpp[ip+1]*dp[ip+1]) 
			      / viscosity;
			   diff = pp[ip] - value;
			   u_new = RPMean(pp[ip], value,
			   rpp[ip]*dp[ip], rpp[ip+1]*dp[ip+1]);
			   break;
		     }
		     u_new = u_new * ffx * ( permxp[ip] / viscosity ) 
			* 2.0 * (diff/dx);
		  }
		  else if (fdir[1])
		  {
		     switch(fdir[1])
		     {
			case -1:
			   dir = -1;
			   diff  = pp[ip-sy_p] - pp[ip];
			   u_old = ffy 
			      * PMean(pp[ip-sy_p], pp[ip], 
			      permyp[ip-sy_p], permyp[ip])
			      * (diff / dy )
			      * RPMean(pp[ip-sy_p], pp[ip], 
			      rpp[ip-sy_p]*dp[ip-sy_p], rpp[ip]*dp[ip]) 
			      / viscosity;
			   diff =  value - pp[ip];
			   u_new = RPMean(value, pp[ip], 
			   rpp[ip-sy_p]*dp[ip-sy_p], rpp[ip]*dp[ip]);
			   break;
			case  1:
			   dir = 1;
			   diff  = pp[ip] - pp[ip+sy_p];
			   u_old = ffy 
			      * PMean(pp[ip], pp[ip+sy_p], 
			      permyp[ip], permyp[ip+sy_p])
			      * (diff / dy )
			      * RPMean(pp[ip], pp[ip+sy_p], 
			      rpp[ip]*dp[ip], rpp[ip+sy_p]*dp[ip+sy_p])
			      / viscosity;
			   diff = pp[ip] - value;
			   u_new = RPMean(pp[ip], value,
			   rpp[ip]*dp[ip], rpp[ip+sy_p]*dp[ip+sy_p]);
			   break;
		     }
		     u_new = u_new * ffy * ( permyp[ip] / viscosity ) 
			* 2.0 * (diff/dy);
		  }
		  else if (fdir[2])
		  {
		     switch(fdir[2])
		     {
			case -1:
			{
			   dir = -1;
			   lower_cond = (pp[ip-sz_p] / dz) 
			      - 0.5 * dp[ip-sz_p] * gravity;
			   upper_cond = (pp[ip] / dz) + 0.5 * dp[ip] * gravity;
			   diff = lower_cond - upper_cond;

			   u_old = ffz 
			      * PMean(pp[ip-sz_p], pp[ip], 
			      permzp[ip-sz_p], permzp[ip])
			      * diff
			      * RPMean(lower_cond, upper_cond, 
			      rpp[ip-sz_p]*dp[ip-sz_p], rpp[ip]*dp[ip]) 
			      / viscosity;

			   lower_cond = (value / dz) - 0.25 * dp[ip] * gravity;
			   upper_cond = (pp[ip] / dz) + 0.25 * dp[ip] * gravity;
			   diff = lower_cond - upper_cond;
			   u_new = RPMean(lower_cond, upper_cond, 
			   rpp[ip-sz_p]*dp[ip-sz_p], rpp[ip]*dp[ip]);
			   break;
			}   /* End case -1 */
			case  1:
			{
			   dir = 1;
			   lower_cond = (pp[ip] / dz) - 0.5 * dp[ip] * gravity;
			   upper_cond = (pp[ip+sz_p] / dz) 
			      - 0.5 * dp[ip+sz_p] * gravity;
			   diff = lower_cond - upper_cond;
			   u_old = ffz 
			      * PMean(pp[ip], pp[ip+sz_p], 
			      permzp[ip], permzp[ip+sz_p])
			      * diff
			      * RPMean(lower_cond, upper_cond, 
			      rpp[ip]*dp[ip], rpp[ip+sz_p]*dp[ip+sz_p])
			      / viscosity;
			   lower_cond = (pp[ip] / dz) - 0.25 * dp[ip] * gravity;
			   upper_cond = (value / dz) + 0.25 * dp[ip] * gravity;
			   diff = lower_cond - upper_cond;
			   u_new = RPMean(lower_cond, upper_cond,
			   rpp[ip]*dp[ip], rpp[ip+sz_p]*dp[ip+sz_p]);
			   break;
			}   /* End case 1 */
		     }
		     u_new = u_new * ffz * ( permzp[ip] / viscosity ) 
			* 2.0 * diff;
		  }

		  /* Remove the boundary term computed above */
		  fp[ip] -= dt * dir * u_old;

		  /* Add the correct boundary term */
		  fp[ip] += dt * dir * u_new;
	       });

	       break;
	    }

	    case FluxBC:
	    {
	       BCStructPatchLoop(i, j, k, fdir, ival, bc_struct, ipatch, is,
	       {
		  ip   = SubvectorEltIndex(p_sub, i, j, k);

		  if (fdir[0])
		  {
		     switch(fdir[0])
		     {
			case -1:
			   dir = -1;
			   diff  = pp[ip-1] - pp[ip];
			   u_old = ffx * PMean(pp[ip-1], pp[ip], 
			   permxp[ip-1], permxp[ip])
			      * (diff / dx )
			      * RPMean(pp[ip-1], pp[ip], 
			      rpp[ip-1]*dp[ip-1], rpp[ip]*dp[ip]) 
			      / viscosity;
			   break;
			case  1:
			   dir = 1;
			   diff  = pp[ip] - pp[ip+1];
			   u_old = ffx * PMean(pp[ip], pp[ip+1], 
			   permxp[ip], permxp[ip+1])
			      * (diff / dx )
			      * RPMean(pp[ip], pp[ip+1], 
			      rpp[ip]*dp[ip], rpp[ip+1]*dp[ip+1]) 
			      / viscosity;
			   break;
		     }
		     u_new = ffx;
		  }
		  else if (fdir[1])
		  {
		     switch(fdir[1])
		     {
			case -1:
			   dir = -1;
			   diff  = pp[ip-sy_p] - pp[ip];
			   u_old = ffy * PMean(pp[ip-sy_p], pp[ip], 
			   permyp[ip-sy_p], permyp[ip])
			      * (diff / dy )
			      * RPMean(pp[ip-sy_p], pp[ip], 
			      rpp[ip-sy_p]*dp[ip-sy_p], rpp[ip]*dp[ip]) 
			      / viscosity;
			   break;
			case  1:
			   dir = 1;
			   diff  = pp[ip] - pp[ip+sy_p];
			   u_old = ffy * PMean(pp[ip], pp[ip+sy_p], 
			   permyp[ip], permyp[ip+sy_p])
			      * (diff / dy )
			      * RPMean(pp[ip], pp[ip+sy_p], 
			      rpp[ip]*dp[ip], rpp[ip+sy_p]*dp[ip+sy_p])
			      / viscosity;
			   break;
		     }
		     u_new = ffy;
		  }
		  else if (fdir[2])
		  {
		     switch(fdir[2])
		     {
			case -1:
			   dir = -1;
			   lower_cond = (pp[ip-sz_p] / dz) 
			      - 0.5 * dp[ip-sz_p] * gravity;
			   upper_cond = (pp[ip] / dz) + 0.5 * dp[ip] * gravity;
			   diff = lower_cond - upper_cond;
			   u_old = ffz * PMean(pp[ip-sz_p], pp[ip], 
			   permzp[ip-sz_p], permzp[ip])
			      * diff
			      * RPMean(lower_cond, upper_cond, 
			      rpp[ip-sz_p]*dp[ip-sz_p], rpp[ip]*dp[ip]) 
			      / viscosity;
			   break;
			case  1:
			   dir = 1;
			   lower_cond = (pp[ip] / dz) - 0.5 * dp[ip] * gravity;
			   upper_cond = (pp[ip+sz_p] / dz)
			      + 0.5 * dp[ip+sz_p] * gravity;
			   diff = lower_cond - upper_cond;
			   u_old = ffz * PMean(0, 0, permzp[ip], permzp[ip+sz_p])
			      * diff
			      * RPMean(lower_cond, upper_cond,
			      rpp[ip]*dp[ip], rpp[ip+sz_p]*dp[ip+sz_p])
			      / viscosity;
			   break;
		     }
		     u_new = ffz;
		  }

		  /* Remove the boundary term computed above */
		  fp[ip] -= dt * dir * u_old;
		  /* Add the correct boundary term */
		  u_new = u_new * bc_patch_values[ival];
		  fp[ip] += dt * dir * u_new;
	       });

	       break;
	    }     /* End fluxbc case */

	    case OverlandBC:
	    {
	       BCStructPatchLoop(i, j, k, fdir, ival, bc_struct, ipatch, is,
	       {
		  ip   = SubvectorEltIndex(p_sub, i, j, k);

		  if (fdir[0])
		  {

		     switch(fdir[0])
		     {
			case -1:
			   dir = -1;
			   diff  = pp[ip-1] - pp[ip];
			   u_old = ffx * PMean(pp[ip-1], pp[ip], 
			   permxp[ip-1], permxp[ip])
			      * (diff / dx )
			      * RPMean(pp[ip-1], pp[ip], 
			      rpp[ip-1]*dp[ip-1], rpp[ip]*dp[ip]) 
			      / viscosity;
			   break;
			case  1:
			   dir = 1;
			   diff  = pp[ip] - pp[ip+1];
			   u_old = ffx * PMean(pp[ip], pp[ip+1], 
			   permxp[ip], permxp[ip+1])
			      * (diff / dx )
			      * RPMean(pp[ip], pp[ip+1], 
			      rpp[ip]*dp[ip], rpp[ip+1]*dp[ip+1]) 
			      / viscosity;
			   break;
		     }
		     u_new = ffx;
		  }
		  else if (fdir[1])
		  {

		     switch(fdir[1])
		     {
			case -1:
			   dir = -1;
			   diff  = pp[ip-sy_p] - pp[ip];
			   u_old = ffy * PMean(pp[ip-sy_p], pp[ip], 
			   permyp[ip-sy_p], permyp[ip])
			      * (diff / dy )
			      * RPMean(pp[ip-sy_p], pp[ip], 
			      rpp[ip-sy_p]*dp[ip-sy_p], rpp[ip]*dp[ip]) 
			      / viscosity;
			   break;
			case  1:
			   dir = 1;
			   diff  = pp[ip] - pp[ip+sy_p];
			   u_old = ffy * PMean(pp[ip], pp[ip+sy_p], 
			   permyp[ip], permyp[ip+sy_p])
			      * (diff / dy )
			      * RPMean(pp[ip], pp[ip+sy_p], 
			      rpp[ip]*dp[ip], rpp[ip+sy_p]*dp[ip+sy_p])
			      / viscosity;
			   break;
		     }
		     u_new = ffy;
		  }
		  else if (fdir[2])
		  {

		     switch(fdir[2])
		     {
			case -1:
			   dir = -1;
			   lower_cond = (pp[ip-sz_p] / dz) 
			      - 0.5 * dp[ip-sz_p] * gravity;
			   upper_cond = (pp[ip] / dz) + 0.5 * dp[ip] * gravity;
			   diff = lower_cond - upper_cond;
			   u_old = ffz * PMean(pp[ip-sz_p], pp[ip], 
			   permzp[ip-sz_p], permzp[ip])
			      * diff
			      * RPMean(lower_cond, upper_cond, 
			      rpp[ip-sz_p]*dp[ip-sz_p], rpp[ip]*dp[ip]) 
			      / viscosity;
			   break;
			case  1:
			   dir = 1;
			   lower_cond = (pp[ip] / dz) - 0.5 * dp[ip] * gravity;
			   upper_cond = (pp[ip+sz_p] / dz)
			      + 0.5 * dp[ip+sz_p] * gravity;
			   diff = lower_cond - upper_cond;
			   u_old = ffz * PMean(0, 0, permzp[ip], permzp[ip+sz_p])
			      * diff
			      * RPMean(lower_cond, upper_cond,
			      rpp[ip]*dp[ip], rpp[ip+sz_p]*dp[ip+sz_p])
			      / viscosity;
			   break;
		     }
		     u_new = ffz;
		  }

		  /* Remove the boundary term computed above */
		  fp[ip] -= dt * dir * u_old;

		  u_new = u_new * bc_patch_values[ival]; //sk: here we go in and implement surface routing!

		  fp[ip] += dt * dir * u_new;
	       });
 

	       // SGS TODO can these loops be merged?
	       BCStructPatchLoopOvrlnd(i, j, k, fdir, ival, bc_struct, ipatch, is,
	       {
		  if (fdir[2])
		  {
		     switch(fdir[2])
		     {
			case 1:
			   io   = SubvectorEltIndex(p_sub, i, j, 0);
			   ip   = SubvectorEltIndex(p_sub, i, j, k);

			   dir_x = 0.0;
			   dir_y = 0.0;
			   if(x_sl_dat[io] > 0.0) dir_x = -1.0;
			   if(y_sl_dat[io] > 0.0) dir_y = -1.0;
			   if(x_sl_dat[io] < 0.0) dir_x = 1.0; 
			   if(y_sl_dat[io] < 0.0) dir_y = 1.0; 

			   qx_[io] = dir_x * (RPowerR(fabs(x_sl_dat[io]),0.5) / mann_dat[io]) * RPowerR(max((pp[ip]),0.0),(5.0/3.0));

			   qy_[io] = dir_y * (RPowerR(fabs(y_sl_dat[io]),0.5) / mann_dat[io]) * RPowerR(max((pp[ip]),0.0),(5.0/3.0));

			   break;
		     }
		  }

	       });

	       BCStructPatchLoop(i, j, k, fdir, ival, bc_struct, ipatch, is,
	       {
		  if (fdir[2])
		  {
		     switch(fdir[2])
		     {
			case 1:
			   io   = SubvectorEltIndex(p_sub, i, j, 0);

		           ke_[io] = max(qx_[io],0.0) - max(-qx_[io+1],0.0);
		           kw_[io] = max(qx_[io-1],0.0) - max(-qx_[io],0.0);

		           kn_[io] = max(qy_[io],0.0) - max(-qy_[io+sy_p],0.0);
		           ks_[io] = max(qy_[io-sy_p],0.0) - max(-qy_[io],0.0);
		   
 			   break;
		     }
		  }

	       });


	       *outflow = 0.0;
	       BCStructPatchLoop(i, j, k, fdir, ival, bc_struct, ipatch, is,
	       {
		  if (fdir[2])
		  {
		     switch(fdir[2])
		     {
			case 1:
			   dir = 1;
			   ip   = SubvectorEltIndex(p_sub, i, j, k);
			   io   = SubvectorEltIndex(p_sub, i, j, 0);

			   q_overlnd = 0.0;
			   q_overlnd = vol * (max(pp[ip],0.0) - max(opp[ip],0.0)) /dz +
			      dt * vol * ((ke_[io]-kw_[io])/dx + (kn_[io] - ks_[io])/dy) / dz;
		   
			   obf_dat[io] = 0.0;
			   if ( i >= 0 && i <= (gnx-1) && j == 0 && qy_[io] < 0.0 ){ //south face
			      obf_dat[io]+= fabs(qy_[io]);
			   } else if (i == 0 && j >= 0 && j <= (gny-1) && qx_[io] < 0.0) { // west face
			      obf_dat[io]+= fabs(qx_[io]);
			   } else if (i >= 0 && i <= (gnx-1) && j == (gny-1) && qy_[io] > 0.0) { //north face
			      obf_dat[io]+= fabs(qy_[io]);
			   } else if (i == (gnx-1) && j >= 0 && j <= (gny-1) && qx_[io] > 0.0) { //east face
			      obf_dat[io]+= fabs(qx_[io]);
			   } else if (i > 0 && i < (gnx-1) && j > 0 && j < (gny-1)) { //interior
			      obf_dat[io] = qx_[io];
			   }
			   
			   
			   if (j==0 && i==0){
			      *outflow=fabs(ks_[io])+fabs(kw_[io]);
			   }
			   
			   fp[ip] += q_overlnd;
			   
   			   break;
		     }
		  }
			
	       });
	    
	       break;
	    }     /* End OverlandBC case */
	 
	 }     /* End switch BCtype */
      }        /* End ipatch loop */
   }           /* End subgrid loop */
   
   

   FreeBCStruct(bc_struct);

   PFModuleInvoke( void, bc_internal, (problem, problem_data, fval, NULL, 
   time, pressure, CALCFCN));

   /* Set pressures outside domain to zero.  
    * Recall: equation to solve is f = 0, so components of f outside 
    * domain are set to the respective pressure value.
    *
    * Should change this to set pressures to scaling value.
    * CSW: Should I set this to pressure * vol * dt ??? */

   ForSubgridI(is, GridSubgrids(grid))
   {
      subgrid = GridSubgrid(grid, is);
	 
      p_sub = VectorSubvector(pressure, is);
      f_sub = VectorSubvector(fval, is);

      /* RDF: assumes resolutions are the same in all 3 directions */
      r = SubgridRX(subgrid);

      ix = SubgridIX(subgrid);
      iy = SubgridIY(subgrid);
      iz = SubgridIZ(subgrid);
	 
      nx = SubgridNX(subgrid);
      ny = SubgridNY(subgrid);
      nz = SubgridNZ(subgrid);
	 
      pp = SubvectorData(p_sub);
      fp = SubvectorData(f_sub);

      GrGeomOutLoop(i, j, k, gr_domain,
      r, ix, iy, iz, nx, ny, nz,
      {
	 ip   = SubvectorEltIndex(f_sub, i, j, k);
	 fp[ip] = pp[ip];
      });
   }

   EndTiming(public_xtra -> time_index);

   FreeVector(KW);
   FreeVector(KE);
   FreeVector(KN);
   FreeVector(KS);
   FreeVector(qx);
   FreeVector(qy); 

   return;
}


/*--------------------------------------------------------------------------
 * NlFunctionEvalInitInstanceXtra
 *--------------------------------------------------------------------------*/

PFModule    *NlFunctionEvalInitInstanceXtra(Problem     *problem,
					    Grid        *grid,
					    double      *temp_data)
					    
{
   PFModule      *this_module   = ThisPFModule;
   InstanceXtra  *instance_xtra;

   if ( PFModuleInstanceXtra(this_module) == NULL )
      instance_xtra = ctalloc(InstanceXtra, 1);
   else
      instance_xtra = PFModuleInstanceXtra(this_module);

   if ( problem != NULL)
   {
      (instance_xtra -> problem) = problem;
   }

   if ( PFModuleInstanceXtra(this_module) == NULL )
   {
      (instance_xtra -> density_module) =
         PFModuleNewInstance(ProblemPhaseDensity(problem), () );
      (instance_xtra -> saturation_module) =
         PFModuleNewInstance(ProblemSaturation(problem), (NULL, NULL) );
      (instance_xtra -> rel_perm_module) =
         PFModuleNewInstance(ProblemPhaseRelPerm(problem), (NULL, NULL) );
      (instance_xtra -> phase_source) =
         PFModuleNewInstance(ProblemPhaseSource(problem), (grid));
      (instance_xtra -> bc_pressure) =
         PFModuleNewInstance(ProblemBCPressure(problem), (problem) );
      (instance_xtra -> bc_internal) =
         PFModuleNewInstance(ProblemBCInternal(problem), () );

   }
   else
   {
      PFModuleReNewInstance((instance_xtra -> density_module), ());
      PFModuleReNewInstance((instance_xtra -> saturation_module), 
      (NULL, NULL));
      PFModuleReNewInstance((instance_xtra -> rel_perm_module), 
      (NULL, NULL));
      PFModuleReNewInstance((instance_xtra -> phase_source), (NULL));
      PFModuleReNewInstance((instance_xtra -> bc_pressure), (problem));
      PFModuleReNewInstance((instance_xtra -> bc_internal), ());
   }

   PFModuleInstanceXtra(this_module) = instance_xtra;
   return this_module;
}


/*--------------------------------------------------------------------------
 * NlFunctionEvalFreeInstanceXtra
 *--------------------------------------------------------------------------*/

void  NlFunctionEvalFreeInstanceXtra()
{
   PFModule      *this_module   = ThisPFModule;
   InstanceXtra  *instance_xtra = PFModuleInstanceXtra(this_module);

   if(instance_xtra)
   {
      PFModuleFreeInstance(instance_xtra -> density_module);
      PFModuleFreeInstance(instance_xtra -> saturation_module);
      PFModuleFreeInstance(instance_xtra -> rel_perm_module);
      PFModuleFreeInstance(instance_xtra -> phase_source);
      PFModuleFreeInstance(instance_xtra -> bc_pressure);
      PFModuleFreeInstance(instance_xtra -> bc_internal);
      
      tfree(instance_xtra);
   }
}


/*--------------------------------------------------------------------------
 * NlFunctionEvalNewPublicXtra
 *--------------------------------------------------------------------------*/

PFModule   *NlFunctionEvalNewPublicXtra()
{
   PFModule      *this_module   = ThisPFModule;
   PublicXtra    *public_xtra;


   public_xtra = ctalloc(PublicXtra, 1);

   (public_xtra -> time_index) = RegisterTiming("NL_F_Eval");

   PFModulePublicXtra(this_module) = public_xtra;

   return this_module;
}


/*--------------------------------------------------------------------------
 * NlFunctionEvalFreePublicXtra
 *--------------------------------------------------------------------------*/

void  NlFunctionEvalFreePublicXtra()
{
   PFModule    *this_module   = ThisPFModule;
   PublicXtra  *public_xtra   = PFModulePublicXtra(this_module);


   if (public_xtra)
   {
      tfree(public_xtra);
   }
}


/*--------------------------------------------------------------------------
 * NlFunctionEvalSizeOfTempData
 *--------------------------------------------------------------------------*/

int  NlFunctionEvalSizeOfTempData()
{
   return 0;
}



