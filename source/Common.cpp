#include "Pch.h"
#include "cas/Common.h"
#include "cas/IObject.h"

cas::Value::Value(cas::IObject* obj) : type(obj->GetType()), obj(obj)
{

}
