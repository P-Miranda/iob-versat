#include "templateEngine.hpp"

#include "memory.hpp"
#include "parser.hpp"

struct Frame{
  Hashmap<String,Value>* table;
  Frame* previousFrame;
};

static Opt<Value> GetValue(Frame* frame,String var){
  Frame* ptr = frame;

  while(ptr){
    Value* possible = ptr->table->Get(var);
    if(possible){
      return *possible;
    } else {
      ptr = ptr->previousFrame;
    }
  }
  return {};
}

static Value* ValueExists(Frame* frame,String id){
  Frame* ptr = frame;

  while(ptr){
    Value* possible = ptr->table->Get(id);
    if(possible){
      return possible;
    }
    ptr = ptr->previousFrame;
  }
  return nullptr;
}

static void CreateValue(Frame* frame,String id,Value val){
  frame->table->Insert(id,val);
}

static void SetValue(Frame* frame,String id,Value val){
  Value* possible = ValueExists(frame,id);
  if(possible){
    *possible = val;
  } else {
    frame->table->Insert(id,val);
  }
}

static Frame* CreateFrame(Frame* previous,Arena* out){
  Frame* frame = PushStruct<Frame>(out);
  frame->table = PushHashmap<String,Value>(out,16); // Testing a fixed hashmap for now.
  frame->previousFrame = previous;
  return frame;
}

static Frame* globalFrame;
void InitializeTemplateEngine(Arena* perm){
  globalFrame = CreateFrame(nullptr,perm);
  globalFrame->table = PushHashmap<String,Value>(perm,99);
  globalFrame->previousFrame = nullptr;
}

void ProcessTemplateSimple(FILE* outputFile,String tmpl){
  TEMP_REGION(temp,nullptr);
  
  auto b = StartString(temp);

  int size = tmpl.size;
  for(int i = 0; i < size; i++){
    if(i + 2 < size && tmpl[i] == '@' && tmpl[i+1] == '{'){
      i = i+2;

      int start = i;
      for(; i < size; i++){
        if(tmpl[i] == '}'){
          break;
        }
      }
      String subName = String{&tmpl[start],i - start};

      Opt<Value> optVal = GetValue(globalFrame,subName);

      if(!optVal.has_value()){
        printf("[Error] Template did not find the member: '%.*s'\n",UN(subName));
        Assert(false);
      }

      Value val = optVal.value();

      if(val.type == ValueType_STRING){
        b->PushString(val.str);
      } else if(val.type == ValueType_NUMBER) {
        b->PushString("%ld",val.number);
      } else if(val.type == ValueType_BOOLEAN) {
        if(val.boolean){
          b->PushString("true");
        } else {
          b->PushString("false");
        }
      } else {
        Assert(false);
      }
    } else {
      b->PushChar(tmpl[i]);
    }
  }

  String content = EndString(temp,b);
  
  fprintf(outputFile,"%.*s",UNPACK_SS(content));
  fflush(outputFile);

  ClearTemplateEngine();
}

void TemplateSimpleSubstitute(StringBuilder* b,String tmpl,Hashmap<String,String>* subs){
  int size = tmpl.size;
  for(int i = 0; i < size; i++){
    if(i + 2 < size && tmpl[i] == '@' && tmpl[i+1] == '{'){
      i = i+2;

      int start = i;
      for(; i < size; i++){
        if(tmpl[i] == '}'){
          break;
        }
      }
      String subName = String{&tmpl[start],i - start};

      String toSubstituteWith = subs->GetOrFail(subName);
      b->PushString(toSubstituteWith);
    } else {
      b->PushChar(tmpl[i]);
    }
  }
}

String TemplateSubstitute(String tmpl,String* valuesToReplace,Arena* out){
  TEMP_REGION(temp,out);
  auto b = StartString(temp);
  
  int size = tmpl.size;
  for(int i = 0; i < size; i++){
    if(i + 2 < size && tmpl[i] == '@' && tmpl[i+1] == '{'){
      i = i+2;

      int start = i;
      for(; i < size; i++){
        if(tmpl[i] == '}'){
          break;
        }
      }
      String subIndex = String{&tmpl[start],i - start};
      int index = ParseInt(subIndex);
      
      b->PushString(valuesToReplace[index]);
    } else {
      b->PushChar(tmpl[i]);
    }
  }

  return EndString(out,b);
}

