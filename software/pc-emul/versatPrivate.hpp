#ifndef INCLUDED_VERSAT_PRIVATE_HPP
#define INCLUDED_VERSAT_PRIVATE_HPP

#include <vector>
#include <unordered_map>

#include "memory.hpp"
#include "logger.hpp"

#include "versat.hpp"

struct ComplexFUInstance;

typedef int* (*FUFunction)(ComplexFUInstance*);
typedef int (*MemoryAccessFunction)(ComplexFUInstance* instance, int address, int value,int write);

enum DelayType {
   DELAY_TYPE_BASE               = 0x0,
   DELAY_TYPE_SINK_DELAY         = 0x1,
   DELAY_TYPE_SOURCE_DELAY       = 0x2,
   DELAY_TYPE_COMPUTE_DELAY      = 0x4
};

inline DelayType operator|(DelayType a, DelayType b)
{
    return static_cast<DelayType>(static_cast<int>(a) | static_cast<int>(b));
}

template<typename Node,typename Edge>
struct Graph{
   Node* nodes;
   int numberNodes;
   Edge* edges;
   int numberEdges;

   BitArray validNodes;
};

#define CHECK_DELAY(inst,T) ((inst->declaration->delayType & T) == T)

struct Wire{
   SizedString name;
   int bitsize;
};

struct StaticInfo{
   FUDeclaration* module;
   SizedString name;
   Wire* wires;
   int nConfigs;
   int* ptr; // Pointer to config of existing unit
};

bool operator<(const StaticInfo& lhs,const StaticInfo& rhs);

struct SimpleNode{
   SizedString name;
   FUDeclaration* declaration;

   bool isStatic;
   //void* data;
};

struct PortInstance{
   ComplexFUInstance* inst;
   int port;
};

struct ConnectionInfo{
   PortInstance instConnectedTo;
   int port;
   int delay;
};

struct Edge{ // A edge in a graph
   PortInstance units[2];
   int delay;
};

// A declaration is constant after being registered
struct FUDeclaration{
   SizedString name;

   int nInputs;
   int* inputDelays;
   int nOutputs;
   int* latencies;

   // Interfaces
   int nConfigs;
   Wire* configWires;
   int nStates;
   Wire* stateWires;
   int nDelays; // Code only handles 1 single instace, for now, hardware needs this value for correct generation
   int nIOs;
   int memoryMapBits;
   int nStaticConfigs;
   int extraDataSize;

   // Stores different accelerators depending on properties we want
   Accelerator* baseCircuit;
   Accelerator* fixedMultiEdgeCircuit;
   Accelerator* fixedDelayCircuit;

   // Merged accelerator
   FUDeclaration* mergedType[2]; // TODO: probably change it from static to a dynamic allocating with more space, in order to accommodate multiple mergings (merge A with B and then the result with C)

   // Iterative
   SizedString unitName;
   FUDeclaration* baseDeclaration;
   Accelerator* initial;
   Accelerator* forLoop;
   int dataSize;

   FUFunction initializeFunction;
   FUFunction startFunction;
   FUFunction updateFunction;
   FUFunction destroyFunction;
   MemoryAccessFunction memAccessFunction;

   const char* operation;
   Pool<StaticInfo> staticUnits;

   enum {SINGLE = 0x0,COMPOSITE = 0x1,SPECIAL = 0x2,MERGED = 0x3,ITERATIVE = 0x4} type;
   DelayType delayType;

   bool isOperation;
   bool implementsDone;
   bool isMemoryMapped;
};

struct GraphComputedData{
   int numberInputs;
   int numberOutputs;
   int inputPortsUsed;
   int outputPortsUsed;
   ConnectionInfo* inputs;
   ConnectionInfo* outputs;
   enum {TAG_UNCONNECTED,TAG_COMPUTE,TAG_SOURCE,TAG_SINK,TAG_SOURCE_AND_SINK} nodeType;
   int inputDelay;
   int order;
};

struct VersatComputedData{
   int memoryMaskSize;
   int memoryAddressOffset;
   char memoryMask[33];
};

struct Parameter{
   SizedString name;
   SizedString value;
   Parameter* next;
};

struct ComplexFUInstance : public FUInstance{
   // Various uses
   FUInstance* declarationInstance;

   Accelerator* iterative;

   GraphComputedData* graphData;
   VersatComputedData* versatData;
   char tag;
   bool savedConfiguration; // For subunits registered, indicate when we save configuration before hand
   bool savedMemory; // Same for memory
   int sharedIndex;
   bool sharedEnable;
   bool initialized;
};

struct DebugState{
   bool outputGraphs;
   bool outputAccelerator;
   bool outputVersat;
   bool outputVCD;
   bool useFixedBuffers;
};

struct Versat{
	Pool<FUDeclaration> declarations;
	Pool<Accelerator> accelerators;

	Arena permanent;
	Arena temp;

	int base;
	int numberConfigurations;

