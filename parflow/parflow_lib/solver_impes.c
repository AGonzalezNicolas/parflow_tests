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

/***************************************************************************
 *
 * Top level Multi-Phase Fractional-Flow solver module:
 *              solves pressure equation,
 *              advects saturations,
 *              advects contaminants.
 *           Implicit Pressure Explicit Saturation method is used.
 *
 *---------------------------------------------------------------------------
 *
 ***************************************************************************/

#include "parflow.h"


/*------------------------------------------------------------------------
 * Structures
 *------------------------------------------------------------------------*/

typedef struct
{
   PFModule          *discretize_pressure;
   PFModule          *diag_scale;
   PFModule          *linear_solver;
   PFModule          *permeability_face;
   PFModule          *phase_velocity_face;
   PFModule          *total_velocity_face;
   PFModule          *advect_satur;
   PFModule          *advect_concen;
   PFModule          *set_problem_data;

   Problem           *problem;

   int                sadvect_order;
   int                advect_order;
   double             CFL;
   int                max_iterations;
   double             rel_tol;               /* relative tolerance */
   double             abs_tol;               /* absolute tolerance */
   double             drop_tol;              /* drop tolerance */
   int                print_subsurf_data;    /* print perm./porosity? */
   int                print_press;           /* print pressures? */
   int                print_velocities;      /* print velocities? */
   int                print_satur;           /* print saturations? */
   int                print_concen;          /* print concentrations? */
   int                print_wells;           /* print well data? */

   int                write_silo_subsurf_data;    /* print perm./porosity? */
   int                write_silo_press;           /* print pressures? */
   int                write_silo_velocities;      /* print velocities? */
   int                write_silo_satur;           /* print saturations? */
   int                write_silo_concen;          /* print concentrations? */

} PublicXtra;

typedef struct
{
   PFModule          *discretize_pressure;
   PFModule          *diag_scale;
   PFModule          *linear_solver;
   PFModule          *permeability_face;
   PFModule          *phase_velocity_face;
   PFModule          *total_velocity_face;
   PFModule          *advect_satur;
   PFModule          *advect_concen;
   PFModule          *set_problem_data;

   PFModule          *retardation;
   PFModule          *phase_density;
   PFModule          *phase_mobility;
   PFModule          *ic_phase_satur;
   PFModule          *ic_phase_concen;
   PFModule          *bc_phase_saturation;
   PFModule          *constitutive;

   Grid              *grid;
   Grid              *grid2d;
   Grid              *x_grid;
   Grid              *y_grid;
   Grid              *z_grid;

   ProblemData       *problem_data;

   double            *temp_data;

} InstanceXtra;


/*-------------------------------------------------------------------------
 * SolverImpes
 *-------------------------------------------------------------------------*/

