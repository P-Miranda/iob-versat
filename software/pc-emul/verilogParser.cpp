#include "verilogParser.hpp"

#include <cstdio>
#include <vector>

#include "memory.hpp"

#include "templateEngine.hpp"

static MacroMap macros;
static Arena* tempArena;

void PerformDefineSubstitution(Arena* output,SizedString name){
   SizedString subs = macros[name];

   Tokenizer inside(subs,"`",{});

   while(!inside.Done()){
      Token peek = inside.PeekFindUntil("`");

      if(peek.size < 0){
         break;
      } else {
         inside.AdvancePeek(peek);
         PushString(output,peek);

         inside.AssertNextToken("`");

         Token name = inside.NextToken();

         PerformDefineSubstitution(output,name);
      }
   }

   Token finish = inside.Finish();
   PushString(output,finish);
}

void PreprocessVerilogFile_(Arena* output, SizedString fileContent,std::vector<const char*>* includeFilepaths);

static void DoIfStatement(Arena* output,Tokenizer* tok,std::vector<const char*>* includeFilepaths){
   Token first = tok->NextToken();
   Token macroName = tok->NextToken();

   bool compareVal = false;
   if(CompareString(first,"`ifdef") || CompareString(first,"`elsif")){
      compareVal = true;
   } else if(CompareString(first,"`ifndef")){
      compareVal = false;
   } else {
      Assert(false);
   }

   bool exists = (macros.find(macroName) != macros.end());
   bool doIf = (compareVal == exists);

   void* mark = tok->Mark();
   while(!tok->Done()){
      Token peek = tok->PeekToken();

      if(CompareString(peek,"`endif")){
         if(doIf){
            Token content = tok->Point(mark);
            PreprocessVerilogFile_(output,content,includeFilepaths);
         }
         tok->AdvancePeek(peek);
         break;
      }

      if(CompareString(peek,"`else")){
         if(doIf){
            Token content = tok->Point(mark);
            PreprocessVerilogFile_(output,content,includeFilepaths);
         } else {
            tok->AdvancePeek(peek);
            mark = tok->Mark();
            doIf = true;
         }
         continue;
      }

      if(CompareString(peek,"`ifdef") || CompareString(peek,"`ifndef") || CompareString(first,"`elsif")){
         DoIfStatement(output,tok,includeFilepaths);
      } else {
         tok->AdvancePeek(peek);
      }
   }
}

