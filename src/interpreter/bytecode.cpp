#include "bytecode.h"

#include "vm.h"
#include "errormsg.h"
#include "flattener.h"

#include "object.h"
#include "main.h"
#include "program.h"
#include "scope.h"
#include "diagnostics.h"
#include "call.h"
#include "scope.h"

// ---------------------------------------------------------------------------------------------------------------------
// Internal Constructors

Scope* InternalScopeConstructor(Scope* inheritedScope)
{
    auto scope = ScopeConstructor(inheritedScope);
    AddRuntimeScope(scope);

    return scope;
}

Scope* InternalCopyScope(Scope* scopeToCopy)
{
    Scope* scope;
    if(scopeToCopy == &NothingScope || scopeToCopy == nullptr)
    {
       scope = ScopeConstructor(nullptr);
    }
    else
    {
        scope = CopyScope(scopeToCopy);
    }
    
    AddRuntimeScope(scope);

    return scope;
}

Call* InternalCallConstructor(const String* name=nullptr)
{
    Call* call = CallConstructor(name);
    BindScope(call, &NothingScope);
    BindType(call, &NullType);
    AddRuntimeCall(call);

    return call;
}

Call* InternalPrimitiveCallConstructor(BindingType type, int value)
{
    Call* foundCall = nullptr;
    if(ListContainsPrimitiveCall(RuntimeCalls, value, &foundCall))
    {
        return foundCall;
    }
    else if(ListContainsPrimitiveCall(ConstPrimitives, value, &foundCall))
    {
        return foundCall;
    }
    else
    {
        Call* call = InternalCallConstructor();
        BindType(call, type);
        call->Value = static_cast<void*>(ObjectValueConstructor(value));

        return call;
    }
}

Call* InternalPrimitiveCallConstructor(BindingType type, double value)
{
    Call* foundCall = nullptr;
    if(ListContainsPrimitiveCall(RuntimeCalls, value, &foundCall))
    {
        return foundCall;
    }
    else if(ListContainsPrimitiveCall(ConstPrimitives, value, &foundCall))
    {
        return foundCall;
    }
    else
    {
        Call* call = InternalCallConstructor();
        BindType(call, type);
        call->Value = static_cast<void*>(ObjectValueConstructor(value));

        return call;
    }
}


Call* InternalPrimitiveCallConstructor(BindingType type, bool value)
{
    Call* foundCall = nullptr;
    if(ListContainsPrimitiveCall(RuntimeCalls, value, &foundCall))
    {
        return foundCall;
    }
    else if(ListContainsPrimitiveCall(ConstPrimitives, value, &foundCall))
    {
        return foundCall;
    }
    else
    {
        Call* call = InternalCallConstructor();
        BindType(call, type);
        call->Value = static_cast<void*>(ObjectValueConstructor(value));

        return call;
    }
}


