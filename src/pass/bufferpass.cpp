#include "reffine/pass/bufferpass.h"

#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

/*

Implementation:
https://github.com/natalievolk/reffine/blob/vectorize/main.cpp#L287

- every call of FetchDataPtr on an ArrowTable should be transformed into:
    - FetchDataPtr on the ArrowTable
    - create a Sym around it
    - call the get_elem_ptr on it
- then use this returned ptr just like a buffer
*/