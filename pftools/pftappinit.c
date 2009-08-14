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

/* 
 * pftappinit.c
 *
 * This file contains routines to add the PFTools commands to the Tcl
 * interpreter as well as start the appropriate version(command line
 * or GUI and command line).
 *
 */

#include "parflow_config.h"

#include <stdlib.h>

#ifndef _WIN32
#include <sys/param.h>
#endif

#include "pftools.h"
#include "tools_io.h"

#if defined(__WIN32__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif

/*
 * VC++ has an alternate entry point called DllMain, so we need to rename
 * our entry point.
 */

#if defined(_MSC_VER)
#define EXPORT(a,b) __declspec(dllexport) a b
#define DllEntryPoint DllMain
#else
#define EXPORT(a,b) a b
#endif

EXTERN EXPORT(int,Parflow_Init) (Tcl_Interp *interp);

/*
 *----------------------------------------------------------------------
 *
 * DllEntryPoint --
 *
 *	This wrapper function is used by Windows to invoke the
 *	initialization code for the DLL.  If we are compiling
 *	with Visual C++, this routine will be renamed to DllMain.
 *	routine.
 *
 * Results:
 *	Returns TRUE;
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef __WIN32__
BOOL APIENTRY
DllEntryPoint(hInst, reason, reserved)
    HINSTANCE hInst;		/* Library instance handle. */
    DWORD reason;		/* Reason this function is being called. */
    LPVOID reserved;		/* Not used. */
{
    return TRUE;
}
#endif


/* Pft_Init - This function is used to add the PFTools commands to Tcl as */
/*            an extension of the Tcl language.  The PFTools commands     */
/*            essentially become new Tcl commands executable from the Tcl */
/*            interpreter.                                                */
/*                                                                        */
/* Parameters - Tcl_Interp *interp                                        */
/*                                                                        */
/* Return value - TCL_OK if the PFTools data structures are initialized   */
/*                without error.                                          */
/*                                                                        */
/*                TCL_ERROR if the PFTools data structures cannot be      */
/*                allocated memory.                                       */

EXPORT(int,Parflow_Init)(Tcl_Interp *interp)
{
   Data *data;

   if ((data = InitPFToolsData()) == NULL) {
      Tcl_SetResult(interp, "Error: Could not initialize data structures for PFTools", TCL_STATIC);
      return TCL_ERROR;
   }

   /************************************************************************
     When you add commands here make sure you add the public ones to the
     pftools.tcl with the namespace export command 
     ***********************************************************************/
      
#if HAVE_HDF
   Tcl_CreateCommand(interp, "Parflow::pfloadsds", (Tcl_CmdProc *)LoadSDSCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfsavesds", (Tcl_CmdProc *)SaveSDSCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
#endif

   Tcl_CreateCommand(interp, "Parflow::pfbfcvel", (Tcl_CmdProc *)BFCVelCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfgetsubbox", (Tcl_CmdProc *)GetSubBoxCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfenlargebox", (Tcl_CmdProc *)EnlargeBoxCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfload", (Tcl_CmdProc *)LoadPFCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfreload", (Tcl_CmdProc *)ReLoadPFCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfdist", (Tcl_CmdProc *)PFDistCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfsave", (Tcl_CmdProc *)SavePFCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfgetelt", (Tcl_CmdProc *)GetEltCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfgridtype", (Tcl_CmdProc *)GridTypeCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfgetgrid", (Tcl_CmdProc *)GetGridCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfcvel", (Tcl_CmdProc *)CVelCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfvvel", (Tcl_CmdProc *)VVelCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfvmag", (Tcl_CmdProc *)VMagCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfhhead", (Tcl_CmdProc *)HHeadCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfphead", (Tcl_CmdProc *)PHeadCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfflux", (Tcl_CmdProc *)FluxCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfnewlabel", (Tcl_CmdProc *)NewLabelCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfaxpy", (Tcl_CmdProc *)AxpyCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfsum", (Tcl_CmdProc *)SumCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfgetstats", (Tcl_CmdProc *)GetStatsCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfmdiff", (Tcl_CmdProc *)MDiffCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfdiffelt", (Tcl_CmdProc *)DiffEltCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfsavediff", (Tcl_CmdProc *)SaveDiffCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfgetlist", (Tcl_CmdProc *)GetListCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfnewgrid", (Tcl_CmdProc *)NewGridCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfnewlabel", (Tcl_CmdProc *)NewLabelCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfdelete", (Tcl_CmdProc *)DeleteCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfcomputetop", (Tcl_CmdProc *)ComputeTopCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfextracttop", (Tcl_CmdProc *)ExtractTopCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfsurfacestorage", (Tcl_CmdProc *)SurfaceStorageCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfsubsurfacestorage", (Tcl_CmdProc *)SubsurfaceStorageCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfsurfacerunoff", (Tcl_CmdProc *)SurfaceRunoffCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(interp, "Parflow::pfwatertabledepth", (Tcl_CmdProc *)WaterTableDepthCommand,
                     (ClientData) data, (Tcl_CmdDeleteProc *) NULL);

#ifdef SGS
   Tcl_CreateExitHandler((Tcl_ExitProc *)PFTExitProc, (ClientData) data);
#endif

   Tcl_SetVar(interp, "tcl_prompt1", "puts -nonewline {pftools> }",
              TCL_GLOBAL_ONLY);

   return Tcl_PkgProvide(interp, "parflow", "1.0");
}




