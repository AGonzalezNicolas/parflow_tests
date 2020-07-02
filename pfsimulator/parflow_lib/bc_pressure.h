/*BHEADER*********************************************************************
* (c) 1995   The Regents of the University of California
*
* See the file COPYRIGHT_and_DISCLAIMER for a complete copyright
* notice, contact person, and disclaimer.
*
* $Revision: 1.1.1.1 $
*********************************************************************EHEADER*/

#ifndef _BC_PRESSURE_HEADER
#define _BC_PRESSURE_HEADER


/*----------------------------------------------------------------
 * Procedure for adding new Boundary Condition Types (@MCB: 06/24/2019)
 *
 * 1) Add the new type as a constant integer below, using the same
 *     name as it would appear in the TCL file.
 *
 * 2) Add two new struct definitions using the BC_TYPE_TABLE and
 *     BC_INTERVAL_TYPE_TABLE macros.  Do not include a comma between
 *     the previous and new definition.  The BC_TYPE macro takes two
 *     parameters: The type you defined in Step 1, and the members of
 *     the struct to be created, wrapped inside {}.
 *  NOTE: The order in which you defined the boundary condition types
 *     must match the order in which you define them in the tables! These
 *     are used at the beginning of BCPressurePackageNewPublicXtra to
 *     populate the name array that is later branched on.
 *     So if FluxConst is defined as 2, it must be the third (0 indexing)
 *     entry in both the tables.
 *
 * 3) Add new branches for the new type in the following functions:
 *      bc_pressure_pacakge.c:BCPressurePackage
 *      bc_pressure_pacakge.c:BCPressurePackageNewPublicXtra
 *      bc_pressure_pacakge.c:BCPressurePackageFreePublicXtra (if applicable)
 *      problem_bc_pressure:BCStruct
 *      And any other relevant files that will deal with this pressure type
 *
 * 4) If applicable, add a new #define type in problem_bc.h for branching
 *     inside BCStructPatchLoop calls.
 *-----------------------------------------------------------------*/




/*----------------------------------------------------------------
 * BCPressure Types
 * NOTE: These are not the values used to branch inside of
 *       BCStructPatchLoops, those are defined in problem_bc.h
 *----------------------------------------------------------------*/
/**
 * @name BCPressure Types
 *
 * @brief List of BCPressure type names as they appear in the TCL file
 *
 * @{
 */
#define DirEquilRefPatch 0
#define DirEquilPLinear  1
#define FluxConst        2
#define FluxVolumetric   3
#define PressureFile     4
#define FluxFile         5
#define ExactSolution    6
#define OverlandFlow     7
#define OverlandFlowPFB  8
#define SeepageFace      9
#define OverlandKinematic 10
#define OverlandDiffusive 11
/** @} */


/*----------------------------------------------------------------
 * Table to generate Type structs used in BCPressurePackage functions
 * These contain information for all interval steps to be read later
 * These structs will be local to the BCPressurePackage source file only
 *----------------------------------------------------------------*/
#define BC_TYPE_TABLE            \
  BC_TYPE(DirEquilRefPatch, {    \
    int reference_solid;         \
    int reference_patch;         \
    double *values;              \
    double **value_at_interface; \
  })                             \
  BC_TYPE(DirEquilPLinear, {     \
    double *xlower;              \
    double *ylower;              \
    double *xupper;              \
    double *yupper;              \
    int    *num_points;          \
    double **points;             \
    double **values;             \
    double **value_at_interface; \
  })                             \
  BC_TYPE(FluxConst, {           \
    double *values;              \
  })                             \
  BC_TYPE(FluxVolumetric, {      \
    double *values;              \
  })                             \
  BC_TYPE(PressureFile, {        \
    char **filenames;            \
  })                             \
  BC_TYPE(FluxFile, {            \
    char **filenames;            \
  })                             \
  BC_TYPE(ExactSolution, {       \
    int function_type;           \
  })                             \
  BC_TYPE(OverlandFlow, {        \
    double *values;              \
  })                             \
  BC_TYPE(OverlandFlowPFB, {     \
    char **filenames;            \
  })                             \
  BC_TYPE(SeepageFace, {         \
    double *values;              \
  })                             \
  BC_TYPE(OverlandKinematic, {   \
    double *values;              \
  })                             \
  BC_TYPE(OverlandDiffusive, {   \
    double *values;              \
  })


/**
 * @name TypeStruct Allocator
 * @brief Will allocate a new variable of the specified TypeStruct
 *
 * @param type Type of TypeStruct to allocate
 * @param var Variable name for newly declared and allocated TypeStruct pointer
 */
#define NewTypeStruct(type, var) \
  Type ## type * var = ctalloc(Type ## type, 1)

/**
 * @name TypeStruct Setter
 * @brief Stores an allocated TypeStruct pointer into public_xtra
 *
 * @param public_xtra The public_xtra pointer for the module
 * @param var Name of the variable to be stored (Should match name used in NewTypeStruct)
 * @param i Patch index
 */
#define StoreTypeStruct(public_xtra, var, i) \
  (public_xtra)->data[(i)] = (void*)(var);

