#ifndef INCLUDED_VERSAT_HPP
#define INCLUDED_VERSAT_HPP

#include <stdio.h>
#include <stdint.h>
#include <unordered_map>
#include <initializer_list>

#include "utils.hpp"

// Forward declarations
struct Versat;
struct Accelerator;
struct FUInstance;

typedef int32_t* (*FUFunction)(FUInstance*);
typedef int32_t (*MemoryAccessFunction)(FUInstance* instance, int address, int value,int write);

enum DelayType {
   DELAY_TYPE_BASE               = 0x0,
   DELAY_TYPE_SINK_DELAY         = 0x1,
   DELAY_TYPE_SOURCE_DELAY       = 0x2,
   DELAY_TYPE_COMPUTE_DELAY      = 0x4,
   DELAY_TYPE_IMPLEMENTS_DONE    = 0x8
};

#define CHECK_DELAY(inst,T) ((inst->declaration->delayType & T) == T)

struct Wire{
   const char* name;
   int bitsize;
};

struct FUDeclaration{
   HierarchyName name;

   int nInputs;
   int nOutputs;

   int* inputDelays;
   int* latencies;

   Accelerator* circuit; // Composite declaration

   // Config and state interface
   int nConfigs;
   Wire* configWires;

   int nStates;
   Wire* stateWires;

   int nDelays; // Code only handles 1 for now, hardware needs this value for correct generation

   int memoryMapDWords;
   int extraDataSize;
   bool doesIO;
   FUFunction initializeFunction;
   FUFunction startFunction;
   FUFunction updateFunction;
   MemoryAccessFunction memAccessFunction;

   enum {SINGLE = 0x0,COMPOSITE = 0x1,SPECIAL = 0x2} type;

   enum DelayType delayType;
};

struct PortInstance{
   FUInstance* inst;
   int port;
};

struct ConnectionInfo{
   PortInstance inst;
   int port;
   int delay;
};

struct GraphComputedData{
   int numberInputs;
   int numberOutputs;
   int inputPortsUsed;
   int outputPortsUsed;
   ConnectionInfo* inputs; // Delay not used
   ConnectionInfo* outputs;
   enum {TAG_UNCONNECTED,TAG_COMPUTE,TAG_SOURCE,TAG_SINK,TAG_SOURCE_AND_SINK} nodeType;
   int inputDelay;
};

struct VersatComputedData{
   char memoryMask[32];
   int memoryMaskSize;
   int addressTopBit;
};

struct FUInstance{
	HierarchyName name;

   // Embedded memory
   int* memMapped;
   int* config;
   int* state;
   int* delay;

   int baseDelay;

   // PC only
   Accelerator* accel;
	FUDeclaration* declaration;
	int id;
   Accelerator* compositeAccel;

	int32_t* outputs;
	int32_t* storedOutputs;
   void* extraData;

   // Configuration + State variables that versat needs access to
   int done; // Units that are sink or sources of data must implement done to ensure circuit does not end prematurely

   // Various uses
   GraphComputedData* tempData;
   VersatComputedData* versatData;
   char tag;
};

struct Versat{
	Pool<FUDeclaration> declarations;
	Pool<Accelerator> accelerators;

	int base;
	int numberConfigurations;

	// Declaration for units that versat needs to instantiate itself
	FUDeclaration* delay;
   FUDeclaration* input;
   FUDeclaration* output;
   FUDeclaration* pipelineRegister;
};

struct DAGOrder{
   FUInstance** sinks;
   int numberSinks;
   FUInstance** sources;
   int numberSources;
   FUInstance** computeUnits;
   int numberComputeUnits;
   FUInstance** instancePtrs; // Source, then compute, then sink (source_and_sink treated as sink)
};

struct Edge{ // A edge in a graph
   PortInstance units[2];
   int delay;
};

struct Accelerator{
   Versat* versat;

   Pool<FUInstance> instances;
	Pool<Edge> edges;

   Pool<FUInstance*> inputInstancePointers;
   FUInstance* outputInstance;

   int32_t* config;
   int32_t* state;
   int32_t* memMapped;
   int32_t* delay;

   // TODO: Maybe add config, state and memMapped to these structs
   AllocInfo configAlloc;
   AllocInfo stateAlloc;
   AllocInfo memMappedAlloc;
   AllocInfo delayAlloc;

	void* configuration;
	int configurationSize;

	bool locked; // Used for debug purposes
	void* graphDataMem;
	void* versatDataMem;
   DAGOrder order;
   int cyclesPerRun;

	int entityId;
	bool init;