void      SolverImpes()
{
   PFModule      *this_module      = ThisPFModule;
   PublicXtra    *public_xtra      = PFModulePublicXtra(this_module);
   InstanceXtra  *instance_xtra    = PFModuleInstanceXtra(this_module);

   Problem      *problem           = (public_xtra -> problem);

   int           sadvect_order       = (public_xtra -> sadvect_order);
   int           advect_order        = (public_xtra -> advect_order);
   double        CFL                 = (public_xtra -> CFL);
   int           max_iterations      = (public_xtra -> max_iterations);
/*   double        rel_tol             = (public_xtra -> rel_tol);  */
   double        abs_tol             = (public_xtra -> abs_tol);
   double        drop_tol            = (public_xtra -> drop_tol);
   int           print_subsurf_data  = (public_xtra -> print_subsurf_data);
   int           print_press         = (public_xtra -> print_press);
   int           print_velocities    = (public_xtra -> print_velocities);
   int           print_satur         = (public_xtra -> print_satur);
   int           print_concen        = (public_xtra -> print_concen);
   int           print_wells         = (public_xtra -> print_wells);

   PFModule     *discretize_pressure = 
                                 (instance_xtra -> discretize_pressure);
   PFModule     *diag_scale          = (instance_xtra -> diag_scale);
   PFModule     *linear_solver       = (instance_xtra -> linear_solver);
   PFModule     *permeability_face   = (instance_xtra -> permeability_face);
   PFModule     *phase_velocity_face = 
                                 (instance_xtra -> phase_velocity_face);
   PFModule     *total_velocity_face = 
                                 (instance_xtra -> total_velocity_face);
   PFModule     *advect_satur        = (instance_xtra -> advect_satur);
   PFModule     *phase_density       = (instance_xtra -> phase_density);
   PFModule     *advect_concen       = (instance_xtra -> advect_concen);
   PFModule     *set_problem_data    = (instance_xtra -> set_problem_data);

   PFModule     *retardation         = (instance_xtra -> retardation);
   PFModule     *phase_mobility      = (instance_xtra -> phase_mobility);
   PFModule     *ic_phase_satur      = (instance_xtra -> ic_phase_satur);
   PFModule     *ic_phase_concen     = (instance_xtra -> ic_phase_concen);
   PFModule     *bc_phase_saturation = 
                                 (instance_xtra -> bc_phase_saturation);
   PFModule     *constitutive        = (instance_xtra -> constitutive);

   ProblemData  *problem_data        = (instance_xtra -> problem_data);

   Grid         *grid                = (instance_xtra -> grid);
   Grid         *x_grid              = (instance_xtra -> x_grid);
   Grid         *y_grid              = (instance_xtra -> y_grid);
   Grid         *z_grid              = (instance_xtra -> z_grid);
				     
   Vector       *temp_mobility_x     = NULL;
   Vector       *temp_mobility_y     = NULL;
   Vector       *temp_mobility_z     = NULL;
   Vector       *stemp               = NULL;
   Vector       *ctemp               = NULL;

   int           start_count         = ProblemStartCount(problem);
   double        start_time          = ProblemStartTime(problem);
   double        stop_time           = ProblemStopTime(problem);
   double        dump_interval       = ProblemDumpInterval(problem);

   GrGeomSolid  *gr_domain;

   Vector       *pressure;
   Vector       *total_mobility_x, *total_mobility_y, *total_mobility_z;
   Vector      **saturations;
   Vector      **concentrations;

   Vector      **phase_x_velocity, **phase_y_velocity, **phase_z_velocity;
   Vector       *total_x_velocity,  *total_y_velocity,  *total_z_velocity;
   Vector       *z_permeability;

   Vector       *solidmassfactor;

   Matrix       *A;
   Vector       *f;

   EvalStruct   *eval_struct;

   int           iteration_number, number_logged, file_number, dump_index;
   int           indx, phase, concen;
   int           transient, recompute_pressure, still_evolving, 
                 any_file_dumped;
   int           dump_files, evolve_saturations, evolve_concentrations;
   int           is_multiphase;

   double        t;
   double        dt;
   double       *phase_dt, min_phase_dt, total_dt, print_dt, well_dt, bc_dt;
   double        phase_maximum, total_maximum;
   double        dtmp, *phase_densities;

   CommHandle   *handle;

   char          dt_info;
   char          file_prefix[64], file_postfix[64];

   double       *time_log, *dt_log;
   int          *seq_log,  *dumped_log;
   char         *recomp_log, *dt_info_log;

   is_multiphase = ProblemNumPhases(problem) > 1;

   t = start_time;

   /*-------------------------------------------------------------------
    * Allocate temp vectors
    *-------------------------------------------------------------------*/
   if ( is_multiphase )
   {
      temp_mobility_x = NewVector(instance_xtra -> grid, 1, 1);
      temp_mobility_y = NewVector(instance_xtra -> grid, 1, 1);
      temp_mobility_z = NewVector(instance_xtra -> grid, 1, 1);
      stemp           = NewVector(instance_xtra -> grid, 1, 3);
   }
   ctemp              = NewVector(instance_xtra -> grid, 1, 3);


   IfLogging(1)
   {
      seq_log      = talloc(int,    max_iterations + 1);
      time_log     = talloc(double, max_iterations + 1);
      dt_log       = talloc(double, max_iterations + 1);
      dt_info_log  = talloc(char,   max_iterations + 1);
      dumped_log   = talloc(int,    max_iterations + 1);
      recomp_log   = talloc(char,   max_iterations + 1);
      number_logged = 0;
   }

   sprintf(file_prefix, GlobalsOutFileName);

   /* do turning bands (and other stuff maybe) */
   PFModuleInvoke(void, set_problem_data, (problem_data));
   gr_domain = ProblemDataGrDomain(problem_data);

   phase_densities = talloc(double, ProblemNumPhases(problem));

   for(phase = 0; phase < ProblemNumPhases(problem)-1; phase++)
      {
	 PFModuleInvoke(void, phase_density, 
			(phase, NULL, NULL, &dtmp, &phase_densities[phase], 
			 CALCFCN));
      }

   if ( print_subsurf_data )
   {
      sprintf(file_postfix, "perm_x");
      WritePFBinary(file_prefix, file_postfix, 
		    ProblemDataPermeabilityX(problem_data));

      sprintf(file_postfix, "perm_y");
      WritePFBinary(file_prefix, file_postfix, 
		    ProblemDataPermeabilityY(problem_data));

      sprintf(file_postfix, "perm_z");
      WritePFBinary(file_prefix, file_postfix, 
		    ProblemDataPermeabilityZ(problem_data));

      sprintf(file_postfix, "porosity");
      WritePFBinary(file_prefix, file_postfix, 
		    ProblemDataPorosity(problem_data));
   }

   if ( public_xtra -> write_silo_subsurf_data )
   {
      sprintf(file_postfix, "perm_x");
      WriteSilo(file_prefix, file_postfix, ProblemDataPermeabilityX(problem_data),
                t, 0, "PermeabilityX");

      sprintf(file_postfix, "perm_y");
      WriteSilo(file_prefix, file_postfix, ProblemDataPermeabilityY(problem_data),
                t, 0, "PermeabilityY");

      sprintf(file_postfix, "perm_z");
      WriteSilo(file_prefix, file_postfix, ProblemDataPermeabilityZ(problem_data),
                t, 0, "PermeabilityZ");

      sprintf(file_postfix, "porosity");
      WriteSilo(file_prefix, file_postfix, ProblemDataPorosity(problem_data),
	        t, 0, "Porosity");

   }

   if(!amps_Rank(amps_CommWorld))
   {
      PrintWellData(ProblemDataWellData(problem_data), 
		    (WELLDATA_PRINTPHYSICAL | WELLDATA_PRINTVALUES));
   }

   /* Check to see if time dependency is requested and set flags */
   if ( start_count < 0 )
   {
      transient             = 0;
      still_evolving        = 0;
      recompute_pressure    = 1;
      evolve_saturations    = 0;
      evolve_concentrations = 0;
   }
   else
   {
      transient             = 1;
      still_evolving        = 1;
      recompute_pressure    = 1;
      evolve_saturations    = 1;
      evolve_concentrations = 1;
   }

   pressure = NewVector( grid, 1, 1 );
   InitVectorAll(pressure, 0.0);

   total_mobility_x = NewVector( grid, 1, 1 );
   InitVectorAll(total_mobility_x, 0.0);

   total_mobility_y = NewVector( grid, 1, 1 );
   InitVectorAll(total_mobility_y, 0.0);

   total_mobility_z = NewVector( grid, 1, 1 );
   InitVectorAll(total_mobility_z, 0.0);

   /*-------------------------------------------------------------------
    * Allocate and set up initial saturations
    *-------------------------------------------------------------------*/

   saturations = ctalloc(Vector *, ProblemNumPhases(problem) );

   if ( is_multiphase )
   {
      for(phase = 0; phase < ProblemNumPhases(problem)-1; phase++)
      {
         saturations[phase] = NewVector( grid, 1, 3 );
         InitVectorAll(saturations[phase], 0.0);

         PFModuleInvoke(void, ic_phase_satur,
		        (saturations[phase], phase, problem_data));
         PFModuleInvoke(void, bc_phase_saturation,
                        (saturations[phase], phase, gr_domain));

         handle = InitVectorUpdate(saturations[phase], VectorUpdateGodunov);
         FinalizeVectorUpdate(handle);
      }

      saturations[ProblemNumPhases(problem)-1] = NewVector( grid, 1, 3 );
      InitVectorAll(saturations[ProblemNumPhases(problem)-1], 0.0);

      PFModuleInvoke(void, constitutive, (saturations));

      handle = InitVectorUpdate(saturations[ProblemNumPhases(problem)-1], 
				VectorUpdateGodunov);
      FinalizeVectorUpdate(handle);
   }
   else
   {
      saturations[0] = NULL;
   }

   /*-------------------------------------------------------------------
    * If (transient); initialize some stuff
    *-------------------------------------------------------------------*/

   if ( transient )
   {
      iteration_number = file_number = start_count;
      dump_index = 1;
      if ( (t >= stop_time) || (iteration_number > max_iterations) )
      {
         recompute_pressure    = 0;

         transient             = 0;
         still_evolving        = 0;
         evolve_saturations    = 0;
         evolve_concentrations = 0;

         print_press           = 0;
         print_satur           = 0;
         print_concen          = 0;
         print_wells           = 0;
      }
      else
      {
         dt = 0.0;

         dump_files = 1;
         
         if ( is_multiphase )
            eval_struct = NewEvalStruct( problem );

         if ( ProblemNumContaminants(problem) > 0 )
         {
            solidmassfactor = NewVector( grid, 1, 2);

            /*----------------------------------------------------------------
             * Allocate and set up initial concentrations
             *----------------------------------------------------------------*/

            concentrations = ctalloc( Vector *,
                                      ProblemNumPhases(problem)*ProblemNumContaminants(problem) );

            indx = 0;
            for(phase = 0; phase < ProblemNumPhases(problem); phase++)
            {
               for(concen = 0; concen < ProblemNumContaminants(problem); concen++)
               {
                  concentrations[indx] = NewVector( grid, 1, 3 );
                  InitVectorAll(concentrations[indx], 0.0);

                  PFModuleInvoke(void, ic_phase_concen,
                                 (concentrations[indx], phase, concen, problem_data));

                  handle = InitVectorUpdate(concentrations[indx], VectorUpdateGodunov);
                  FinalizeVectorUpdate(handle);
                  indx++;
               }
            }
         }

         /*----------------------------------------------------------------
          * Allocate phase/total velocities and total mobility
          *----------------------------------------------------------------*/

         phase_x_velocity = ctalloc(Vector *, ProblemNumPhases(problem) );
         for(phase = 0; phase < ProblemNumPhases(problem); phase++)
         {
            phase_x_velocity[phase] = NewVector( x_grid, 1, 1 );
            InitVectorAll(phase_x_velocity[phase], 0.0);
         }

         phase_y_velocity = ctalloc(Vector *, ProblemNumPhases(problem) );
         for(phase = 0; phase < ProblemNumPhases(problem); phase++)
         {
            phase_y_velocity[phase] = NewVector( y_grid, 1, 1 );
            InitVectorAll(phase_y_velocity[phase], 0.0);
         }

         phase_z_velocity = ctalloc(Vector *, ProblemNumPhases(problem) );
         for(phase = 0; phase < ProblemNumPhases(problem); phase++)
         {
            phase_z_velocity[phase] = NewVector( z_grid, 1, 2 );
            InitVectorAll(phase_z_velocity[phase], 0.0);
         }

         phase_dt = ctalloc( double, ProblemNumPhases(problem) );

	 if ( is_multiphase )
	 {
	    total_x_velocity = NewVector( x_grid, 1, 1 );
            InitVectorAll(total_x_velocity, 0.0);

            total_y_velocity = NewVector( y_grid, 1, 1 );
            InitVectorAll(total_y_velocity, 0.0);

            total_z_velocity = NewVector( z_grid, 1, 2 );
            InitVectorAll(total_z_velocity, 0.0);

         /*----------------------------------------------------------------
          * Allocate and set up edge permeabilities
          *----------------------------------------------------------------*/
            z_permeability = NewVector( z_grid, 1, 2 );
            InitVectorAll(z_permeability, 0.0);

            PFModuleInvoke(void, permeability_face,
			   (z_permeability, 
			    ProblemDataPermeabilityZ(problem_data)));

	  }

         /*****************************************************************/
         /*          Print out any of the requested initial data          */
         /*****************************************************************/

         any_file_dumped = 0;

         /*----------------------------------------------------------------
          * Print out the initial saturations?
          *----------------------------------------------------------------*/

         if ( dump_files && is_multiphase ) 
	 {
	    for(phase = 0; phase < ProblemNumPhases(problem); phase++)
	    {
	       sprintf(file_postfix, "satur.%01d.%05d", phase, file_number );
	       if ( print_satur )
	       {
		  WritePFBinary(file_prefix, file_postfix, saturations[phase] );
		  any_file_dumped = 1;
	       }

	       if ( public_xtra -> write_silo_satur )
	       {
		  WriteSilo(file_prefix, file_postfix, saturations[phase], 
			    t, file_number, "Saturation");
		  any_file_dumped = 1;
	       }
	    }
	 }
	 
         /*----------------------------------------------------------------
          * Print out the initial concentrations?
          *----------------------------------------------------------------*/

         if ( dump_files )
         {
            if ( ProblemNumContaminants(problem) > 0 )
            {
               indx = 0;
               for(phase = 0; phase < ProblemNumPhases(problem); phase++)
               {
                  for(concen = 0; concen < ProblemNumContaminants(problem); concen++)
                  {

		     sprintf(file_postfix, "concen.%01d.%02d.%05d", phase, concen, file_number);
		     if ( print_concen ) 
		     {
			WritePFSBinary(file_prefix, file_postfix, 
				       concentrations[indx], drop_tol);

			any_file_dumped = 1;
		     }

		     if ( public_xtra -> write_silo_concen ) 
		     {
			WriteSilo(file_prefix, file_postfix, concentrations[indx],
				  t, file_number, "Concentration");
			any_file_dumped = 1;
		     }
		  
		     indx++;		     
		  }
	       }
            }
         }

         /*----------------------------------------------------------------
          * Print out the initial well data?
          *----------------------------------------------------------------*/

         if ( print_wells && dump_files )
         {
            WriteWells(file_prefix,
                       problem,
                       ProblemDataWellData(problem_data),
                       t, 
                       WELLDATA_WRITEHEADER);
         }

         /*----------------------------------------------------------------
          * Log this step
          *----------------------------------------------------------------*/

         IfLogging(1)
         {
            seq_log[number_logged]       = iteration_number;
            time_log[number_logged]      = t;
            dt_log[number_logged]        = dt;
            dt_info_log[number_logged]   = 'i';
            if ( any_file_dumped )
               dumped_log[number_logged] = file_number;
            else
               dumped_log[number_logged] = -1;
            if ( recompute_pressure )
               recomp_log[number_logged] = 'y';
            else
               recomp_log[number_logged] = 'n';
            number_logged++;
         }

         if ( any_file_dumped ) file_number++;
      }
   }
   else
   {
      file_number = 1;

      /*-------------------------------------------------------------------
       * Print out the initial well data?
       *-------------------------------------------------------------------*/

      if ( print_wells )
      {
         WriteWells(file_prefix,
                    problem,
                    ProblemDataWellData(problem_data),
                    t, 
                    WELLDATA_WRITEHEADER);
      }
   }

   /***********************************************************************/
   /*                                                                     */
   /*                Begin the main computational section                 */
   /*                                                                     */
   /***********************************************************************/

   do
   {
      if ( recompute_pressure )
      {

         /******************************************************************/
         /*                  Compute the total mobility                    */
	/*******************************************************************/
         InitVectorAll(total_mobility_x, 0.0);
         InitVectorAll(total_mobility_y, 0.0);
         InitVectorAll(total_mobility_z, 0.0);

         PFModuleInvoke(void, phase_mobility,
                        (total_mobility_x,
			 total_mobility_y,
			 total_mobility_z,
                         ProblemDataPermeabilityX(problem_data),
                         ProblemDataPermeabilityY(problem_data),
                         ProblemDataPermeabilityZ(problem_data),
                         0,
                         saturations[0],
                         ProblemPhaseViscosity(problem, 0)));

         for(phase = 1; phase < ProblemNumPhases(problem); phase++)
         {
            /* Get the mobility of this phase */
            PFModuleInvoke(void, phase_mobility,
                           (temp_mobility_x,
			    temp_mobility_y,
			    temp_mobility_z,
                            ProblemDataPermeabilityX(problem_data),
                            ProblemDataPermeabilityY(problem_data),
                            ProblemDataPermeabilityZ(problem_data),
                            phase,
                            saturations[phase],
                            ProblemPhaseViscosity(problem, phase)));

            Axpy(1.0, temp_mobility_x, total_mobility_x);
            Axpy(1.0, temp_mobility_y, total_mobility_y);
            Axpy(1.0, temp_mobility_z, total_mobility_z);
         }

         handle = InitVectorUpdate(total_mobility_x, VectorUpdateAll);
         FinalizeVectorUpdate(handle);

         handle = InitVectorUpdate(total_mobility_y, VectorUpdateAll);
         FinalizeVectorUpdate(handle);

         handle = InitVectorUpdate(total_mobility_z, VectorUpdateAll);
         FinalizeVectorUpdate(handle);

         /******************************************************************/
         /*                  Solve for the base pressure                   */
         /******************************************************************/
         /* Discretize and solve the pressure equation */
         PFModuleInvoke(void, discretize_pressure, (&A, &f, problem_data, 
						    t, total_mobility_x,
						    total_mobility_y,
						    total_mobility_z,
						    saturations));
         PFModuleInvoke(void, diag_scale, (pressure, A, f, DO));
         PFModuleReNewInstance(linear_solver, (NULL, NULL, problem_data, A, 
					       NULL));
         PFModuleInvoke(void, linear_solver, (pressure, f, abs_tol, 0));
         PFModuleInvoke(void, diag_scale, (pressure, A, f, UNDO));

         if ( transient )
         {
            /***************************************************************/
            /*                      Compute the velocities                 */
            /***************************************************************/
            for(phase = 0; phase < ProblemNumPhases(problem); phase++)
            {
               /* Compute the velocity for this phase */
               PFModuleInvoke(void, phase_velocity_face,
                              (phase_x_velocity[phase],
                               phase_y_velocity[phase],
                               phase_z_velocity[phase],
                               problem_data,
                               pressure,
                               saturations,
                               phase));

               phase_maximum = MaxPhaseFieldValue(phase_x_velocity[phase],
                                                  phase_y_velocity[phase],
                                                  phase_z_velocity[phase],
                                                  ProblemDataPorosity(
							    problem_data));

	       /* Put in a check for a possibly 0 velocity in this phase */
	       if (phase_maximum != 0.0)
		 phase_dt[phase] = CFL / phase_maximum;
	       else
		 phase_dt[phase] = (stop_time - t);

               if ( phase == 0 )
               {
                  min_phase_dt = phase_dt[0];
               }
               else
               {
                  if ( phase_dt[phase] < min_phase_dt ) 
                  {
                     min_phase_dt = phase_dt[phase];
                  }
               }

            }

            /* Compute the total velocity */
	    if ( is_multiphase )
	    {
               PFModuleInvoke(void, total_velocity_face,
			      (total_x_velocity,
			       total_y_velocity,
			       total_z_velocity,
			       problem_data,
			       total_mobility_x,
			       total_mobility_y,
			       total_mobility_z,
			       pressure,
			       saturations));

               total_maximum = MaxTotalFieldValue(problem, eval_struct,
						  saturations[0],
						  total_x_velocity,
						  total_y_velocity,
						  total_z_velocity,
						  z_permeability,
						  ProblemDataPorosity(
							    problem_data));

	       /* Put in a check for a possibly 0 velocity in all phases */
	       if (total_maximum != 0.0)
		 total_dt = CFL / total_maximum;
	       else
		 total_dt = stop_time - t;
	    }

            /**************************************************************/
            /*                       Print the pressure                   */
            /**************************************************************/

            /* Dump the pressure values at this time-step */
            if ( print_press )
            {
               sprintf(file_postfix, "press.%05d", file_number - 1);
               WritePFBinary(file_prefix, file_postfix, pressure);
            }

	    if(public_xtra -> write_silo_press) 
	    {
	       sprintf(file_postfix, "press.%05d", file_number - 1);
	       WriteSilo(file_prefix, file_postfix, pressure,
			 t, file_number - 1, "Pressure");
	       any_file_dumped = 1;
	    }


            if ( print_velocities )
            {
               for(phase = 0; phase < ProblemNumPhases(problem); phase++)
               {
                 sprintf(file_postfix, "phasex.%01d.%05d", phase, file_number - 1);
                 WritePFBinary(file_prefix, file_postfix, phase_x_velocity[phase]);

                 sprintf(file_postfix, "phasey.%01d.%05d", phase, file_number - 1);
                 WritePFBinary(file_prefix, file_postfix, phase_y_velocity[phase]);

                 sprintf(file_postfix, "phasez.%01d.%05d", phase, file_number - 1);
                 WritePFBinary(file_prefix, file_postfix, phase_z_velocity[phase]);
               }

               if ( is_multiphase )
               {
                  sprintf(file_postfix, "totalx.%05d", file_number - 1);
                  WritePFBinary(file_prefix, file_postfix, total_x_velocity);

                  sprintf(file_postfix, "totaly.%05d", file_number - 1);
                  WritePFBinary(file_prefix, file_postfix, total_y_velocity);

                  sprintf(file_postfix, "totalz.%05d", file_number - 1);
                  WritePFBinary(file_prefix, file_postfix, total_z_velocity);
               }
            }
         }
      }

      if ( transient )
      {

         iteration_number++;

         if ( iteration_number >= max_iterations )
         {
            still_evolving = 0;
         }

         /*----------------------------------------------------------------
          *
          * Get delta t's for all wells and boundary conditions.
          *
          *----------------------------------------------------------------*/

          well_dt = TimeCycleDataComputeNextTransition(problem,
                                 t,
                                 WellDataTimeCycleData(ProblemDataWellData(
							     problem_data)));

          bc_dt = TimeCycleDataComputeNextTransition(problem,
                      t,
                      BCPressureDataTimeCycleData(ProblemDataBCPressureData(
							     problem_data)));

         /*----------------------------------------------------------------
          *
          * Compute the new dt value based on the dt's from the velocity
          * field computations and any time stepping criterion imposed
          * by the user.
          *
          * Modified by RDF
          *
          *----------------------------------------------------------------*/

         {
	    if ( is_multiphase )
	    {
	       dt = min(total_dt, min_phase_dt);
	    }
            else
	    {
               dt = phase_dt[0];
	    }

            if ( well_dt < 0.0 )
            {
               well_dt = dt;
            }

            if ( bc_dt < 0.0 )
            {
               bc_dt = dt;
            }
   
            /*-------------------------------------------------------------
             * Determine what needs to be computed/evolved on this
             * iteration.  Indicate what determined the value of `dt'.
             *-------------------------------------------------------------*/

            if ( is_multiphase )
            {
               if ( dt == total_dt )
               {
                  recompute_pressure    = 1;
                  evolve_saturations    = 1;
                  evolve_concentrations = 1;

                  dt_info = 's';
               }
               else if (dt > well_dt)
               {
                  dt = well_dt;

                  recompute_pressure    = 1;
                  evolve_saturations    = 1;
                  evolve_concentrations = 1;

                  dt_info = 'w';
               }
               else if (dt > bc_dt)
               {
                  dt = bc_dt;

                  recompute_pressure    = 1;
                  evolve_saturations    = 1;
                  evolve_concentrations = 1;

                  dt_info = 'b';
               }
               else
               {
                  recompute_pressure    = 0;
                  evolve_saturations    = 0;
                  evolve_concentrations = 1;

                  dt_info = 'c';
               }
	    }
            else
	    {
               if (dt > well_dt)
               {
                  dt = well_dt;

                  recompute_pressure    = 1;
                  evolve_concentrations = 1;

                  dt_info = 'w';
               }
               else if (dt > bc_dt)
               {
                  dt = bc_dt;

                  recompute_pressure    = 1;
                  evolve_concentrations = 1;

                  dt_info = 'b';
               }
               else
               {
                  recompute_pressure = 0;
                  evolve_concentrations = 1;

                  dt_info = 'c';
               }
	    }

            /*------------------------------------------------------------
             * If we are printing out results, then determine if we need
             * to print them after this time step.
             *
             * If we are dumping output at real time intervals, the value
             * of dt may be changed.  If this happens, we want to
             * compute/evolve all values.  We also set `dump_info' to `p'
             * to indicate that the dump interval decided the time step for
             * this iteration.
             *------------------------------------------------------------*/

            if ( print_press || print_velocities || print_satur || print_concen || print_wells )
            {
               dump_files = 0;

               if ( dump_interval > 0 )
               {
                  print_dt = start_time + dump_index*dump_interval - t;

                  if ( dt >= print_dt )
                  {
                     dt = print_dt;

                     if ( is_multiphase )
                     {
                        recompute_pressure = 1;
                        evolve_saturations = 1;
                     }
                     else
                     {
			/* SGS recompute_pressure = 0;*/
                     }

                     evolve_concentrations = 1;

                     dt_info = 'p';

                     dump_files = 1;
                     dump_index++;
                  }
               }
               else if ( dump_interval < 0)
               {
                  if ( (iteration_number % (-(int)dump_interval)) == 0 )
                  {
                     dump_files = 1;
                  }
               }
	       else
	       {
		  dump_files = 0;
	       }
            }

            /*-------------------------------------------------------------
             * If this is the last iteration, set appropriate variables.
             *-------------------------------------------------------------*/

            if ( (t + dt) >= stop_time )
            {
               still_evolving = 0;

               dt = stop_time - t;
               dt_info = 'f';
               recompute_pressure    = 0;
               evolve_saturations    = 1;
               evolve_concentrations = 1;
            }

            t += dt;
         }

         any_file_dumped = 0;

         /******************************************************************/
         /*         Solve for and print the saturations                    */
         /******************************************************************/

         /* Solve for the saturation values at this time-step 
	    if necessary. */

         if ( is_multiphase )
	 {
	    if ( evolve_saturations )
            {
               for(phase = 0; phase < ProblemNumPhases(problem)-1; phase++)
               {
                  InitVectorAll(stemp, 0.0);
		  Copy(saturations[phase], stemp);
		  PFModuleInvoke(void, bc_phase_saturation,
				 (stemp, phase, gr_domain));


		  /* Evolve to the new time */
		  PFModuleInvoke(void, advect_satur,
				 (problem_data, phase,
				  stemp, saturations[phase],
				  total_x_velocity, 
				  total_y_velocity, 
				  total_z_velocity, 
				  z_permeability,
				  ProblemDataPorosity(problem_data),
				  ProblemPhaseViscosities(problem),
				  phase_densities,
				  ProblemGravity(problem),
				  t, dt, sadvect_order));
               }
	       InitVectorAll(saturations[ProblemNumPhases(problem)-1], 0.0);
	       PFModuleInvoke(void, constitutive, (saturations));

	       handle = InitVectorUpdate(saturations[
					 ProblemNumPhases(problem)-1], 
					 VectorUpdateGodunov);
	       FinalizeVectorUpdate(handle);

	       /* Print the saturation values at this time-step? */
	       if ( dump_files )
               {
                  for(phase = 0; phase < ProblemNumPhases(problem); phase++)
                  {
		     sprintf(file_postfix, "satur.%01d.%05d", phase, 
			     file_number );

		     if ( print_satur )
		     {
			WritePFBinary(file_prefix, file_postfix,
				      saturations[phase] );
			any_file_dumped = 1;
		     }

		     if( public_xtra -> write_silo_satur )
		     {
			WriteSilo(file_prefix, file_postfix, saturations[phase], 
				  t, file_number, "Saturation");
			any_file_dumped = 1;
		     }
		     
		  }
               }
            }
	 }

         /******************************************************************/
         /*            Solve for and print the concentrations              */
         /******************************************************************/

         if ( evolve_concentrations )
         {
            if ( ProblemNumContaminants(problem) > 0 )
            {
               /* Solve for the concentration values at this time-step. */
               indx = 0;
               for(phase = 0; phase < ProblemNumPhases(problem); phase++)
               {
                  for(concen = 0; concen < ProblemNumContaminants(problem); concen++)
                  {
                     PFModuleInvoke(void, retardation,
                                    (solidmassfactor,
                                     concen,
                                     problem_data));
                     handle = InitVectorUpdate(solidmassfactor, VectorUpdateAll2);
                     FinalizeVectorUpdate(handle);

                     InitVectorAll(ctemp, 0.0);
                     Copy(concentrations[indx], ctemp);

                     PFModuleInvoke(void, advect_concen,
                                    (problem_data, phase, concen,
                                     ctemp, concentrations[indx],
                                     phase_x_velocity[phase], 
                                     phase_y_velocity[phase], 
                                     phase_z_velocity[phase],
                                     solidmassfactor,
                                     t, dt, advect_order));
                     indx++;

                  }
               }
            }

            /* Print the concentration values at this time-step? */
            if ( dump_files )
            {
               if ( ProblemNumContaminants(problem) > 0 )
               {
                  indx = 0;
                  for(phase = 0; phase < ProblemNumPhases(problem); phase++)
                  {
                     for(concen = 0; concen < ProblemNumContaminants(problem); concen++)
                     {
                        sprintf(file_postfix, "concen.%01d.%02d.%05d", phase, concen, file_number);
			if ( print_concen ) { 
			   WritePFSBinary(file_prefix, file_postfix, 
					  concentrations[indx], drop_tol);
			   any_file_dumped = 1;
			}

			if ( public_xtra -> write_silo_concen )
			{
			   WriteSilo(file_prefix, file_postfix, concentrations[indx], 
				     t, file_number, "Concentration");
			   any_file_dumped = 1;
			}

                        indx++;
                     }
                  }
               }

            }
         }

         /******************************************************************/
         /*                  Print the Well Data                           */
         /******************************************************************/

         if ( evolve_concentrations || evolve_saturations )
         {
            /* Print the well data values at this time-step? */
            if ( print_wells && dump_files )
            {
               WriteWells(file_prefix,
                          problem,
                          ProblemDataWellData(problem_data),
                          t, 
			  WELLDATA_DONTWRITEHEADER);
            }
         }

         /*----------------------------------------------------------------
          * Log this step
          *----------------------------------------------------------------*/

         IfLogging(1)
         {
            seq_log[number_logged]       = iteration_number;
            time_log[number_logged]      = t;
            dt_log[number_logged]        = dt;
            dt_info_log[number_logged]   = dt_info;
            if ( any_file_dumped )
               dumped_log[number_logged] = file_number;
            else
               dumped_log[number_logged] = -1;
            if ( recompute_pressure )
               recomp_log[number_logged] = 'y';
            else
               recomp_log[number_logged] = 'n';
            number_logged++;
         }

         if ( any_file_dumped ) file_number++;

      }
      else
      {
         if ( print_press )
         {
            sprintf(file_postfix, "press" );
            WritePFBinary(file_prefix, file_postfix, pressure);
         }

	 if(public_xtra -> write_silo_press) 
	 {
            sprintf(file_postfix, "press" );
	    WriteSilo(file_prefix, file_postfix, pressure,
		      t, file_number, "Pressure");
	 }
      }
   }
   while(still_evolving);

   /***************************************************************/
   /*                 Print the pressure and saturation           */
   /***************************************************************/

   /* Dump the pressure values at end if requested */
   if( ProblemDumpAtEnd(problem) )
   {
      if(public_xtra -> print_press) {
	 sprintf(file_postfix, "press.%05d", file_number);
	 WritePFBinary(file_prefix, file_postfix, pressure);
	 any_file_dumped = 1;
      }
      
      if(public_xtra -> write_silo_press) 
      {
	 sprintf(file_postfix, "press.%05d", file_number);
	 WriteSilo(file_prefix, file_postfix, pressure,
		   t, file_number, "Pressure");
	 any_file_dumped = 1;
      }

      for(phase = 0; phase < ProblemNumPhases(problem); phase++)
      {
	 sprintf(file_postfix, "satur.%01d.%05d", phase, file_number );
	 if ( print_satur )
	 {
	    WritePFBinary(file_prefix, file_postfix, saturations[phase] );
		  any_file_dumped = 1;
	 }

	 if ( public_xtra -> write_silo_satur )
	 {
	    WriteSilo(file_prefix, file_postfix, saturations[phase], 
		      t, file_number, "Saturation");
	    any_file_dumped = 1;
	 }
      }


      if ( ProblemNumContaminants(problem) > 0 )
      {
	 indx = 0;
	 for(phase = 0; phase < ProblemNumPhases(problem); phase++)
	 {
	    for(concen = 0; concen < ProblemNumContaminants(problem); concen++)
	    {
	       
	       sprintf(file_postfix, "concen.%01d.%02d.%05d", phase, concen, file_number);
	       if ( print_concen ) 
	       {
		  WritePFSBinary(file_prefix, file_postfix, 
				 concentrations[indx], drop_tol);
		  
		  any_file_dumped = 1;
	       }
	       
	       if ( public_xtra -> write_silo_concen ) 
	       {
		  WriteSilo(file_prefix, file_postfix, concentrations[indx],
			    t, file_number, "Concentration");
		  any_file_dumped = 1;
	       }
	       
	       indx++;		     
	    }
	 }
      }
   }
 
   if ( transient )
   {
      free(phase_dt);

      if ( is_multiphase )
      {
         FreeVector(z_permeability);

         FreeVector(total_z_velocity);
         FreeVector(total_y_velocity);
         FreeVector(total_x_velocity);

         FreeEvalStruct(eval_struct);
      }

      for(phase = 0; phase < ProblemNumPhases(problem); phase++)
      {
         FreeVector(phase_z_velocity[phase]);
      }
      tfree(phase_z_velocity);

      for(phase = 0; phase < ProblemNumPhases(problem); phase++)
      {
         FreeVector(phase_y_velocity[phase]);
      }
      tfree(phase_y_velocity);

      for(phase = 0; phase < ProblemNumPhases(problem); phase++)
      {
         FreeVector(phase_x_velocity[phase]);
      }
      tfree(phase_x_velocity);

      if ( ProblemNumContaminants(problem) > 0 )
      {
         indx = 0;
         for(phase = 0; phase < ProblemNumPhases(problem); phase++)
         {
            for(concen = 0; concen < ProblemNumContaminants(problem); concen++)
            {
               FreeVector( concentrations[indx] );
               indx++;
            }
         }
         tfree(concentrations);

         FreeVector(solidmassfactor);
      }

   }

   if ( is_multiphase )
   {
      for(phase = 0; phase < ProblemNumPhases(problem); phase++)
      {
         FreeVector( saturations[phase] );
      }
   }
   tfree( saturations );
   tfree(phase_densities);


   /*-------------------------------------------------------------------
    * Free temp vectors
    *-------------------------------------------------------------------*/
   if ( is_multiphase )
   {
      FreeVector(stemp);
      FreeVector(temp_mobility_x);
      FreeVector(temp_mobility_y);
      FreeVector(temp_mobility_z);
   }
   FreeVector(ctemp);

   FreeVector( total_mobility_x );
   FreeVector( total_mobility_y );
   FreeVector( total_mobility_z );
   FreeVector( pressure );

   if(!amps_Rank(amps_CommWorld))
   {
      PrintWellData(ProblemDataWellData(problem_data), 
		    (WELLDATA_PRINTSTATS));
   }

   /*----------------------------------------------------------------------
    * Print log
    *----------------------------------------------------------------------*/

   IfLogging(1)
   {
      FILE*  log_file;
      int        k;

      log_file = OpenLogFile("SolverImpes");

      if ( transient )
      {
         fprintf(log_file, "Transient Problem Solved.\n");
         fprintf(log_file, "-------------------------\n");
         fprintf(log_file, "Sequence #       Time         \\Delta t         Dumpfile #   Recompute?\n");
         fprintf(log_file, "----------   ------------   ------------ -     ----------   ----------\n");

         for (k = 0; k < number_logged; k++)
         {
            if ( dumped_log[k] == -1 )
               fprintf(log_file, "  %06d     %8e   %8e %1c                       %1c\n",
                            k, time_log[k], dt_log[k], dt_info_log[k], recomp_log[k]);
            else
               fprintf(log_file, "  %06d     %8e   %8e %1c       %06d          %1c\n",
                            k, time_log[k], dt_log[k], dt_info_log[k], dumped_log[k], recomp_log[k]);
         }
      }
      else
      {
         fprintf(log_file, "Non-Transient Problem Solved.\n");
         fprintf(log_file, "-----------------------------\n");
      }

      CloseLogFile(log_file);

      tfree(seq_log);
      tfree(time_log);
      tfree(dt_log);
      tfree(dt_info_log);
      tfree(dumped_log);
      tfree(recomp_log);
   }

}