void ClearTemplateEngine(){
  globalFrame->table->Clear();
}

Value MakeValue(){
  Value val = {};
  val.type = ValueType_NIL;
  return val;
};

Value MakeValue(int number){
  Value val = {};
  val.type = ValueType_NUMBER;
  val.number = number;
  return val;
};

Value MakeValue(i64 number){
  Value val = {};
  val.type = ValueType_NUMBER;
  val.number = number;
  return val;
}

Value MakeValue(String str){
  Value val = {};
  val.type = ValueType_STRING;
  val.str = str;
  return val;
}

Value MakeValue(bool b){
  Value val = {};
  val.type = ValueType_BOOLEAN;
  val.boolean = b;
  return val;
}

void TemplateSetCustom(const char* id,Value val){
  SetValue(globalFrame,STRING(id),val);
}

void TemplateSetNumber(const char* id,int number){
  SetValue(globalFrame,STRING(id),MakeValue(number));
}

void TemplateSetString(const char* id,const char* str){
  Value val = {};
  val.type = ValueType_STRING;
  val.str = STRING(str);

  SetValue(globalFrame,STRING(id),val);
}

void TemplateSetString(const char* id,String str){
  Value val = {};
  val.type = ValueType_STRING;
  val.str = str;

  SetValue(globalFrame,STRING(id),val);
}

void TemplateSetHex(const char* id,int number){
  // TODO: Need to indicate that this is a hexadecimal number
  Value val = {};
  val.type = ValueType_NUMBER;
  val.number = number;
  
  SetValue(globalFrame,STRING(id),val);
}

void TemplateSetBool(const char* id,bool boolean){
  Value val = {};
  val.type = ValueType_BOOLEAN;
  val.boolean = boolean;
  
  SetValue(globalFrame,STRING(id),val);
}

// This shouldn't be here, but cannot be on parser.cpp because otherwise struct parser would fail
Array<Value> ExtractValues(const char* format,String tok,Arena* out){
  // TODO: This is pretty much a copy of CheckFormat but with small modifications
  // There should be a way to refactor into a single function, and probably less error prone
  if(!CheckFormat(format,tok)){
    return {};
  }

  auto arr = StartArray<Value>(out);

  int tokenIndex = 0;
  for(int formatIndex = 0; 1;){
    char formatChar = format[formatIndex];

    if(formatChar == '\0'){
      break;
    }

    Assert(tokenIndex < tok.size);

    if(formatChar == '%'){
      char type = format[formatIndex + 1];
      formatIndex += 2;

      switch(type){
      case 'd':{
        String numberStr = {};
        numberStr.data = &tok[tokenIndex];

        for(tokenIndex += 1; tokenIndex < tok.size; tokenIndex++){
          if(!IsNum(tok[tokenIndex])){
            break;
          }
        }

        numberStr.size = &tok.data[tokenIndex] - numberStr.data;
        int number = ParseInt(numberStr);

        Value* val = arr.PushElem();
        *val = {};
        val->type = ValueType_NUMBER;
        val->number = number;
      }break;
      case 's':{
        char terminator = format[formatIndex];
        String str = {};
        str.data = &tok[tokenIndex];

        for(;tokenIndex < tok.size; tokenIndex++){
          if(tok[tokenIndex] == terminator){
            break;
          }
        }

        str.size = &tok.data[tokenIndex] - str.data;

        Value* val = arr.PushElem();
        *val = {};
        val->type = ValueType_STRING;
        val->str = str;
      }break;
      case '\0':{
        NOT_POSSIBLE("Format char not finished"); // TODO: Probably should be a error that reports 
      }break;
      default:{
        NOT_IMPLEMENTED("Implement as needed");
      }break;
      }
    } else {
      Assert(formatChar == tok[tokenIndex]);
      formatIndex += 1;
      tokenIndex += 1;
    }
  }

  Array<Value> values = EndArray(arr);

  return values;
}