	enum {ACCELERATOR,SUBACCELERATOR,CIRCUIT} type;
};

struct UnitInfo{
   int nConfigsWithDelay;
   int configBitSize;
   int nStates;
   int stateBitSize;
   int memoryMapDWords;
   int implementsDelay;
   int numberDelays;
   int implementsDone;
   int doesIO;
};

struct MappingNode{ // Mapping (edge to edge or node to node)
   Edge edges[2];
};

struct MappingEdge{ // Edge between mapping from edge to edge
   MappingNode nodes[2];
};

struct ConsolidationGraph{
   MappingNode* nodes;
   int numberNodes;
   MappingEdge* edges;
   int numberEdges;

   // Used in get clique;
   int* validNodes;
};

// Private but needed to be on header for type introspection
struct Mapping{
   FUInstance* source;
   FUInstance* sink;
};

struct Range{
   int high;
   int low;
};

struct VersatComputedValues{
   int memoryMapped;
   int numberConnections;
   int unitsMapped;
   int nConfigs;
   int nStates;
   int nDelays;
   int nUnitsIO;
   int configurationBits;
   int stateBits;
   int lowerAddressSize;
   int configurationAddressBits;
   int stateAddressBits;
   int maxMemoryMapDWords;
   int memoryAddressBits;
   int stateConfigurationAddressBits;
   int memoryMappingAddressBits;

   // Auxiliary for versat generation;
   int memoryConfigDecisionBit;

   // Address
   Range memoryAddressRange;
   Range configAddressRange;
   Range stateAddressRange;

   // Bits
   Range configBitsRange;
   Range stateBitsRange;
};

struct SubgraphData{
   Accelerator* accel;
   FUInstance* instanceMapped;
};

Accelerator* Flatten(Versat* versat,Accelerator* accel,int times);

void OutputGraphDotFile(Accelerator* accel,bool collapseSameEdges,const char* filenameFormat,...) __attribute__ ((format (printf, 3, 4)));

// Versat functions
void InitVersat(Versat* versat,int base,int numberConfigurations);
FUDeclaration* RegisterFU(Versat* versat,FUDeclaration declaration);
void OutputVersatSource(Versat* versat,Accelerator* accel,const char* sourceFilepath,const char* constantsFilepath,const char* dataFilepath);
void OutputCircuitSource(Versat* versat,FUDeclaration accelDecl,Accelerator* accel,FILE* file);
void OutputMemoryMap(Versat* versat,Accelerator* accel);

FUDeclaration* GetTypeByName(Versat* versat,SizedString str);

// Accelerator functions
Accelerator* CreateAccelerator(Versat* versat);
FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type);
FUInstance* CreateNamedFUInstance(Accelerator* accel,FUDeclaration* type,SizedString entityName,HierarchyName* hierarchyParent);
void RemoveFUInstance(Accelerator* accel,FUInstance* inst);

void LockAccelerator(Accelerator* accel);

// Can use printf style arguments, but only chars and integers.
// Put arguments right after format string
#define GetInstanceByName(ACCEL,...) GetInstanceByName_(ACCEL,NUMBER_ARGS(__VA_ARGS__),__VA_ARGS__)
FUInstance* GetInstanceByName_(Accelerator* accel,int argc, ...);

void SaveConfiguration(Accelerator* accel,int configuration);
void LoadConfiguration(Accelerator* accel,int configuration);

void AcceleratorRun(Accelerator* accel);

void AddInput(Accelerator* accel);

// Helper functions
int32_t GetInputValue(FUInstance* instance,int port);

Edge* ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex);

void VersatUnitWrite(FUInstance* instance,int address, int value);
int32_t VersatUnitRead(FUInstance* instance,int address);

int CalculateLatency(FUInstance* inst);
void CalculateDelay(Versat* versat,Accelerator* accel);
void SetDelayRecursive(FUInstance* inst,int delay);
void CalculateGraphData(Accelerator* accel);
void CalculateVersatData(Accelerator* accel,VersatComputedValues vals);

ConsolidationGraph GenerateConsolidationGraph(Accelerator* accel1,Accelerator* accel2);
ConsolidationGraph MaxClique(ConsolidationGraph graph);
Accelerator* MergeGraphs(Versat* versat,Accelerator* accel1,Accelerator* accel2,ConsolidationGraph graph);

SubgraphData SubGraphAroundInstance(Versat* versat,Accelerator* accel,FUInstance* instance,int layers);

FUInstance* GetInstance(Accelerator* circuit,std::initializer_list<char*> names);

#endif // INCLUDED_VERSAT_HPP
