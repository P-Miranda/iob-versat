// This file has been auto-generated

#ifndef INCLUDED_VERSAT_ACCELERATOR_HEADER
#define INCLUDED_VERSAT_ACCELERATOR_HEADER

#ifdef __cplusplus
#include <cstdint>
#else
#include "stdbool.h"
#include "stdint.h"
#endif

typedef intptr_t iptr;

// Config
@{configStructs}

// State
@{stateStructs}

@{acceleratorState}

// Address
@{addrStructs}

@{acceleratorConfig}

@{acceleratorStatic}

@{acceleratorDelay}

// NOTE: The address for memory mapped units depends on the address of
//       the accelerator.  Because of this, the full address can only
//       be calculated after calling versat_init(iptr), which is the
//       function that sets the versat_base variable.  It is for this
//       reason that the address info for every unit is a define. Addr
//       variables must be instantiated only after calling
//       versat_init.

// Base address for each memory mapped unit

@{memMappedAddresses}

extern iptr versat_base;

#ifdef __cplusplus
extern "C" {
#endif

// Always call first before calling any other function.
void versat_init(int base);

// Versat runtime does not provide any output unless provided with a printf like function by the user.
typedef int (*VersatPrintf)(const char* format,...);
void SetVersatDebugPrintfFunction(VersatPrintf function);

// In pc-emul provides a low bound on performance.
// In sim-run refines the lower bound but still likely to be smaller than reality due to memory delays that are only present in real circuits.
int GetAcceleratorCyclesElapsed();

void RunAccelerator(int times);
void StartAccelerator();
void EndAccelerator();
void ResetAccelerator();
void VersatMemoryCopy(volatile void* dest,volatile const void* data,int byteSize);
void VersatUnitWrite(volatile const void* baseaddr,int index,int val);
int VersatUnitRead(volatile const void* baseaddr,int index);
float VersatUnitReadFloat(volatile const void* baseaddr,int index);
void SignalLoop();
void VersatLoadDelay(volatile const unsigned int* delayBuffer);

// PC-Emul side functions that allow to enable or disable certain portions of the emulation
// Their embedded counterparts simply do nothing
void ConfigEnableDMA(bool value);
void ConfigCreateVCD(bool value);
void ConfigSimulateDatabus(bool value); 

@{AddressStruct}

// PC-Emul side function only that allow us to simulate what addresses a V unit would access, instead of having to run the accelerator and having to inspect the VCD file, we can simulate it at pc-emul.
typedef struct{
  int amountOfExternalValuesRead;
  int amountOfInternalValuesUsed; // Repeated values are only counted once. The VRead is simulated in order to calculate this.
} SimulateVReadResult;

int SimulateAddressGen(iptr* arrayToFill,int arraySize,AddressVArguments args);
SimulateVReadResult SimulateVRead(AddressVArguments args);
void SimulateAndPrintAddressGen(AddressVArguments args);

#ifdef __cplusplus
} // extern "C"
#endif

// These value must match the same parameter used to instantiate the Versat hardware unit.
// Bad things will happen if a mismatch occurs
#ifndef VERSAT_AXI_DATA_W
  #define VERSAT_AXI_DATA_W @{databusDataSize}
#endif
#ifndef VERSAT_DATA_W
  #define VERSAT_DATA_W 32 // TODO: Techically Versat could support non 32 datapaths, but no attempt or test has been performed. It will most likely fail, especially the units which almost always expect 32 bits.
#endif

#define VERSAT_DIFF_W (VERSAT_AXI_DATA_W / VERSAT_DATA_W)

// Needed by PC-EMUL to correctly simulate the design, embedded compiler should remove these symbols from firmware because not used by them 
static const char* acceleratorTypeName = "@{typeName}";
static bool isSimpleAccelerator = @{isSimple};
static bool acceleratorSupportsDMA = @{useDMA};

static const int memMappedStart = @{memMappedStart};
static const int versatAddressSpace = @{versatAddressSpace};

#define ACCELERATOR_TOP_ADDR_INIT @{addrBlock}
static unsigned int delayBuffer[] = @{delayBlock};
static AcceleratorDelay accelDelay = @{delayBlock};

static const int staticStart = @{staticStart} * sizeof(iptr);
static const int delayStart = @{delayStart} * sizeof(iptr);
static const int configStart = @{configStart} * sizeof(iptr);
static const int stateStart = @{stateStart} * sizeof(int);

static const unsigned int AcceleratorConfigSize = sizeof(@{typeName}Config);

extern volatile @{typeName}Config* accelConfig; // @{nConfigs}
extern volatile @{typeName}State* accelState; // @{nStates}

static inline iptr ALIGN(iptr base,iptr alignment){
  // TODO: Because alignment is power of 2 (unless we want to support weird AXI_DATA_W values), we can use a faster implementation here.
  iptr diff = base % alignment;
  if(diff == 0){
    return base;
  }

  iptr result = (base - diff + alignment);
  return result;
}

extern volatile AcceleratorStatic* accelStatic;

@{allStaticDefines}

@{simpleStuff}

@{mergeStuff}

static bool forceDoubleLoop = false;
static bool forceSingleLoop = false;

@{allAddrGen}

#endif // INCLUDED_VERSAT_ACCELERATOR_HEADER