/*-------------------------------------------------------------------------
 * SolverImpesInitInstanceXtra
 *-------------------------------------------------------------------------*/

PFModule *SolverImpesInitInstanceXtra()
{
   PFModule      *this_module   = ThisPFModule;
   PublicXtra    *public_xtra   = PFModulePublicXtra(this_module);
   InstanceXtra  *instance_xtra;

   Problem      *problem = (public_xtra -> problem);

   Grid         *grid;
   Grid         *grid2d;
   Grid         *x_grid;
   Grid         *y_grid;
   Grid         *z_grid;

   SubgridArray *new_subgrids;
   SubgridArray *all_subgrids, *new_all_subgrids;

   Subgrid      *subgrid, *new_subgrid;

   double       *temp_data, *temp_data_placeholder;
   // SGS TODO total_mobility_sz is not being set anywhere so initialized to 0 here.
   int           total_mobility_sz = 0;
   int           pressure_sz, velocity_sz, satur_sz, 
                 concen_sz, temp_data_size, sz;
   int           is_multiphase;

   int           i;


   is_multiphase = ProblemNumPhases(problem) > 1;

   if ( PFModuleInstanceXtra(this_module) == NULL )
      instance_xtra = ctalloc(InstanceXtra, 1);
   else
      instance_xtra = PFModuleInstanceXtra(this_module);

   /*-------------------------------------------------------------------
    * Create the grids
    *-------------------------------------------------------------------*/

   /* Create the flow grid */
   grid = CreateGrid(GlobalsUserGrid);
   
   /*sk: Create a two-dimensional grid for later use*/
 
   all_subgrids = GridAllSubgrids(grid);
 
   new_all_subgrids = NewSubgridArray();
   ForSubgridI(i, all_subgrids)
   {
      subgrid = SubgridArraySubgrid(all_subgrids, i);
      new_subgrid = DuplicateSubgrid(subgrid);
      SubgridNZ(new_subgrid) = 1;
      AppendSubgrid(new_subgrid, new_all_subgrids);
   }
   new_subgrids  = GetGridSubgrids(new_all_subgrids);
   grid2d        = NewGrid(new_subgrids, new_all_subgrids);
   CreateComputePkgs(grid2d);
 
   /* Create the x velocity grid */

   all_subgrids = GridAllSubgrids(grid);

   /***** Set up a new subgrid grown by one in the x-direction *****/

   new_all_subgrids = NewSubgridArray();
   ForSubgridI(i, all_subgrids)
   {
      subgrid = SubgridArraySubgrid(all_subgrids, i);
      new_subgrid = DuplicateSubgrid(subgrid);
      SubgridNX(new_subgrid) += 1;
      AppendSubgrid(new_subgrid, new_all_subgrids);
   }
   new_subgrids  = GetGridSubgrids(new_all_subgrids);
   x_grid        = NewGrid(new_subgrids, new_all_subgrids);
   CreateComputePkgs(x_grid);

   /* Create the y velocity grid */

   all_subgrids = GridAllSubgrids(grid);

   /***** Set up a new subgrid grown by one in the y-direction *****/

   new_all_subgrids = NewSubgridArray();
   ForSubgridI(i, all_subgrids)
   {
      subgrid = SubgridArraySubgrid(all_subgrids, i);
      new_subgrid = DuplicateSubgrid(subgrid);
      SubgridNY(new_subgrid) += 1;
      AppendSubgrid(new_subgrid, new_all_subgrids);
   }
   new_subgrids  = GetGridSubgrids(new_all_subgrids);
   y_grid        = NewGrid(new_subgrids, new_all_subgrids);
   CreateComputePkgs(y_grid);

   /* Create the z velocity grid */

   all_subgrids = GridAllSubgrids(grid);

   /***** Set up a new subgrid grown by one in the z-direction *****/

   new_all_subgrids = NewSubgridArray();
   ForSubgridI(i, all_subgrids)
   {
      subgrid = SubgridArraySubgrid(all_subgrids, i);
      new_subgrid = DuplicateSubgrid(subgrid);
      SubgridNZ(new_subgrid) += 1;
      AppendSubgrid(new_subgrid, new_all_subgrids);
   }
   new_subgrids  = GetGridSubgrids(new_all_subgrids);
   z_grid        = NewGrid(new_subgrids, new_all_subgrids);
   CreateComputePkgs(z_grid);

   (instance_xtra -> grid) = grid;
   (instance_xtra -> grid2d) = grid2d;
   (instance_xtra -> x_grid) = x_grid;
   (instance_xtra -> y_grid) = y_grid;
   (instance_xtra -> z_grid) = z_grid;

   /*-------------------------------------------------------------------
    * Create problem_data
    *-------------------------------------------------------------------*/

   (instance_xtra -> problem_data) = NewProblemData(grid,grid2d);
   

   /*-------------------------------------------------------------------
    * Initialize module instances
    *-------------------------------------------------------------------*/

   if ( PFModuleInstanceXtra(this_module) == NULL )
   {
      (instance_xtra -> discretize_pressure) =
	 PFModuleNewInstance((public_xtra -> discretize_pressure),
			     (problem, grid, NULL));
      (instance_xtra -> diag_scale) =
	 PFModuleNewInstance((public_xtra -> diag_scale),
                 (grid));
      (instance_xtra -> linear_solver) =
	 PFModuleNewInstance((public_xtra -> linear_solver),
			     (problem, grid, NULL, NULL, NULL));
      (instance_xtra -> phase_velocity_face) =
	 PFModuleNewInstance((public_xtra -> phase_velocity_face),
                 (problem, grid, x_grid, y_grid, z_grid, NULL));
      (instance_xtra -> advect_concen) =
	 PFModuleNewInstance((public_xtra -> advect_concen),
                 (problem, grid, NULL));
      (instance_xtra -> set_problem_data) =
	 PFModuleNewInstance((public_xtra -> set_problem_data),
			     (problem, grid, NULL));

      (instance_xtra -> retardation) =
	 PFModuleNewInstance(ProblemRetardation(problem), (NULL));
      (instance_xtra -> phase_mobility) =
	 PFModuleNewInstance(ProblemPhaseMobility(problem), ());
      (instance_xtra -> ic_phase_concen) =
	 PFModuleNewInstance(ProblemICPhaseConcen(problem), ());
      (instance_xtra -> phase_density) =
	 PFModuleNewInstance(ProblemPhaseDensity(problem), ());


      if ( is_multiphase )
      {
         (instance_xtra -> permeability_face) =
	   PFModuleNewInstance((public_xtra -> permeability_face),
			       (z_grid));
	 (instance_xtra -> total_velocity_face) =
	   PFModuleNewInstance((public_xtra -> total_velocity_face),
			       (problem, grid, x_grid, y_grid, z_grid, 
				NULL));
	 (instance_xtra -> advect_satur) =
	   PFModuleNewInstance((public_xtra -> advect_satur),
			       (problem, grid, NULL));

	 (instance_xtra -> ic_phase_satur) =
	   PFModuleNewInstance(ProblemICPhaseSatur(problem), ());
	 (instance_xtra -> bc_phase_saturation) =
	   PFModuleNewInstance(ProblemBCPhaseSaturation(problem), ());
	 (instance_xtra -> constitutive) =
	   PFModuleNewInstance(ProblemSaturationConstitutive(problem), 
			       (grid));
      }         
   }
   else
   {
      PFModuleReNewInstance((instance_xtra -> discretize_pressure),
			    (problem, grid, NULL));
      PFModuleReNewInstance((instance_xtra -> diag_scale),
			    (grid));
      PFModuleReNewInstance((instance_xtra -> linear_solver),
			    (problem, grid, NULL, NULL, NULL));
      PFModuleReNewInstance((instance_xtra -> phase_velocity_face),
                (problem, grid, x_grid, y_grid, z_grid, NULL));
      PFModuleReNewInstance((instance_xtra -> advect_concen),
                (problem, grid, NULL));
      PFModuleReNewInstance((instance_xtra -> set_problem_data),
			    (problem, grid, NULL));

      PFModuleReNewInstance((instance_xtra -> retardation), (NULL));
      PFModuleReNewInstance((instance_xtra -> phase_mobility), ());
      PFModuleReNewInstance((instance_xtra -> ic_phase_concen), ());
      PFModuleReNewInstance((instance_xtra -> phase_density), ());

      if ( is_multiphase )
      {
         PFModuleReNewInstance((instance_xtra -> permeability_face),
			       (z_grid));
	 PFModuleReNewInstance((instance_xtra -> total_velocity_face),
			       (problem, grid, x_grid, y_grid, z_grid, 
				NULL));
	 PFModuleReNewInstance((instance_xtra -> advect_satur),
			       (problem, grid, NULL));
	 PFModuleReNewInstance((instance_xtra -> ic_phase_satur), ());
	 PFModuleReNewInstance((instance_xtra -> bc_phase_saturation), ());
	 PFModuleReNewInstance((instance_xtra -> constitutive), (grid));
      }         
   }

   /*-------------------------------------------------------------------
    * Set up temporary data
    *-------------------------------------------------------------------*/

   if ( is_multiphase )
   {

      /* compute size for total mobility computation */
     sz = 0;

     /* compute size for saturation advection */
     sz = 0;
     sz += PFModuleSizeOfTempData(instance_xtra -> advect_satur);
     satur_sz = sz;
   }

   /* compute size for pressure solve */
   sz = 0;
   sz = max(sz, 
	    PFModuleSizeOfTempData(instance_xtra -> discretize_pressure));
   sz = max(sz, PFModuleSizeOfTempData(instance_xtra -> linear_solver));
   pressure_sz = sz;

   /* compute size for velocity computation */
   sz = 0;
   sz = max(sz, 
	    PFModuleSizeOfTempData(instance_xtra -> phase_velocity_face));
   if ( is_multiphase )
   {
      sz = max(sz, 
	       PFModuleSizeOfTempData(instance_xtra -> total_velocity_face));
   }
   velocity_sz = sz;

   /* compute size for concentration advection */
   sz = 0;
   sz = max(sz, PFModuleSizeOfTempData(instance_xtra -> retardation));
   sz = max(sz, PFModuleSizeOfTempData(instance_xtra -> advect_concen));
   concen_sz = sz;

   /* set temp_data size to max of pressure_sz, satur_sz, and concen_sz*/
   temp_data_size = max(max(pressure_sz, velocity_sz), concen_sz);
   if ( is_multiphase )
   {
      temp_data_size = max(temp_data_size,max(total_mobility_sz, satur_sz));
   }
/*     temp_data_size = total_mobility_sz + pressure_sz + velocity_sz 
 *                      + satur_sz + concen_sz;  */

   /* allocate temporary data */
   temp_data = NewTempData(temp_data_size);
   (instance_xtra -> temp_data) = temp_data;

   /* renew set_problem_data module */
   PFModuleReNewInstance((instance_xtra -> set_problem_data),
                         (NULL, NULL, temp_data));

   /* renew pressure solve modules that take temporary data */
   PFModuleReNewInstance((instance_xtra -> discretize_pressure),
			 (NULL, NULL, temp_data));
/*   temp_data += 
            PFModuleSizeOfTempData(instance_xtra -> discretize_pressure);  */
   PFModuleReNewInstance((instance_xtra -> linear_solver),
			 (NULL, NULL, NULL, NULL, temp_data));
/*   temp_data += PFModuleSizeOfTempData(instance_xtra -> linear_solver);  */

   /* renew velocity computation modules that take temporary data */
   PFModuleReNewInstance((instance_xtra -> phase_velocity_face),
             (NULL, NULL, NULL, NULL, NULL, temp_data));
   if ( is_multiphase )
   {
     PFModuleReNewInstance((instance_xtra -> total_velocity_face),
			   (NULL, NULL, NULL, NULL, NULL, temp_data));
     /* temp_data += PFModuleSizeOfTempData(instance_xtra -> 
      *   	                            total_velocity_face);  */

      /* renew saturation advection modules that take temporary data */
      temp_data_placeholder = temp_data;
      PFModuleReNewInstance((instance_xtra -> advect_satur),
			    (NULL, NULL, temp_data_placeholder));
      temp_data_placeholder += 
	PFModuleSizeOfTempData(instance_xtra -> advect_satur);
   }

   /* renew concentration advection modules that take temporary data */
   temp_data_placeholder = temp_data;
   PFModuleReNewInstance((instance_xtra -> retardation),
             (temp_data_placeholder));
   PFModuleReNewInstance((instance_xtra -> advect_concen),
             (NULL, NULL, temp_data_placeholder));
   temp_data_placeholder += max(
		     PFModuleSizeOfTempData(instance_xtra -> retardation),
                     PFModuleSizeOfTempData(instance_xtra -> advect_concen)
                               );
   /* set temporary vector data used for advection */

   temp_data += temp_data_size;

   PFModuleInstanceXtra(this_module) = instance_xtra;
   return this_module;
}