/**
 * @name TypeStruct Accessor
 * @brief Declares and assigns a variable to the given TypeStruct type
 *
 * @param type Type of TypeStruct to cast to
 * @param var Name of the variable that will be declared
 * @param public_xtra Pointer to the modules public_xtra
 * @param i Patch index
 */
#define GetTypeStruct(type, var, public_xtra, i) \
  Type ## type * var = (Type ## type*)(public_xtra->data[i])


// MCB: These two macros aren't really necessary but they do make the code cleaner
#define ForEachPatch(num_patches, i) \
  for (i = 0; i < num_patches; i++)
#define ForEachInterval(interval_division, interval_number) \
  for (interval_number = 0; interval_number < interval_division; interval_number++)


/*----------------------------------------------------------------
 * BCPressure Values
 * These contain information for a particular interval
 * @MCB: These appear to consistently have the same layout as
 *       the type struct definitions, with one less reference level.
 *       Could use C++ std::remove_pointer to only need one table def.
 *----------------------------------------------------------------*/
#define BC_INTERVAL_TYPE_TABLE   \
  BC_TYPE(DirEquilRefPatch, {    \
    int reference_solid;         \
    int reference_patch;         \
    double value;                \
    double *value_at_interfaces; \
  })                             \
  BC_TYPE(DirEquilPLinear, {     \
    double xlower;               \
    double ylower;               \
    double xupper;               \
    double yupper;               \
    int num_points;              \
    double *points;              \
    double *values;              \
    double *value_at_interfaces; \
  })                             \
  BC_TYPE(FluxConst, {           \
    double value;                \
  })                             \
  BC_TYPE(FluxVolumetric, {      \
    double value;                \
  })                             \
  BC_TYPE(PressureFile, {        \
    char *filename;              \
  })                             \
  BC_TYPE(FluxFile, {            \
    char *filename;              \
  })                             \
  BC_TYPE(ExactSolution, {       \
    int function_type;           \
  })                             \
  BC_TYPE(OverlandFlow, {        \
    double value;                \
  })                             \
  BC_TYPE(OverlandFlowPFB, {     \
    char *filename;              \
  })                             \
  BC_TYPE(SeepageFace, {         \
    double value;                \
  })                             \
  BC_TYPE(OverlandKinematic, {   \
    double value;                \
  })                             \
  BC_TYPE(OverlandDiffusive, {   \
    double value;                \
  })


/* @MCB: Interval structs are used in multiple C files, generate the definitions in the header */
#define BC_TYPE(type, values) typedef struct values BCPressureType ## type;
BC_INTERVAL_TYPE_TABLE
#undef BC_TYPE


/**
 * @name BCPresssureType Allocator
 * @brief Allocates a new BCPressureType struct
 *
 * @param type BCPressureType to allocate
 * @param varname Name of the variable to declare and allocate
 */
#define NewBCPressureTypeStruct(type, varname) \
  BCPressureType ## type * varname = ctalloc(BCPressureType ## type, 1)

/**
 * @name BCPressureType Accessor
 * @brief Declares and assignes a variable to the specified BCPressureType struct
 *
 * @param type BCPressureType to cast to
 * @param varname Name of variable to declare and assign
 * @param bc_pressure_data Struct to load data from
 * @param ipatch Patch index
 * @param interval_number Interval index
 */
#define GetBCPressureTypeStruct(type, varname, bc_pressure_data, ipatch, interval_number) \
  BCPressureType ## type * varname                                                        \
    = (BCPressureType ## type*)BCPressureDataIntervalValue(bc_pressure_data,              \
                                                           (ipatch), (interval_number));  \
  PF_UNUSED(varname)

/*----------------------------------------------------------------
 * BCPressure Data structure
 *----------------------------------------------------------------*/

typedef struct {
  int num_phases;

  int num_patches;

  int                *types;
  int                *patch_indexes;
  int                *cycle_numbers;
  int                *bc_types;
  void             ***values;

  /* time info */
  TimeCycleData      *time_cycle_data;
} BCPressureData;

/*--------------------------------------------------------------------------
 * Accessor macros: BCPressureValues
 * @MCB: With the new macro system these don't make as much sense, replace/deprecate?
 *--------------------------------------------------------------------------*/
#define DirEquilRefPatchValue(patch) \
  ((patch)->value)

#define DirEquilRefPatchRefSolid(patch) \
  ((patch)->reference_solid)
#define DirEquilRefPatchRefPatch(patch) \
  ((patch)->reference_patch)

#define DirEquilRefPatchValueAtInterfaces(patch) \
  ((patch)->value_at_interfaces)
#define DirEquilRefPatchValueAtInterface(patch, i) \
  ((patch)->value_at_interfaces[i - 1])
/*--------------------------------------------------------------------------*/
#define DirEquilPLinearXLower(patch) \
  ((patch)->xlower)

#define DirEquilPLinearYLower(patch) \
  ((patch)->ylower)

#define DirEquilPLinearXUpper(patch) \
  ((patch)->xupper)

#define DirEquilPLinearYUpper(patch) \
  ((patch)->yupper)

#define DirEquilPLinearNumPoints(patch) \
  ((patch)->num_points)

