#pragma once
#include "atomicIntegral.h"
#include "transInterface.h"
#include "atomicVector.h"

typedef TransInterface<int> AtomInt;
typedef TransInterface<std::vector<int>> AtomIntVector;