#pragma once

#include "utilsCore.hpp"
#include "memory.hpp"

struct AddressGenDef;
struct FUDeclaration;
struct SymbolicExpression;
struct VerilogPortSpec;

// ============================================================================
// Options (generally read only after program start)

enum VersatOperationMode{
  VersatOperationMode_GENERATE_ACCELERATOR,
  VersatOperationMode_GENERATE_TESTBENCH
};

struct Options{
  Array<String> verilogFiles;
  Array<String> extraSources;
  Array<String> includePaths;
  Array<String> unitFolderPaths;
  
  String hardwareOutputFilepath;
  String softwareOutputFilepath;
  String debugPath;

  String prefixIObPort;
  
  String generetaSourceListName; // TODO: Not yet implemented
  
  String specificationFilepath;
  String topName;
  int databusDataSize; // AXI_DATA_W

  bool addInputAndOutputsToTop;
  bool debug;
  bool shadowRegister;
  bool architectureHasDatabus;
  bool useFixedBuffers;
  bool generateFSTFormat;
  bool useDMA;
  bool exportInternalMemories;
  
  bool extraIOb;
  bool useSymbolAddress; // If the system removes the LSB bits of the address (alignment info) and if we must generate code to account for that.

  VersatOperationMode opMode;
};

enum GraphDotFormat : int;

struct DebugState{
  GraphDotFormat dotFormat;
  bool outputGraphs;
  bool outputConsolidationGraphs;
  bool outputVCD;
};

Options DefaultOptions(Arena* out);

// ============================================================================
// Global Data

extern Options globalOptions;
extern DebugState globalDebug;
extern Arena* globalPermanent;

extern Pool<FUDeclaration> globalDeclarations;

// ============================================================================
// Global Readonly (after init)

// Most of this data cannot be described in a constant expression unless we make changes to the way stuff works in a fundamental way or we add a meta build step.
// However both approaches are just more trouble than they are worth. Just add stuff in here and augment the InitializeDefaultData as needed.

extern SymbolicExpression* SYM_zero;
extern SymbolicExpression* SYM_one;
extern SymbolicExpression* SYM_eight;
extern SymbolicExpression* SYM_dataW;
extern SymbolicExpression* SYM_addrW;
extern SymbolicExpression* SYM_axiAddrW;
extern SymbolicExpression* SYM_axiDataW;
extern SymbolicExpression* SYM_delayW;
extern SymbolicExpression* SYM_lenW;
extern SymbolicExpression* SYM_axiStrobeW;
extern SymbolicExpression* SYM_dataStrobeW;

// Verilog interfaces. Direction is from controller POV.
// Format means that the wires contain a single '%d' because they support multiple appearances.
// Most of the functions that expect an interface will have an extra function terminated in Indexed to support
// interface formats.

// TODO: Currently this is hard coded but technically nothing stop us from making this more data oriented.
//       The only usecase that I can see is to support different memory interfaces, but do not know how
//       important this is. Memory interfaces are kinda limitted, right?
extern Array<VerilogPortSpec> INT_IOb;
extern Array<VerilogPortSpec> INT_IObFormat;

// NOTE: We cannot have generic interfaces for this because the size of wires can change
//extern Array<VerilogPortSpec> INT_DPFormat;
//extern Array<VerilogPortSpec> INT_TPFormat;

void InitializeDefaultData(Arena* perm);