#define DirEquilPLinearPoints(patch) \
  ((patch)->points)
#define DirEquilPLinearPoint(patch, i) \
  ((patch)->points[i])

#define DirEquilPLinearValues(patch) \
  ((patch)->values)
#define DirEquilPLinearValue(patch, i) \
  ((patch)->values[i])

#define DirEquilPLinearValueAtInterfaces(patch) \
  ((patch)->value_at_interfaces)
#define DirEquilPLinearValueAtInterface(patch, i) \
  ((patch)->value_at_interfaces[i - 1])
/*--------------------------------------------------------------------------*/
#define FluxConstValue(patch) \
  ((patch)->value)
/*--------------------------------------------------------------------------*/
#define FluxVolumetricValue(patch) \
  ((patch)->value)

/*--------------------------------------------------------------------------*/
#define PressureFileName(patch) \
  ((patch)->filename)

/*--------------------------------------------------------------------------*/
#define FluxFileName(patch) \
  ((patch)->filename)

/*--------------------------------------------------------------------------*/
#define ExactSolutionFunctionType(patch) \
  ((patch)->function_type)

/*--------------------------------------------------------------------------*/
#define OverlandFlowValue(patch) \
  ((patch)->value)
/*--------------------------------------------------------------------------*/
#define OverlandFlowPFBFileName(patch) \
  ((patch)->filename)
/*--------------------------------------------------------------------------*/
#define SeepageFaceValue(patch) \
  ((patch)->value)

/*--------------------------------------------------------------------------*/
  #define OverlandKinematicValue(patch) \
  ((patch)->value)
/*--------------------------------------------------------------------------*/
    #define OverlandDiffusiveValue(patch) \
  ((patch)->value)
/*--------------------------------------------------------------------------
 * Accessor macros: BCPressureData
 *--------------------------------------------------------------------------*/
#define BCPressureDataNumPhases(bc_pressure_data) \
  ((bc_pressure_data)->num_phases)

#define BCPressureDataNumPatches(bc_pressure_data) \
  ((bc_pressure_data)->num_patches)

#define BCPressureDataTypes(bc_pressure_data) \
  ((bc_pressure_data)->types)
#define BCPressureDataType(bc_pressure_data, i) \
  ((bc_pressure_data)->types[i])

#define BCPressureDataCycleNumbers(bc_pressure_data) \
  ((bc_pressure_data)->cycle_numbers)
#define BCPressureDataCycleNumber(bc_pressure_data, i) \
  ((bc_pressure_data)->cycle_numbers[i])

#define BCPressureDataPatchIndexes(bc_pressure_data) \
  ((bc_pressure_data)->patch_indexes)
#define BCPressureDataPatchIndex(bc_pressure_data, i) \
  ((bc_pressure_data)->patch_indexes[i])

#define BCPressureDataBCTypes(bc_pressure_data) \
  ((bc_pressure_data)->bc_types)
#define BCPressureDataBCType(bc_pressure_data, i) \
  ((bc_pressure_data)->bc_types[i])

#define BCPressureDataValues(bc_pressure_data) \
  ((bc_pressure_data)->values)
#define BCPressureDataIntervalValues(bc_pressure_data, i) \
  ((bc_pressure_data)->values[i])
#define BCPressureDataIntervalValue(bc_pressure_data, i, interval_number) \
  (((bc_pressure_data)->values[i])[interval_number])

#define BCPressureDataTimeCycleData(bc_pressure_data) \
  ((bc_pressure_data)->time_cycle_data)

/*--------------------------------------------------------------------------
 * BCPressure Data constants used in the program.
 *--------------------------------------------------------------------------*/


/*----------------------------------------------------------------
 * BCPressurePackage Actions
 *----------------------------------------------------------------*/
/**
 * @name InputType Accessor
 * @brief Gets the input type at the specified index from BCPressurePackage public_xtra
 *
 * @param public_xtra public_xtra for module
 * @param i Index for input type
 */
#define InputType(public_xtra, i) \
  ((public_xtra)->input_types[(i)])

/* Send cases wrapped in {} for sanity */
/**
 * @name BC Patch control flow wrappers
 * @brief Wraps switch-case control flow inside self-describing macro names
 * @{
 */
#define Do_SetupPatchTypes(public_xtra, interval, i, cases) \
  switch (InputType(public_xtra, i))                        \
  {                                                         \
    cases;                                                  \
  }

#define SetupPatchType(type, body) \
  case type:                       \
  {                                \
    body;                          \
    break;                         \
  }
#define Do_SetupPatchIntervals(public_xtra, interval, i, cases) \
  switch (InputType(public_xtra, i))                            \
  {                                                             \
    cases;                                                      \
  }

#define SetupPatchInterval(type, body) \
  case type:                           \
  {                                    \
    body;                              \
    break;                             \
  }
#define Do_FreePatches(public_xtra, i, ...) \
  switch (InputType(public_xtra, i))        \
  {                                         \
    __VA_ARGS__;                            \
  }

#define FreePatch(type, body) \
  case type:                  \
  {                           \
    body;                     \
    break;                    \
  }
/** @} */

#endif
