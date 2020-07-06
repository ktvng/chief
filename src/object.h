#ifndef __OBJECT_H
#define __OBJECT_H

#include "main.h"

// ---------------------------------------------------------------------------------------------------------------------
// Referables (Objects/Methods)

/// defines the class of an object which imbues type properties
typedef std::string ObjectClass;

/// type of a referable object
enum class ReferableType
{
    Object,
    Method
};

/// abstract class for anything that a Reference can point to
class Referable
{
    public:
    ReferableType Type;
    
};

/// emulated object in Pebble
/// [Class] refers to the Object Class which governs type properties
/// [Attributes] are references to other Objects/Methods
/// [Value] is used by primitive objects for basic operations.
///         there are currently only int*, double*, bool*, std::string* value types
class Object : public Referable
{
    public:
    ObjectClass Class;
    Scope* Attributes;
    void* Value;
};

/// emulated method in Pebble
/// [CodeBlock] is the code associated with the method
/// [Parameters] are the parameters input to the method
class Method : public Referable
{
    public:
    Block* CodeBlock;
    std::vector<std::string> ParameterNames;
};


// ---------------------------------------------------------------------------------------------------------------------
// ObjectClasses

inline const ObjectClass BaseClass = "Object";
inline const ObjectClass IntegerClass = "Integer";
inline const ObjectClass DecimalClass = "Decimal";
inline const ObjectClass StringClass = "String";
inline const ObjectClass BooleanClass = "Boolean";
inline const ObjectClass NullClass = "Nothing";
inline const ObjectClass ArrayClass = "Array";
inline const ObjectClass TupleClass = "Tuple";


// ---------------------------------------------------------------------------------------------------------------------
// Global Object Index

ObjectReferenceMap* EntryInIndexOf(const Object* obj);
void IndexObject(Object* obj, Reference* ref);


// ---------------------------------------------------------------------------------------------------------------------
// Access referenced objects/methods

Object* ObjectOf(const Reference* ref);
Method* MethodOf(const Reference* ref);


// ---------------------------------------------------------------------------------------------------------------------
// Create references and associated (primitive) object

Reference* CreateReferenceToNewObject(String name, ObjectClass objClass, bool value);
Reference* CreateReferenceToNewObject(String name, ObjectClass objClass, int value);
Reference* CreateReferenceToNewObject(String name, ObjectClass objClass, const String value);
Reference* CreateReferenceToNewObject(String name, ObjectClass objClass, double value);
Reference* CreateReferenceToNewObject(String name, ObjectClass objClass, void* value);


// ---------------------------------------------------------------------------------------------------------------------
// Create references to existing Referable (object/method)

Reference* CreateReference(String name, Referable* refable);


// ---------------------------------------------------------------------------------------------------------------------
// Create reference to NullObject (Nothing)

Reference* CreateNullReference();
Reference* CreateNullReference(String name);
Object* NullObject();


// ---------------------------------------------------------------------------------------------------------------------
// Create reference from Token 

Reference* CreateReferenceToNewObject(String name, Token* valueToken);
Reference* CreateReferenceToNewObject(Token* nameToken, Token* valueToken);


// ---------------------------------------------------------------------------------------------------------------------
// Object type handling

ObjectClass GetPrecedenceClass(const Object& obj1, const Object& obj2);
bool IsNumeric(const Reference* ref);
bool IsString(const Reference* ref);


// ---------------------------------------------------------------------------------------------------------------------
// Value for primitive objects

std::string GetStringValue(const Object& obj);
int GetIntValue(const Object& obj);
double GetDecimalValue(const Object& obj);
bool GetBoolValue(const Object& obj);

#endif