void PreprocessVerilogFile_(Arena* output, SizedString fileContent,std::vector<const char*>* includeFilepaths){
   Tokenizer tokenizer = Tokenizer(fileContent, "()`\\\",+-/*",{"`include","`define","`timescale","`ifdef","`else","`elsif","`endif","`ifndef"});
   Tokenizer* tok = &tokenizer;

   while(!tok->Done()){
      Token peek = tok->PeekToken();
      if(CompareToken(peek,"`include")){ // Assuming defines only happen outside module (Not correct but follows common usage, never seen include between parameters or port definitions)
         tok->AdvancePeek(peek);
         tok->AssertNextToken("\"");

         Token fileName = tok->NextFindUntil("\"");
         tok->AssertNextToken("\"");

         // Open include file
         std::string filename(fileName.str,fileName.size);
         FILE* file = nullptr;
         std::string filepath;
         for(const char* str : *includeFilepaths){
            filepath = str + filename;

            file = fopen(filepath.c_str(),"r");

            if(file){
               break;
            }
         }

         if(!file){
            printf("Couldn't find file: %.*s\n",fileName.size,fileName.str);
            printf("Looked on the following folders:\n");

            printf("  %s\n",GetCurrentDirectory());
            for(const char* str : *includeFilepaths){
               printf("  %s\n",str);
            }

            DebugSignal();
            exit(0);
         }

         int fileSize = GetFileSize(file);

         Byte* mem = PushBytes(tempArena,fileSize + 1);
         fread(mem,sizeof(char),fileSize,file);
         mem[fileSize] = '\0';

         PreprocessVerilogFile_(output,MakeSizedString((const char*) mem,fileSize),includeFilepaths);

         fclose(file);
      } else if(CompareToken(peek,"`define")){ // Same for defines
         tok->AdvancePeek(peek);
         Token defineName = tok->NextToken();

         Token emptySpace = tok->PeekWhitespace();
         if(emptySpace.size == 0){ // Function macro
            Token arguments = tok->PeekFindIncluding(")");
            defineName = ExtendToken(defineName,arguments);
            tok->AdvancePeek(arguments);
         }

         Token first = tok->FindFirst({"\n","//","\\"});
         Token body = {};

         if(CompareString(first,"//")){ // If single comment newline or slash, the macro does not contain the comment
            body = tok->PeekFindUntil("//");
         } else {
            Token line = tok->PeekFindUntil("\n");
            body = line;
            while(!tok->Done()){
               if(line.size == -1){
                  line = tok->Finish();
                  body = ExtendToken(body,line);
                  break;
               }

               Tokenizer inside(line,"\\",{}); // Handles slices inside comments

               bool hasSlice = false;
               while(!inside.Done()){
                  Token t = inside.NextToken();
                  if(CompareString(t,"\\")){
                     hasSlice = true;
                     break;
                  }
               }

               if(hasSlice){
                  tok->AdvancePeek(line);
                  body = ExtendToken(body,line);
                  line = tok->PeekFindUntil("\n");
               } else {
                  tok->AdvancePeek(line);
                  body = ExtendToken(body,line);
                  break;
               }
            }
         }

         //printf("`%.*s  %.*s\n",defineName.size,defineName.str,mini(10,body.size),body.str);
         macros[defineName] = body;
      } else if(CompareToken(peek,"`timescale")){
         Token line = tok->PeekFindIncluding("\n");

         tok->AdvancePeek(line);
      } else if(CompareToken(peek,"`")){
         tok->AdvancePeek(peek);
         Token name = tok->NextToken();

         PerformDefineSubstitution(output,name);
      } else if(CompareToken(peek,"`ifdef") || CompareToken(peek,"`ifndef")){
         DoIfStatement(output,tok,includeFilepaths);
      } else if(CompareToken(peek,"`else")){
         Assert(false);
      } else if(CompareToken(peek,"`endif")){
         Assert(false);
      } else if(CompareToken(peek,"`elsif")){
         Assert(false);
      } else {
         tok->AdvancePeek(peek);

         PushString(output,peek);
      }

      PushString(output,tok->PeekWhitespace());
   }
}

SizedString PreprocessVerilogFile(Arena* output, SizedString fileContent,std::vector<const char*>* includeFilepaths,Arena* arena){
   macros.clear();
   tempArena = arena;

   SizedString res = {};
   res.str = (const char*) &output->mem[output->used];
   Byte* mark = MarkArena(output);

   Byte* tempMark = MarkArena(tempArena);
   PreprocessVerilogFile_(output,fileContent,includeFilepaths);
   PopMark(tempArena,tempMark);

   PushString(output,MAKE_SIZED_STRING("\0"));
   res.size = MarkArena(output) - mark;

   #if 0
   for(auto key : macros){
      printf("%.*s\n",key.first.size,key.first.str);
   }
   #endif

   return res;
}

static Expression* ParseAtom(Tokenizer* tok){
   Expression* expr = PushStruct(tempArena,Expression);

   Token peek = tok->PeekToken();

   if(IsNum(peek.str[0])){
      int res = ParseInt(peek);
      tok->AdvancePeek(peek);

      expr->val = MakeValue(res);
      expr->type = Expression::LITERAL;

      return expr;
   } else if(peek.str[0] == '"'){
      tok->AdvancePeek(peek);
      Token str = tok->PeekFindUntil("\"");
      tok->AdvancePeek(str);
      tok->AssertNextToken("\"");

      expr->val = MakeValue(str);
      expr->type = Expression::LITERAL;

      return expr;
   }

   Token name = tok->NextToken();

   expr->id = name;
   expr->type = Expression::IDENTIFIER;

   return expr;
}