/*-------------------------------------------------------------------------
 * SolverImpesFreeInstanceXtra
 *-------------------------------------------------------------------------*/

void  SolverImpesFreeInstanceXtra()
{
   PFModule      *this_module   = ThisPFModule;
   InstanceXtra  *instance_xtra = PFModuleInstanceXtra(this_module);

   PublicXtra    *public_xtra   = PFModulePublicXtra(this_module);
   Problem       *problem       = (public_xtra -> problem);
   int           is_multiphase;

   is_multiphase = ProblemNumPhases(problem) > 1;

   if ( instance_xtra )
   {
      FreeTempData( (instance_xtra -> temp_data) );

      PFModuleFreeInstance((instance_xtra -> ic_phase_concen));
      PFModuleFreeInstance((instance_xtra -> phase_mobility));
      PFModuleFreeInstance((instance_xtra -> retardation));

      PFModuleFreeInstance((instance_xtra -> set_problem_data));
      PFModuleFreeInstance((instance_xtra -> advect_concen));
      PFModuleFreeInstance((instance_xtra -> phase_velocity_face));
      PFModuleFreeInstance((instance_xtra -> linear_solver));
      PFModuleFreeInstance((instance_xtra -> diag_scale));
      PFModuleFreeInstance((instance_xtra -> discretize_pressure));

      PFModuleFreeInstance((instance_xtra -> phase_density));

      if ( is_multiphase )
      {
         PFModuleFreeInstance((instance_xtra -> constitutive));
	 PFModuleFreeInstance((instance_xtra -> bc_phase_saturation));
	 PFModuleFreeInstance((instance_xtra -> ic_phase_satur));
	 PFModuleFreeInstance((instance_xtra -> advect_satur));
	 PFModuleFreeInstance((instance_xtra -> total_velocity_face));
	 PFModuleFreeInstance((instance_xtra -> permeability_face));
      }

      FreeProblemData((instance_xtra -> problem_data));

      FreeGrid((instance_xtra -> z_grid));
      FreeGrid((instance_xtra -> y_grid));
      FreeGrid((instance_xtra -> x_grid));
      FreeGrid((instance_xtra -> grid2d));
      FreeGrid((instance_xtra -> grid));

      tfree(instance_xtra);
   }
}