Call* InternalPrimitiveCallConstructor(BindingType type, String& value)
{
    Call* foundCall = nullptr;
    if(ListContainsPrimitiveCall(RuntimeCalls, value, &foundCall))
    {
        return foundCall;
    }
    else if(ListContainsPrimitiveCall(ConstPrimitives, value, &foundCall))
    {
        return foundCall;
    }
    else
    {
        Call* call = InternalCallConstructor();
        BindType(call, type);
        call->Value = static_cast<void*>(ObjectValueConstructor(value));

        return call;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper methods
int IndexOfNop()
{
    for(size_t i=0; i<BCI_NumberOfInstructions; i++)
    {
        if(BCI_Instructions[i] == BCI_NOP)
        {
            return i;
        }
    }
    return -1;
}

int IndexOfInstruction(BCI_Method bci)
{
    for(size_t i=0; i<BCI_NumberOfInstructions; i++)
    {
        if(BCI_Instructions[i] == bci)
        {
            return i;
        }
    }
    LogIt(LogSeverityType::Sev3_Critical, "IndexOfInstruction", "instruction not found, returned NOP");
    return IndexOfNop();
}


// ---------------------------------------------------------------------------------------------------------------------
// Instructions Array

BCI_Method BCI_Instructions[] = {
    BCI_LoadCallName,
    BCI_LoadPrimitive,

    BCI_Assign,

    BCI_Add,
    BCI_Subtract,
    BCI_Multiply,
    BCI_Divide,
    
    BCI_SysCall,
    
    BCI_And,
    BCI_Or,
    BCI_Not,

    BCI_NotEquals,
    BCI_Equals,
    BCI_Cmp,
    BCI_LoadCmp,

    BCI_JumpFalse,
    BCI_Jump,

    BCI_Copy,
    BCI_BindType,

    BCI_ResolveDirect,
    BCI_ResolveScoped,

    BCI_DefMethod,
    BCI_BindSection,
    BCI_EvalHere,
    BCI_Eval,
    BCI_Return,

    BCI_Array,

    BCI_EnterLocal,
    BCI_LeaveLocal,

    BCI_Extend,
    BCI_NOP,
    BCI_Dup,
    BCI_EndLine,

    BCI_Swap,
    BCI_JumpNothing,
    BCI_DropTOS,
};




// ---------------------------------------------------------------------------------------------------------------------
// Instruction Helpers

template <typename T>
inline T* PopTOS()
{
    T* tos = static_cast<T*>(MemoryStack.back());
    MemoryStack.pop_back();
    return tos;
}

template <typename T>
inline void PushTOS(T* entity)
{
    MemoryStack.push_back(static_cast<void*>(entity));
}

template <typename T>
inline T* PeekTOS()
{
    return static_cast<T*>(MemoryStack.back());
}

/// finds and returns a Reference with [callName] in [scope] without checking 
/// the inherited scope
inline Call* FindInScopeOnlyImmediate(Scope* scope, const String* callName)
{
    for(auto call: scope->CallsIndex)
    {
        if(call->Name == callName)
            return call;
    }
    return nullptr;
}

/// finds and returns a Reference with [callName] in [scope] and checks
/// the entire chain of inherited scope
inline Call* FindInScopeChain(Scope* scope, const String* callName)
{
    for(auto s = scope; s != nullptr; s = s->InheritedScope)
    {
        for(auto call: s->CallsIndex)
        {
            if(call->Name == callName)
                return call;
        }
    }
    return nullptr;
}

inline void InternalJumpTo(extArg_t ins)
{
    InstructionReg = ins;
    JumpStatusReg = 1;
}

inline Scope* ScopeOf(Call* call)
{
    return call->BoundScope;
}

/// return the size of the MemoryStack
inline size_t MemoryStackSize()
{
    return MemoryStack.size();
}

/// add a new CallFrame to the CallStack with [caller] and [self] the new caller and self
/// objects. changes registers appropriately but does not change the instruction pointer
inline void EnterNewCallFrame(extArg_t callerRefId, Call* caller, Call* self)
{
    std::vector<Scope> localScopeStack;
    CallStack.push_back({ InstructionReg+1, MemoryStackSize(), callerRefId, localScopeStack, LastResultReg });
    PushTOS<Call>(caller);
    PushTOS<Call>(self);

    CallerReg = caller;
    SelfReg = self;
    LocalScopeReg = self->BoundScope;
    LastResultReg = nullptr;
}

/// true if both [obj1] and [obj2] are of [cls]
inline bool BothAre(Call* call1, Call* call2, BindingType type)
{
    return call1->BoundType == type && call2->BoundType == type;
}

/// adds [ref] to [scp]
inline void AddCallToScope(Call* call, Scope* scp)
{
    scp->CallsIndex.push_back(call);
}

/// add a list of Objects [paramsList] to the scope of [methodCall] in reverse order
/// used when evaluating a method
inline void AddParamsToMethodScope(Call* methodCall, std::vector<Call*> paramsList)
{
    if(paramsList.size() != methodCall->BoundScope->CallParameters.size())
    {
        ReportFatalError(SystemMessageType::Exception, 2, 
            Msg("expected %i arguments but got %i", methodCall->BoundScope->CallParameters.size(), paramsList.size()));
    }

    if(paramsList.size() == 0)
    {
        return;
    }

    for(size_t i =0; i<paramsList.size() && i<methodCall->BoundScope->CallParameters.size(); i++)
    {
        auto call = InternalCallConstructor(methodCall->BoundScope->CallParameters[i]);
        auto callParam = paramsList[paramsList.size()-1-i];
        BindScope(call, callParam->BoundScope);
        BindSection(call, callParam->BoundSection);
        /// TODO: type checking here
        BindType(call, callParam->BoundType);
        BindValue(call, callParam->Value);

        AddCallToScope(call, ScopeOf(methodCall));
    }
}

/// pops [numParams] objects from the MemoryStack and returns a list of these elements
/// (which is the reverse order due to the push/pop algorithm)
inline std::vector<Call*> GetParameters(extArg_t numParams)
{
    std::vector<Call*> params;
    params.reserve(numParams);
    for(extArg_t i=0; i<numParams; i++)
    {
        params.push_back(PopTOS<Call>());
    }
    
    return params;
}

/// returns the [n]th bit of [data]
inline bool NthBit(uint8_t data, int n)
{
    return (data & (BitFlag << n)) >> n;
}

extArg_t IndexAfterNextJump()
{
    extArg_t i = InstructionReg;
    for(; i<ByteCodeProgram.size() && 
        (ByteCodeProgram[i].Op != IndexOfInstruction(BCI_Jump)); i++);
    return i+1;
}

/// fills in <= >= using the comparisions already made
inline void FillInRestOfComparisons()
{
    uint8_t geq = NthBit(CmpReg, 2) | NthBit(CmpReg, 4);
    uint8_t leq = NthBit(CmpReg, 2) | NthBit(CmpReg, 3);

    CmpReg = CmpReg | (geq << 6) | (leq << 5);
}

/// sets CmpReg to the result of comparing [lhs] and [rhs] as integers
inline void CompareIntegers(Call* lhs, Call* rhs)
{
    int lVal = IntegerValueOf(lhs);
    int rVal = IntegerValueOf(rhs);

    if(lVal < rVal)
    {
        CmpReg = CmpReg | (BitFlag << 3);
    }
    else if(lVal > rVal)
    {
        CmpReg = CmpReg | (BitFlag << 4);
    }
    else
    {
        CmpReg = CmpReg | (BitFlag << 2);
    }

    FillInRestOfComparisons();
}

/// sets CmpReg to the result of comparing [lhs] and [rhs] as decimals
inline void CompareDecimals(Call* lhs, Call* rhs)
{
    double lVal = DecimalValueOf(lhs);
    double rVal = DecimalValueOf(rhs);

    if(lVal < rVal)
    {
        CmpReg = CmpReg | (BitFlag << 3);
    }
    else if(lVal > rVal)
    {
        CmpReg = CmpReg | (BitFlag << 4);
    }
    else
    {
        CmpReg = CmpReg | (BitFlag << 2);
    }

    FillInRestOfComparisons();
}

/// changes the LocalScopeReg whenever a LocalScope change occurs. the base scope
/// is either the program scope or the self object scope
void AdjustLocalScopeReg()
{
    if(LocalScopeStack().size() == 0)
    {
        if(SelfReg->BoundScope != &NothingScope)
        {
            LocalScopeReg = SelfReg->BoundScope;
        }
        else
        {
            LocalScopeReg = ProgramReg;
        }
    }
    else
    {
        LocalScopeReg = &LocalScopeStack().back();
    }
}

/// resolves [callName] which is a keyword to the appropriate object which is 
/// pushed to TOS
inline void ResolveKeywordCall(const String* callName)
{
    Call* call = &NothingCall;
    if(*callName == "caller")
    {
        call = CallerReg;
    }
    else if(*callName == "self")
    {
        call = SelfReg;
    }
    else
    {
        if(LastResultReg == nullptr)
        {
            call = &NothingCall;
        }
        else
        {
            call = LastResultReg;
        }
    }

    PushTOS<Call>(call);
}

inline bool CallsAreEqual(Call* call1, Call* call2)
{
    return call1->BoundType == call2->BoundType &&
        call1->BoundScope == call2->BoundScope &&
        call1->BoundSection == call2->BoundSection &&
        call1->Value == call2->Value;
}


inline void HandleArrayInitialization(Call* caller, Call* size)
{
    int arraySize = IntegerValueOf(size);
    IfNeededAddArrayIndexCalls(arraySize);
    for(int i=0; i<arraySize; i++)
    {
        auto callatIndex = InternalCallConstructor(CallNamePointerFor(i));
        AddCallToScope(callatIndex, ScopeOf(caller));
    }

    auto sizeCall = InternalCallConstructor(&SizeCallName);
    BindType(sizeCall, &IntegerType);
    BindValue(sizeCall, ObjectValueConstructor(arraySize));
    AddCallToScope(sizeCall, ScopeOf(caller));

    PushTOS<Call>(caller);
}




// ---------------------------------------------------------------------------------------------------------------------
// Instruction Defintions

/// no assumptions
/// leaves the String callName as TOS
void BCI_LoadCallName(extArg_t arg)
{
    PushTOS<String>(&CallNames[arg]);
}

/// no asumptions
/// leaves the primitive specified by arg as TOS
void BCI_LoadPrimitive(extArg_t arg)
{
    if(arg >= ConstPrimitives.size())
        return; 

    PushTOS<Call>(ConstPrimitives[arg]);
}

/// assumes TOS is the new object and TOS1 is the reference to reassign
/// leaves the object
void BCI_Assign(extArg_t arg)
{
    auto rhs = PopTOS<Call>();
    auto lhs = PopTOS<Call>();

    BindSection(lhs, rhs->BoundSection);
    BindScope(lhs, rhs->BoundScope);
    lhs->Value = rhs->Value;

    /// TODO: Type check here
    if(rhs->BoundType != &NullType || lhs->BoundType == &NullType)
    {
        BindType(lhs, rhs->BoundType);
    }

    PushTOS<Call>(lhs);
}

/// assumes TOS1 and TOS2 are rhs object and lhs object respectively
/// leaves the result as TOS
void BCI_Add(extArg_t arg)
{
    auto rCall = PopTOS<Call>();
    auto lCall = PopTOS<Call>();

    Call* call = nullptr;
    if(BothAre(lCall, rCall, &IntegerType))
    {
        int ans = IntegerValueOf(lCall) + IntegerValueOf(rCall);
        call = InternalPrimitiveCallConstructor(&IntegerType, ans);
    }
    else if(BothAre(lCall, rCall, &DecimalType))
    {
        double ans = DecimalValueOf(lCall) + DecimalValueOf(rCall);
        call = InternalPrimitiveCallConstructor(&DecimalType, ans);
    }
    else if(BothAre(lCall, rCall, &StringType))
    {
        String ans = StringValueOf(lCall) + StringValueOf(rCall);
        call = InternalPrimitiveCallConstructor(&StringType, ans);
    }
    else
    {
        call = &NothingCall;
        ReportError(SystemMessageType::Warning, 0, Msg("cannot add types %s and %s", lCall->BoundType, rCall->BoundType));
    }

    PushTOS<Call>(call);
}

void BCI_Subtract(extArg_t arg)
{
    auto rCall = PopTOS<Call>();
    auto lCall = PopTOS<Call>();

    Call* call = nullptr;
    if(BothAre(lCall, rCall, &IntegerType))
    {
        int ans = IntegerValueOf(lCall) - IntegerValueOf(rCall);
        call = InternalPrimitiveCallConstructor(&IntegerType, ans);
    }
    else if(BothAre(lCall, rCall, &DecimalType))
    {
        double ans = DecimalValueOf(lCall) - DecimalValueOf(rCall);
        call = InternalPrimitiveCallConstructor(&DecimalType, ans);
    }
    else
    {
        call = &NothingCall;
        ReportError(SystemMessageType::Warning, 0, Msg("cannot add types %s from %s", rCall->BoundType, lCall->BoundType));
    }

    PushTOS<Call>(call);
}

void BCI_Multiply(extArg_t arg)
{
    auto rCall = PopTOS<Call>();
    auto lCall = PopTOS<Call>();

    Call* call = nullptr;
    if(BothAre(lCall, rCall, &IntegerType))
    {
        int ans = IntegerValueOf(lCall) * IntegerValueOf(rCall);
        call = InternalPrimitiveCallConstructor(&IntegerType, ans);
    }
    else if(BothAre(lCall, rCall, &DecimalType))
    {
        double ans = DecimalValueOf(lCall) * DecimalValueOf(rCall);
        call = InternalPrimitiveCallConstructor(&DecimalType, ans);
    }
    else
    {
        call = &NothingCall;
        ReportError(SystemMessageType::Warning, 0, Msg("cannot multiply types %s from %s", lCall->BoundType, rCall->BoundType));
    }

    PushTOS<Call>(call);
}

void BCI_Divide(extArg_t arg)
{
    auto rCall = PopTOS<Call>();
    auto lCall = PopTOS<Call>();

    Call* call = nullptr;
    if(BothAre(lCall, rCall, &IntegerType))
    {
        int ans = IntegerValueOf(lCall) / IntegerValueOf(rCall);
        call = InternalPrimitiveCallConstructor(&IntegerType, ans);
    }
    else if(BothAre(lCall, rCall, &DecimalType))
    {
        double ans = DecimalValueOf(lCall) / DecimalValueOf(rCall);
        call = InternalPrimitiveCallConstructor(&DecimalType, ans);
    }
    else
    {
        call = &NothingCall;
        ReportError(SystemMessageType::Warning, 0, Msg("cannot divide types %s by %s", lCall->BoundType, rCall->BoundType));
    }

    PushTOS<Call>(call);
}

/// 0 = print
/// 1 = ask
void BCI_SysCall(extArg_t arg)
{
    switch(arg)
    {
        case 0:
        {
            String msg = StringValueOf(PeekTOS<Call>()) + "\n";
            ProgramOutput += msg;
            if(g_outputOn)
            {
                std::cout << msg;
            }
        }
        break;

        case 1:
        {
            String s;
            std::getline(std::cin, s);
            std::cout << s;
            auto call = InternalPrimitiveCallConstructor(&StringType, s);
            PushTOS<Call>(call);
        }
        break;
        
        default:
        break;
    }
}

void BCI_And(extArg_t arg)
{
    auto rCall = PopTOS<Call>();
    auto lCall = PopTOS<Call>();

    bool b = BooleanValueOf(lCall) && BooleanValueOf(rCall);
    Call* call = InternalPrimitiveCallConstructor(&BooleanType, b);

    PushTOS<Call>(call);
}

void BCI_Or(extArg_t arg)
{
    auto rCall = PopTOS<Call>();
    auto lCall = PopTOS<Call>();

    bool b = BooleanValueOf(lCall) || BooleanValueOf(rCall);
    Call* call = InternalPrimitiveCallConstructor(&BooleanType, b);

    PushTOS<Call>(call);
}

void BCI_Not(extArg_t arg)
{
    auto c = PopTOS<Call>();

    bool b = !BooleanValueOf(c);
    Call* call = InternalPrimitiveCallConstructor(&BooleanType, b);

    PushTOS<Call>(call);
}

/// assumes TOS1 1 and TOS2 are rhs object and lhs object respectively
/// leaves obj (bool compare result) as TOS
void BCI_NotEquals(extArg_t arg)
{
    auto rCall = PopTOS<Call>();
    auto lCall = PopTOS<Call>();

    bool b = !CallsAreEqual(rCall, lCall);
    Call* call = InternalPrimitiveCallConstructor(&BooleanType, b);

    PushTOS<Call>(call);
}

/// assumes TOS1 1 and TOS2 are rhs object and lhs object respectively
/// leaves obj (bool compare result) as TOS
void BCI_Equals(extArg_t arg)
{
    auto rCall = PopTOS<Call>();
    auto lCall = PopTOS<Call>();

    bool b = CallsAreEqual(rCall, lCall);
    Call* call = InternalPrimitiveCallConstructor(&BooleanType, b);

    PushTOS<Call>(call);
}

/// assumes TOS1 1 and TOS2 are rhs object and lhs object respectively
/// sets CmpReg to appropriate value
void BCI_Cmp(extArg_t arg)
{
    CmpReg = CmpRegDefaultValue;
    auto rCall = PopTOS<Call>();
    auto lCall = PopTOS<Call>();
    

    if(BothAre(lCall, rCall, &IntegerType))
    {
        CompareIntegers(lCall, rCall);
    }
    else if(BothAre(lCall, rCall, &DecimalType))
    {
        CompareDecimals(lCall, rCall);
    }
    else
    {
        ReportError(SystemMessageType::Warning, 0, Msg("cannot compare types %s and %s", lCall->BoundType, rCall->BoundType));
    }
}

/// assumes CmpReg is valued
/// leaves a new Boolean object as TOS 
void BCI_LoadCmp(extArg_t arg)
{
    Call* boolCall = nullptr;
    switch(arg)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4: 
        case 5:
        case 6:
        boolCall = InternalPrimitiveCallConstructor(&BooleanType, NthBit(CmpReg, arg));
        break;

        default:
        return;
    }

    PushTOS<Call>(boolCall);
}

void BCI_JumpFalse(extArg_t arg)
{
    auto call = PopTOS<Call>();
    if(!BooleanValueOf(call))
    {
        InternalJumpTo(arg);
    }
}

void BCI_Jump(extArg_t arg)
{
    InternalJumpTo(arg);
}

/// assumes TOS is an object
/// TODO: may not work with primitives
/// TODO: should deep copy scope
/// TODO: maybe depreciated
/// leaves object copy as TOS
void BCI_Copy(extArg_t arg)
{
    auto call = PopTOS<Call>();
    auto callCopy = InternalCallConstructor();
    BindSection(callCopy, call->BoundSection);
    
    auto scp = InternalCopyScope(call->BoundScope);
    BindScope(callCopy, scp);
    BindType(callCopy, call->BoundType);

    PushTOS<Call>(callCopy);
}

/// assumes TOS is a Call
/// leaves object of type as TOS
void BCI_BindType(extArg_t arg)
{
    auto call = PeekTOS<Call>();
    BindType(call, call->Name);
}

/// assumes TOS is a String
/// leaves resolved reference as TOS
void BCI_ResolveDirect(extArg_t arg)
{
    auto callName = PopTOS<String>();

    if(CallNameIsKeyword(callName))
    {
        ResolveKeywordCall(callName);
        return;
    }

    auto resolvedCall = FindInScopeOnlyImmediate(LocalScopeReg, callName);
    if(resolvedCall == nullptr)
    {
        resolvedCall = FindInScopeChain(SelfReg->BoundScope, callName);
    }

    if(resolvedCall == nullptr)
    {
        resolvedCall = FindInScopeOnlyImmediate(CallerReg->BoundScope, callName);
    }

    if(resolvedCall == nullptr)
    {
        resolvedCall = FindInScopeOnlyImmediate(ProgramReg, callName);
    }

    if(resolvedCall == nullptr)
    {
        auto newCall = InternalCallConstructor(callName);
        AddCallToScope(newCall, LocalScopeReg);
        PushTOS<Call>(newCall);
    }
    else
    {
        PushTOS<Call>(resolvedCall);
    }

}

/// assumes TOS is a string and TOS1 is an obj
/// leaves resolved reference as TOS
void BCI_ResolveScoped(extArg_t arg)
{
    auto callName = PopTOS<String>();
    if(PeekTOS<Call>()->BoundScope == &NothingScope)
    {
        ReportError(SystemMessageType::Warning, 1, Msg("%s has no bound scope which cannot have attributes", PeekTOS<Call>()->Name));
        return;
    }

    auto callerCall = PopTOS<Call>();
    auto resolvedCall = FindInScopeOnlyImmediate(ScopeOf(callerCall), callName);

    if(resolvedCall == nullptr)
    {
        auto newCall = InternalCallConstructor(callName);
        AddCallToScope(newCall, ScopeOf(callerCall));
        PushTOS<Call>(newCall);
    }
    else
    {
        PushTOS<Call>(resolvedCall);
    }
}

/// assumes the top [arg] entities on the stack are parameter names (strings)
/// leaves a new method object as TOS
void BCI_DefMethod(extArg_t arg)
{
    String** argList;

    if(arg != 0)
    {
        argList = new String*[arg];
        for(extArg_t i = 1; i<=arg; i++)
        {
            auto str = PopTOS<String>();
            argList[arg-i] = str;
        }
    }

    auto call = InternalCallConstructor();
    auto scope =  InternalScopeConstructor(nullptr);
    BindScope(call, scope);
    BindType(call, &MethodType);

    if(arg != 0)
    {
        for(extArg_t i=0; i<arg; i++)
        {
            call->BoundScope->CallParameters.push_back(argList[i]);
        }
        
        delete[] argList;
    }

    PushTOS<Call>(call);
}

/// assumes TOS is a Call
/// binds arg as the section for that call
void BCI_BindSection(extArg_t arg)
{
    auto call = PeekTOS<Call>();
    BindSection(call, arg);
}

/// arg is number of parameters
/// assumes TOS[arg] are objs (params), TOS[arg+1] is an object (method), TOS[arg+2] an obj (caller)
/// adds caller and self to TOS (in that order)
void BCI_Eval(extArg_t arg)
{
    auto paramsList = GetParameters(arg);
    auto methodCall = PopTOS<Call>();
    auto caller = PopTOS<Call>();

    if(methodCall->BoundType == &ArrayType)
    {
        HandleArrayInitialization(methodCall, paramsList[0]);
        return;
    }

    if(methodCall->BoundSection == 0)
    {
        auto methodName = (methodCall->Name == nullptr ? "anonymous variable" : *methodCall->Name);
        ReportFatalError(SystemMessageType::Exception, 3, Msg("%s cannot be called", methodName));
        return;
    }

    extArg_t jumpIns= methodCall->BoundSection;
    AddParamsToMethodScope(methodCall, paramsList);

    /// TODO: figure out caller id
    EnterNewCallFrame(0, caller, methodCall);
    
    InternalJumpTo(jumpIns);
}

/// arg is number of parameters
/// assumes TOS[arg] are objs (params), TOS[arg+1] is an object (method)
/// adds caller and self to TOS (in that order)
void BCI_EvalHere(extArg_t arg)
{
    auto paramsList = GetParameters(arg);
    auto methodCall = PopTOS<Call>();

    extArg_t jumpIns= methodCall->BoundSection;
    AddParamsToMethodScope(SelfReg, paramsList);

    /// TODO: figure out caller id
    EnterNewCallFrame(0, CallerReg, SelfReg);
    
    InternalJumpTo(jumpIns);
}

/// arg is bool flag on whether or not to return a specific object
void BCI_Return(extArg_t arg)
{
    extArg_t jumpBackTo = CallStack.back().ReturnToInstructionId;
    extArg_t stackStart = CallStack.back().MemoryStackStart;

    Call* returnObj = nullptr;
    if(arg == 1)
    {
        returnObj = PopTOS<Call>();
    }

    while(MemoryStack.size() > stackStart)
    {
        PopTOS<void>();
    }

    if(arg == 1)
    {
        PushTOS(returnObj);
    }
    else
    {
        if(CallerReg->BoundType != &NullType)
        {
            PushTOS(CallerReg);
        }
        else
        {
            PushTOS(SelfReg);
        }
    }

    InstructionReg = jumpBackTo;

    LastResultReg = CallStack.back().LastResult;
    CallStack.pop_back();

    extArg_t returnMemStart = CallStack.back().MemoryStackStart;

    CallerReg = static_cast<Call*>(MemoryStack[returnMemStart]);
    SelfReg = static_cast<Call*>(MemoryStack[returnMemStart+1]);

    LocalScopeStack() = CallStack.back().LocalScopeStack;
    AdjustLocalScopeReg();

    JumpStatusReg = 1;
}

/// assumes TOS is index and TOS1 is the parent call
/// leaves the Call indexed as TOS
void BCI_Array(extArg_t arg)
{
    auto indexCall = PopTOS<Call>();
    auto arrayCall = PopTOS<Call>();

    auto sizeCall = FindInScopeOnlyImmediate(ScopeOf(arrayCall), &SizeCallName);

    if(indexCall->BoundType != &IntegerType)
    {
        ReportFatalError(SystemMessageType::Exception, 5, Msg("%s is not an integer", *indexCall->Name));
        return;
    }

    if(sizeCall == nullptr)
    {
        ReportFatalError(SystemMessageType::Exception, 4, Msg("%s is not an array", *arrayCall->Name));
        return;
    }

    auto indexInt = IntegerValueOf(indexCall);
    auto sizeInt = IntegerValueOf(sizeCall);

    if(indexInt >= sizeInt)
    {
        ReportFatalError(SystemMessageType::Exception, 6, Msg("%i is out of bounds in %s of size %i", indexInt, *arrayCall->Name, sizeInt));
        return;
    }

    auto indexedCall = FindInScopeOnlyImmediate(ScopeOf(arrayCall), CallNamePointerFor(indexInt));
    if(indexCall == nullptr)
    {
        LogIt(LogSeverityType::Sev3_Critical, "BCI_Array", "cannot find indexed call");
    }

    PushTOS(indexedCall);
}

/// no assumptions
/// does not change TOS. will update LocalScopeReg
void BCI_EnterLocal(extArg_t arg)
{
    LocalScopeStack().push_back( {{}, LocalScopeReg, false} );
    LocalScopeReg = &LocalScopeStack().back();
    LastResultReg = nullptr;
}

/// no assumptions
/// does not change TOS. will update LocalScopeReg and LastResultReg
void BCI_LeaveLocal(extArg_t arg)
{
    /// TODO: destroy andy local references
    LocalScopeStack().pop_back();
    AdjustLocalScopeReg();
    LastResultReg = nullptr;
}

/// no assumptions
/// adds [arg] as the (ExtensionExp + 1)th bit of ExtendedArg and increments ExtensionExp
void BCI_Extend(extArg_t arg)
{
    switch (ExtensionExp)
    {
        case 0:
        case 1:
        case 2: 
        case 3:
        case 4:
        case 5:
        case 6:
        {
            ExtensionExp++;
            extArg_t shiftedExpr = ((extArg_t)arg) << (8 * ExtensionExp);
            ExtendedArg = ExtendedArg ^ shiftedExpr;
            break;
        }


        default:
        break;
    }
}

/// no assumptions
/// no actions, used for optimizations
void BCI_NOP(extArg_t arg)
{
    // does nothing, used for optimizations
}

/// no assumptions
/// pushes a pointer to the old TOS to the TOS
void BCI_Dup(extArg_t arg)
{
    auto entity = PeekTOS<void>();
    PushTOS<void>(entity);
}

/// assumes TOS is an object
/// changes LastResultReg to this object
void BCI_EndLine(extArg_t arg)
{
    LastResultReg = PopTOS<Call>();
}

/// swaps TOS with TOS1
void BCI_Swap(extArg_t arg)
{
    void* TOS = PopTOS<void>();
    void* TOS1 = PopTOS<void>();

    PushTOS<void>(TOS);
    PushTOS<void>(TOS1);
}

/// assumes TOS is an object
/// pops TOS and jumps if obj == Nothing
void BCI_JumpNothing(extArg_t arg)
{
    auto TOS = PopTOS<Call>();
    if(TOS->BoundType == &NullType)
    {
        InternalJumpTo(arg);
    }
}

/// assumes TOS exists
/// pops TOS 
void BCI_DropTOS(extArg_t arg)
{
    PopTOS<void>();
}
