#ifndef INCLUDED_VERSAT_EXTRA
#define INCLUDED_VERSAT_EXTRA

#include "versat.hpp"

// For simple instantiations, where we only want to test one unit with non complex inputs and outputs
// No mems or vreads/vwrites, just a unit full of combinatorial logic
struct SimpleAccelerator{
   Accelerator* accel;
   FUDeclaration* decl;
   FUInstance* inst;
   FUInstance* inputs[99]; // Inputs are Const
   unsigned int numberInputs;
   FUInstance* outputs[99]; // Outputs are Reg
   unsigned int numberOutputs;
   bool init;
};

// Does nothing if accel already initialized. Returns true if initialization occured
bool InitSimpleAccelerator(SimpleAccelerator* simple,Versat* versat,const char* declarationName);
void RemapSimpleAccelerator(SimpleAccelerator* simple,Versat* versat);
int* RunSimpleAccelerator(SimpleAccelerator* simple, ...); // Return is statically allocated. Do not reuse between calls to function
int* RunSimpleAcceleratorDebug(SimpleAccelerator* simple, ...);

void OutputVersatSource(Versat* versat,SimpleAccelerator* accel,const char* directoryPath);

#endif // INCLUDED_VERSAT_EXTRA