/*-------------------------------------------------------------------------
 * SolverImpesNewPublicXtra
 *-------------------------------------------------------------------------*/

PFModule   *SolverImpesNewPublicXtra(char *name)
{
   PFModule      *this_module   = ThisPFModule;
   PublicXtra    *public_xtra;
   
   char          *switch_name;
   int            switch_value;
   NameArray      switch_na;

   char key[IDB_MAX_KEY_LEN];

   NameArray diag_solver_na;
   NameArray linear_solver_na;

   switch_na = NA_NewNameArray("False True");

   public_xtra = ctalloc(PublicXtra, 1);

   diag_solver_na = NA_NewNameArray("NoDiagScale MatDiagScale");
   sprintf(key, "%s.DiagScale", name);
   switch_name = GetStringDefault(key,"NoDiagScale");
   switch_value  = NA_NameToIndex(diag_solver_na, switch_name);
   switch (switch_value)
   {
      case 0:
      {
	 (public_xtra -> diag_scale) = PFModuleNewModule(NoDiagScale, (key));
	 break;
      }
      case 1:
      {
	 (public_xtra -> diag_scale) = PFModuleNewModule(MatDiagScale, (key));
	 break;
      }
      default:
      {
	 InputError("Error: Invalid value <%s> for key <%s>\n", switch_name,
		     key);
      }
   }
   NA_FreeNameArray(diag_solver_na);

   linear_solver_na = NA_NewNameArray("MGSemi PPCG PCG CGHS");
   sprintf(key, "%s.Linear", name);
   switch_name = GetStringDefault(key, "PCG");
   switch_value  = NA_NameToIndex(linear_solver_na, switch_name);
   switch (switch_value)
   {
      case 0:
      {
	 (public_xtra -> linear_solver) = PFModuleNewModule(MGSemi, (key));
	 break;
      }
      case 1:
      {
	 (public_xtra -> linear_solver) = PFModuleNewModule(PPCG, (key));
	 break;
      }

      case 2:
      {
	 (public_xtra -> linear_solver) = PFModuleNewModule(PCG, (key));
	 break;
      }

      case 3:
      {
	 (public_xtra -> linear_solver) = PFModuleNewModule(CGHS, (key));
	 break;
      }
      default:
      {
	 InputError("Error: Invalid value <%s> for key <%s>\n", switch_name,
		     key);
      }
   }
   NA_FreeNameArray(linear_solver_na);
   
   (public_xtra -> discretize_pressure) =
      PFModuleNewModule(DiscretizePressure, ());

   (public_xtra -> permeability_face) = PFModuleNewModule(PermeabilityFace, 
							  ());
   (public_xtra -> phase_velocity_face) = PFModuleNewModule(
						      PhaseVelocityFace, ());
   (public_xtra -> total_velocity_face) = PFModuleNewModule(
						      TotalVelocityFace, ());
   (public_xtra -> advect_satur) = PFModuleNewModule(SatGodunov, ());
   (public_xtra -> advect_concen) = PFModuleNewModule(Godunov, ());
   (public_xtra -> set_problem_data) = PFModuleNewModule(SetProblemData, ());
   
   (public_xtra -> problem) = NewProblem(ImpesSolve);

   sprintf(key, "%s.SadvectOrder", name);
   public_xtra -> sadvect_order = GetIntDefault(key,2);

   sprintf(key, "%s.AdvectOrder", name);
   public_xtra -> advect_order = GetIntDefault(key,2);

   sprintf(key, "%s.CFL", name);
   public_xtra -> CFL = GetDoubleDefault(key, 0.7);

   sprintf(key, "%s.MaxIter", name);
   public_xtra -> max_iterations = GetIntDefault(key, 1000000);

   sprintf(key, "%s.RelTol", name);
   public_xtra -> rel_tol = GetDoubleDefault(key, 1.0);

   sprintf(key, "%s.AbsTol", name);
   public_xtra -> abs_tol = GetDoubleDefault(key, 1E-9);

   sprintf(key, "%s.Drop", name);
   public_xtra -> drop_tol = GetDoubleDefault(key, 1E-8);

   sprintf(key, "%s.PrintSubsurf", name);
   switch_name = GetStringDefault(key, "True");
   switch_value = NA_NameToIndex(switch_na, switch_name);
   if(switch_value < 0)
   {
	 InputError("Error: invalid print switch value <%s> for key <%s>\n",
		     switch_name, key);
   }
   public_xtra -> print_subsurf_data = switch_value;

   sprintf(key, "%s.PrintPressure", name);
   switch_name = GetStringDefault(key, "True");
   switch_value = NA_NameToIndex(switch_na, switch_name);
   if(switch_value < 0)
   {
	 InputError("Error: invalid print switch value <%s> for key <%s>\n",
		     switch_name, key );
   }
   public_xtra -> print_press = switch_value;

   sprintf(key, "%s.PrintVelocities", name);
   switch_name = GetStringDefault(key, "False");
   switch_value = NA_NameToIndex(switch_na, switch_name);
   if(switch_value < 0)
   {
	 InputError("Error: invalid print switch value <%s> for key <%s>\n",
		     switch_name, key );
   }
   public_xtra -> print_velocities = switch_value;


   sprintf(key, "%s.PrintSaturation", name);
   switch_name = GetStringDefault(key, "True");
   switch_value = NA_NameToIndex(switch_na, switch_name);
   if(switch_value < 0)
   {
	 InputError("Error: invalid print switch value <%s> for key <%s>\n",
		     switch_name, key );
   }
   public_xtra -> print_satur = switch_value;

   sprintf(key, "%s.PrintConcentration", name);
   switch_name = GetStringDefault(key, "True");
   switch_value = NA_NameToIndex(switch_na, switch_name);
   if(switch_value < 0)
   {
	 InputError("Error: invalid print switch value <%s> for key <%s>\n",
		     switch_name, key );
   }
   public_xtra -> print_concen = switch_value;

   sprintf(key, "%s.PrintWells", name);
   switch_name = GetStringDefault(key, "True");
   switch_value = NA_NameToIndex(switch_na, switch_name);
   if(switch_value < 0)
   {
	 InputError("Error: invalid print switch value <%s> for key <%s>\n",
		     switch_name, key);
   }
   public_xtra -> print_wells = switch_value;

   /* Silo file writing control */
   sprintf(key, "%s.WriteSiloSubsurfData", name);
   switch_name = GetStringDefault(key, "False");
   switch_value = NA_NameToIndex(switch_na, switch_name);
   if(switch_value < 0)
   {
      InputError("Error: invalid value <%s> for key <%s>\n",
      switch_name, key);
   }
   public_xtra -> write_silo_subsurf_data = switch_value;

   sprintf(key, "%s.WriteSiloPressure", name);
   switch_name = GetStringDefault(key, "False");
   switch_value = NA_NameToIndex(switch_na, switch_name);
   if(switch_value < 0)
   {
      InputError("Error: invalid value <%s> for key <%s>\n",
      switch_name, key );
   }
   public_xtra -> write_silo_press = switch_value;

   sprintf(key, "%s.WriteSiloVelocities", name);
   switch_name = GetStringDefault(key, "False");
   switch_value = NA_NameToIndex(switch_na, switch_name);
   if(switch_value < 0)
   {
      InputError("Error: invalid value <%s> for key <%s>\n",
      switch_name, key );
   }
   public_xtra -> write_silo_velocities = switch_value;

   sprintf(key, "%s.WriteSiloSaturation", name);
   switch_name = GetStringDefault(key, "False");
   switch_value = NA_NameToIndex(switch_na, switch_name);
   if(switch_value < 0)
   {
      InputError("Error: invalid value <%s> for key <%s>\n",
      switch_name, key);
   }
   public_xtra -> write_silo_satur = switch_value;

   sprintf(key, "%s.WriteSiloConcentration", name);
   switch_name = GetStringDefault(key, "False");
   switch_value = NA_NameToIndex(switch_na, switch_name);
   if(switch_value < 0)
   {
      InputError("Error: invalid value <%s> for key <%s>\n",
      switch_name, key );
   }
   public_xtra -> write_silo_concen = switch_value;

   if( public_xtra -> write_silo_subsurf_data || 
       public_xtra -> write_silo_press  ||
       public_xtra -> write_silo_velocities ||
       public_xtra -> write_silo_satur ||
       public_xtra -> write_silo_concen
      ) {
      WriteSiloInit(GlobalsOutFileName);
   }

   NA_FreeNameArray(switch_na);

   PFModulePublicXtra(this_module) = public_xtra;
   return this_module;
}

