#include "scope.h"

#include "reference.h"
#include "call.h"
#include "diagnostics.h"
#include <iostream>

// ---------------------------------------------------------------------------------------------------------------------
// Constructors

/// create a new scope with [inheritedScope]. all new scopes should be created from this
/// constructor method
Scope* ScopeConstructor(Scope* inheritedScope)
{
    Scope* s = new Scope;
    s->InheritedScope = inheritedScope;
    s->ReferencesIndex = {};
    s->IsDurable = false;

    return s;
}

void ScopeDestructor(Scope* scope)
{
    delete scope;
}

void AddReferenceToScope(Reference* ref, Scope* scope)
{
    scope->ReferencesIndex.push_back(ref);
}

Scope* CopyScope(Scope* scp)
{
    Scope* scpCopy = ScopeConstructor(scp->InheritedScope);

    for(auto call: scp->CallsIndex)
    {
        Call* callCopy = CallConstructor(call->Name);
        BindScope(callCopy, call->BoundScope);
        BindSection(callCopy, call->BoundSection);
        BindType(callCopy, call->BoundType);
    }

    scpCopy->CallParameters = scp->CallParameters;

    return scpCopy;
}