static Expression* ParseExpression(Tokenizer* tok);

static Expression* ParseFactor(Tokenizer* tok){
   Token peek = tok->PeekToken();

   if(CompareString(peek,"(")){
      tok->AdvancePeek(peek);
      Expression* expr = ParseExpression(tok);
      tok->AssertNextToken(")");
      return expr;
   } else {
      Expression* expr = ParseAtom(tok);
      return expr;
   }
}

static Value Eval(Expression* expr,ValueMap& map){
   switch(expr->type){
   case Expression::OPERATION:{
      Value val1 = Eval(expr->expressions[0],map);
      Value val2 = Eval(expr->expressions[1],map);

      switch(expr->op[0]){
      case '+':{
         Value res = MakeValue(val1.number + val2.number);
         return res;
      }break;
      case '-':{
         Value res = MakeValue(val1.number - val2.number);
         return res;
      }break;
      case '*':{
         Value res = MakeValue(val1.number * val2.number);
         return res;
      }break;
      case '/':{
         Value res = MakeValue(val1.number / val2.number);
         return res;
      }break;
      default:{
         Assert(false);
      }break;
      }
   }break;

   case Expression::IDENTIFIER:{
      return map[expr->id];
   }break;
   case Expression::LITERAL:{
      return expr->val;
   }break;
   default:{
      Assert(false);
   }break;
   }

   Assert(false);
   return MakeValue();
}

static ValueMap ParseParameters(Tokenizer* tok,ValueMap& map){
   //TODO: Add type and range to parsing
   /*
   Range currentRange;
   ParameterType type;
   */
   ValueMap parameters;

   while(1){
      Token peek = tok->PeekToken();

      if(CompareToken(peek,"parameter")){
         tok->AdvancePeek(peek);
         // Parse optional type info and range
         continue;
      } else if(CompareToken(peek,")")){
         break;
      } else if(CompareToken(peek,";")){ // To parse inside module parameters, technically wrong but harmless
         tok->AdvancePeek(peek);
         break;
      } else if(CompareToken(peek,",")){
         tok->AdvancePeek(peek);
         continue;
      } else { // Must be a parameter assignemnt
         Token paramName = tok->NextToken();

         tok->AssertNextToken("=");

         Expression* expr = ParseAtom(tok);

         Value val = Eval(expr,map);

         map[paramName] = val;
         parameters[paramName] = val;
      }
   }

   return parameters;
}

static Expression* ParseExpression(Tokenizer* tok){
   Expression* res = ParseOperationType(tok,{{"+","-"},{"*","/"}},ParseFactor,tempArena);

   return res;
}

static Range ParseRange(Tokenizer* tok,ValueMap& map){
   Token peek = tok->PeekToken();

   if(!CompareString(peek,"[")){
      Range range = {};

      return range;
   }

   tok->AssertNextToken("[");

   Range res = {};
   res.high = Eval(ParseExpression(tok),map).number;

   tok->AssertNextToken(":");

   res.low = Eval(ParseExpression(tok),map).number;
   tok->AssertNextToken("]");

   return res;
}

