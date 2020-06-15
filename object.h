#ifndef __OBJECT_H
#define __OBJECT_H

#include "main.h"
#include "arch.h"

ObjectReferenceMap* EntryInIndexOf(const Object* obj);

Reference* CreateReferenceToNewObject(String name, ObjectClass objClass, bool value);
Reference* CreateReferenceToNewObject(String name, ObjectClass objClass, int value);
Reference* CreateReferenceToNewObject(String name, ObjectClass objClass, const String value);
Reference* CreateReferenceToNewObject(String name, ObjectClass objClass, double value);
Reference* CreateReference(String name, Object* obj);
Reference* CreateNullReference();
Reference* CreateNullReference(String name);

Reference* CreateReferenceToNewObject(String name, Token* valueToken);
Reference* CreateReferenceToNewObject(Token* nameToken, Token* valueToken);


ObjectClass GetPrecedenceClass(const Object& obj1, const Object& obj2);
bool IsNumeric(const Reference& ref);


std::string GetStringValue(const Object& obj);
int GetIntValue(const Object& obj);
double GetDecimalValue(const Object& obj);
bool GetBoolValue(const Object& obj);

#endif