/*-------------------------------------------------------------------------
 * SolverImpesFreePublicXtra
 *-------------------------------------------------------------------------*/

void   SolverImpesFreePublicXtra()
{
   PFModule      *this_module = ThisPFModule;
   PublicXtra    *public_xtra = PFModulePublicXtra(this_module);

   if ( public_xtra )
   {
      FreeProblem(public_xtra -> problem, ImpesSolve);

      PFModuleFreeModule(public_xtra -> diag_scale);
      PFModuleFreeModule(public_xtra -> linear_solver);

      PFModuleFreeModule(public_xtra -> set_problem_data);
      PFModuleFreeModule(public_xtra -> advect_concen);
      PFModuleFreeModule(public_xtra -> advect_satur);
      PFModuleFreeModule(public_xtra -> total_velocity_face);
      PFModuleFreeModule(public_xtra -> phase_velocity_face);
      PFModuleFreeModule(public_xtra -> permeability_face);
      PFModuleFreeModule(public_xtra -> discretize_pressure);
      tfree( public_xtra );
   }
}

/*-------------------------------------------------------------------------
 * SolverImpesSizeOfTempData
 *-------------------------------------------------------------------------*/

int  SolverImpesSizeOfTempData()
{

   /* SGS temp data */

   return 0;
}