static Module ParseModule(Tokenizer* tok){
   Module module = {};
   ValueMap values;

   tok->AssertNextToken("module");

   module.name = tok->NextToken();

   Token peek = tok->PeekToken();
   if(CompareToken(peek,"#(")){
      tok->AdvancePeek(peek);
      module.parameters = ParseParameters(tok,values);
      tok->AssertNextToken(")");
   }

   tok->AssertNextToken("(");

   // Parse ports
   while(!tok->Done()){
      peek = tok->PeekToken();

      PortDeclaration port;

      if(CompareToken(peek,"(*")){
         tok->AdvancePeek(peek);
         while(1){
            Token attributeName = tok->NextToken();

            peek = tok->PeekToken();
            if(CompareString(peek,"=")){
               tok->AdvancePeek(peek);
               Expression* expr = ParseExpression(tok);
               Value value = Eval(expr,values);

               peek = tok->PeekToken();

               port.attributes[attributeName] = value;
            }

            if(CompareString(peek,",")){
               tok->AdvancePeek(peek);
               continue;
            }
            if(CompareString(peek,"*)")){
               tok->AdvancePeek(peek);
               break;
            }
         }
      }

      Token portType = tok->NextToken();
      if(CompareString(portType,"input")){
         port.type = PortDeclaration::INPUT;
      } else if(CompareString(portType,"output")){
         port.type = PortDeclaration::OUTPUT;
      } else if(CompareString(portType,"inout")){
         port.type = PortDeclaration::INOUT;
      } else {
         Assert(false);
      }

      peek = tok->PeekToken();

      // TODO: Add a new function to parser to "ignore" the following list of tokens (loop every time until it doesn't find one from the list), and replace this function here with reg and all the different types it can be
      if(CompareString("reg",peek)){
         tok->AdvancePeek(peek);
      }

      port.range = ParseRange(tok,values);
      port.name = tok->NextToken();

      module.ports.push_back(port);

      peek = tok->PeekToken();
      if(CompareToken(peek,")")){
         tok->AdvancePeek(peek);
         break;
      }

      tok->AssertNextToken(",");
   }

   // Any inside module parameters
   #if 0
   while(1){
      Token peek = tok->PeekToken();

      if(CompareToken(peek,"parameter")){
         ParseParameters(tok,values);
      } else if(CompareToken(peek,"endmodule")){
         tok->AdvancePeek(peek);
         break;
      } else {
         tok->AdvancePeek(peek);
      }
   }
   #endif

   Token skip = tok->PeekFindIncluding("endmodule");
   tok->AdvancePeek(skip);

   return module;
}

std::vector<Module> ParseVerilogFile(SizedString fileContent, std::vector<const char*>* includeFilepaths, Arena* arena){
   tempArena = arena;

   Tokenizer tokenizer = Tokenizer(fileContent,":,()[]{}\"+-/*=",{"#(","+:","-:","(*","*)","module","endmodule"});
   Tokenizer* tok = &tokenizer;

   std::vector<Module> modules;
   Byte* mark = MarkArena(tempArena);
   while(!tok->Done()){
      Token peek = tok->PeekToken();

      if(CompareToken(peek,"module")){
         Module module = ParseModule(tok);

         #if 0
         printf("%.*s\n",module.name.size,module.name.str);

         for(auto ptr : module.parameters){
            printf("%.*s %d\n",ptr.first.size,ptr.first.str,ptr.second);
         }

         for(PortDeclaration& ptr : module.ports){
            for(auto ptr2 : ptr.attributes){
               printf("(* %.*s = %d *)",ptr2.first.size,ptr2.first.str,ptr2.second);
            }

            printf("%d [%d:%d] %.*s\n",(int) ptr.type,ptr.range.high,ptr.range.low,ptr.name.size,ptr.name.str);
         }
         #endif

         #if 0
         int maxInputs = -1;
         int maxOutputs = -1;
         bool hasIO = false;

         for(PortDeclaration& ptr : module.ports){
            Tokenizer inside(ptr.name,"",{"in","out","databus"});

            Token tok = inside.NextToken();

            if(CompareString(tok,"in")){
               Token number = inside.NextToken();
               int val = ParseInt(number);
               maxInputs = maxi(maxInputs,val);
               continue;
            }

            if(CompareString(tok,"out")){
               Token number = inside.NextToken();
               int val = ParseInt(number);
               maxOutputs = maxi(maxOutputs,val);
               continue;
            }

            if(CompareString(tok,"databus")){
               hasIO = true;
               continue;
            }
         }

         TemplateSetNumber("maxInputs",maxInputs);
         TemplateSetNumber("maxOutputs",maxOutputs);
         TemplateSetBool("hasIO",hasIO);
         TemplateSetCustom("module",&module,"Module");
         ProcessTemplate(output,"../../submodules/VERSAT/software/templates/unit_verilog_data.tpl",arena);
         #endif

         modules.push_back(module);
      } else {
         tok->AdvancePeek(peek);
      }
   }
   PopMark(tempArena,mark);

   return modules;
}