	// Declaration for units that versat needs to instantiate itself
	FUDeclaration* buffer;
   FUDeclaration* fixedBuffer;
   FUDeclaration* input;
   FUDeclaration* output;
   FUDeclaration* multiplexer;
   FUDeclaration* combMultiplexer;
   FUDeclaration* pipelineRegister;
   FUDeclaration* data;

   DebugState debug;

   // Command line options
   std::vector<const char*> includeDirs;
};

struct DAGOrder{
   ComplexFUInstance** sinks;
   int numberSinks;
   ComplexFUInstance** sources;
   int numberSources;
   ComplexFUInstance** computeUnits;
   int numberComputeUnits;
   ComplexFUInstance** instances;
};

struct Test{
   FUInstance* inst;
   ComplexFUInstance storedValues;
   bool usingStoredValues;
};

struct Accelerator{ // Graph + data storage
   Versat* versat;
   FUDeclaration* subtype; // Set if subaccelerator (accelerator associated to a FUDeclaration). A "free" accelerator has this set to nullptr

   Pool<ComplexFUInstance> instances;
	Pool<Edge> edges;

   Pool<ComplexFUInstance*> inputInstancePointers;
   ComplexFUInstance* outputInstance;

   Allocation<int> configAlloc;
   Allocation<int> stateAlloc;
   Allocation<int> delayAlloc;
   Allocation<int> staticAlloc;
   Allocation<int> outputAlloc;
   Allocation<int> storedOutputAlloc;
   Allocation<Byte> extraDataAlloc;

   Pool<StaticInfo> staticInfo;

	void* configuration;
	int configurationSize;

	int cyclesPerRun;

	int entityId;
	bool init;

	//enum {ACCELERATOR,SUBACCELERATOR,CIRCUIT} type;
};

// Graph associated to an accelerator
class AcceleratorView : public Graph<ComplexFUInstance*,Edge>{
public:
   DAGOrder order;
   bool dagOrder;
   bool graphData;
   bool versatData;
   Accelerator* accel;

   void CalculateGraphData(Arena* arena);
   DAGOrder CalculateDAGOrdering(Arena* arena);
   void CalculateVersatData(Arena* arena);
};

struct SubgraphData{
   Accelerator* accel;
   ComplexFUInstance* instanceMapped;
};

struct HashKey{
   SizedString key;
   int data;
};

class AcceleratorIterator{
public:
   PoolIterator<ComplexFUInstance> stack[16];
   int index;
   bool calledStart;

   ComplexFUInstance* Start(Accelerator* accel); // Must call first
   ComplexFUInstance* Next(); // Returns nullptr in the end

   ComplexFUInstance* CurrentAcceleratorInstance(); // Returns the top accelerator for the last FUInstance returned by Start or Next
};

struct IterativeUnitDeclaration{
   SizedString name;
   SizedString unitName;
   FUDeclaration* baseDeclaration;
   Accelerator* initial;
   Accelerator* forLoop;

   int dataSize;
   int latency;
};

struct UnitValues{
   int inputs;
   int outputs;

   int configs;
   int states;
   int delays;
   int ios;
   int totalOutputs;
   int extraData;
   int statics;

   int memoryMappedBits;
   bool isMemoryMapped;
};

UnitValues CalculateIndividualUnitValues(ComplexFUInstance* inst); // Values for individual unit, not taking into account sub units
UnitValues CalculateAcceleratorUnitValues(Versat* versat,ComplexFUInstance* inst); // Values taking into account sub units
UnitValues CalculateAcceleratorValues(Versat* versat,Accelerator* accel); //

int CalculateLatency(ComplexFUInstance* inst);
void CalculateDelay(Versat* versat,Accelerator* accel);
void SetDelayRecursive(Accelerator* accel);

SizedString CalculateGraphData(AcceleratorView view,Arena* arena);
DAGOrder CalculateDAGOrdering(AcceleratorView view,Arena* arena);
VersatComputedData* CalculateVersatData(AcceleratorView view,Arena* arena);

void ClearFUInstanceTempData(Accelerator* accel);

AcceleratorView CreateAcceleratorView(Accelerator* accel,Arena* arena);

bool IsConfigStatic(Accelerator* topLevel,ComplexFUInstance* inst);

int CalculateTotalOutputs(Accelerator* accel);
int CalculateTotalOutputs(FUInstance* inst);

void ConnectUnits(PortInstance out,PortInstance in);

bool IsGraphValid(Accelerator* accel);

bool IsUnitCombinatorial(FUInstance* inst);

void FixMultipleInputs(Versat* versat,Accelerator* accel);

SubgraphData SubGraphAroundInstance(Versat* versat,Accelerator* accel,ComplexFUInstance* instance,int layers);

inline bool operator==(const PortInstance& p1,const PortInstance& p2){return p1.inst == p2.inst && p1.port == p2.port;};

#endif // INCLUDED_VERSAT_PRIVATE_HPP