#if 0
#define STANDALONE
#endif

#ifdef STANDALONE
int main(int argc,const char* argv[]){
   if(argc < 3){
      printf("Error, need at least 3 arguments: <program> <outputFile> <inputFile1> ...");

      return 0;
   }

   RegisterTypes();

   std::vector<const char*> includePaths;
   std::vector<const char*> filePaths;
   // Parse include paths
   for(int i = 2; i < argc; i++){
      if(CompareString("-I",argv[i])){
         includePaths.push_back(argv[i+1]);
         i += 1;
      } else {
         filePaths.push_back(argv[i]);
      }
   }

   Arena tempArenaInst = {};
   tempArena = &tempArenaInst;

   InitArena(tempArena,Megabyte(64));

   Arena preprocess = {};
   InitArena(&preprocess,Megabyte(64));

   Arena permanent = {};
   InitArena(&permanent,Megabyte(64));

   std::vector<ModuleInfo> allModules;
   for(const char* str : filePaths){
      Byte* mark = MarkArena(tempArena);
      printf("%s\n",str);

      SizedString content = PushFile(tempArena,str);

      if(content.size <= 0){
         printf("Failed to open file: %s\n",str);
         return 0;
      }

      SizedString processed = PreprocessVerilogFile(&preprocess,content,&includePaths,tempArena);
      std::vector<Module> modules = ParseVerilogFile(processed,&includePaths,tempArena);

      for(Module& module : modules){
         ModuleInfo info = {};

         int* inputDelays = PushArray(&permanent,100,int);
         int* outputLatencies = PushArray(&permanent,100,int);;
         int nInputs = 0;
         int nOutputs = 0;
         int nDelays = 0;
         int memoryMappedWords = 0;
         bool doesIO = false;
         bool memoryMap = false;
         for(PortDeclaration decl : module.ports){
            Tokenizer port(decl.name,"",{"in","out","delay","done","rst","clk","run","databus"});

            if(CheckFormat("in%d",decl.name)){
               port.AssertNextToken("in");
               int input = ParseInt(port.NextToken());
               int delay = decl.attributes[MakeSizedString("latency")].number;

               nInputs = maxi(nInputs,input + 1);
               inputDelays[input] = delay;
            } else if(CheckFormat("out%d",decl.name)){
               port.AssertNextToken("out");
               int output = ParseInt(port.NextToken());
               int latency = decl.attributes[MakeSizedString("latency")].number;

               nOutputs = maxi(nOutputs,output + 1);
               outputLatencies[output] = latency;
            } else if(CheckFormat("delay%d",decl.name)){
               port.AssertNextToken("delay");
               int delay = ParseInt(port.NextToken());

               nDelays = maxi(nDelays,delay + 1);
            } else if(CheckFormat("databus_valid",decl.name)){
               doesIO = true;
            } else if(CheckFormat("addr",decl.name)){
               memoryMap = true;

               int range = (decl.range.high - decl.range.low);

               memoryMappedWords = range;
            }
         }

         info.name = module.name;
         info.doesIO = doesIO;
         info.memoryMapped = memoryMap;
         info.nInputs = nInputs;
         info.nOutputs = nOutputs;
         info.inputDelays = inputDelays;
         info.latencies = outputLatencies;

         allModules.push_back(info);
      }
   }

   TemplateSetArray("modules","ModuleInfo",allModules.data(),allModules.size());

   FILE* output = fopen(argv[1],"w");
   ProcessTemplate(output,"../../submodules/VERSAT/software/templates/unit_verilog_data.tpl",tempArena);

   return 0;
}
#endif










