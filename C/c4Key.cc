//
//  c4Key.cc
//  Couchbase Lite Core
//
//  Created by Jens Alfke on 11/6/15.
//  Copyright (c) 2015-2016 Couchbase. All rights reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
//  Unless required by applicable law or agreed to in writing, software distributed under the
//  License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
//  either express or implied. See the License for the specific language governing permissions
//  and limitations under the License.

#include "c4Internal.hh"
#include "c4KeyInternal.hh"
#include "c4Key.h"
#include "c4Document.h"
#include "Collatable.hh"
#include <math.h>
#include <limits.h>
using namespace litecore;


C4Key* c4key_new()                              noexcept {return new c4Key();}
C4Key* c4key_withBytes(C4Slice bytes)           noexcept {return new c4Key(bytes);}
void c4key_reset(C4Key *key)                    noexcept {key->reset();}
void c4key_free(C4Key *key)                     noexcept {delete key;}
void c4key_addNull(C4Key *key)                  noexcept {key->addNull();}
void c4key_addBool(C4Key *key, bool b)          noexcept {key->addBool(b);}
void c4key_addNumber(C4Key *key, double n)      noexcept {*key << n;}
void c4key_addString(C4Key *key, C4Slice str)   noexcept {*key << str;}
void c4key_addMapKey(C4Key *key, C4Slice mapKey)noexcept {*key << mapKey;}
void c4key_beginArray(C4Key *key)               noexcept {key->beginArray();}
void c4key_endArray(C4Key *key)                 noexcept {key->endArray();}
void c4key_beginMap(C4Key *key)                 noexcept {key->beginMap();}
void c4key_endMap(C4Key *key)                   noexcept {key->endMap();}


// C4KeyReader is really identical to CollatableReader, which itself consists of nothing but
// a slice. So these functions use pointer-casting to reinterpret C4KeyReader as CollatableReader.

static inline C4KeyReader asKeyReader(const CollatableReader &r) noexcept {
    return *(C4KeyReader*)&r;
}


C4KeyReader c4key_read(const C4Key *key) noexcept {
    CollatableReader r(*key);
    return asKeyReader(r);
}

/** for java binding */
C4KeyReader* c4key_newReader(const C4Key *key) noexcept {
    return (C4KeyReader*)new CollatableReader(*key);
}

/** Free a C4KeyReader */
void c4key_freeReader(C4KeyReader* r) noexcept {
    delete r;
}

C4KeyToken c4key_peek(const C4KeyReader* r) noexcept {
    static constexpr C4KeyToken tagToType[] = {kC4EndSequence, kC4Null, kC4Bool, kC4Bool, kC4Number,
                                    kC4Number, kC4String, kC4Array, kC4Map, kC4Error, kC4Special};
    Collatable::Tag t = ((CollatableReader*)r)->peekTag();
    if (t >= sizeof(tagToType)/sizeof(tagToType[0]))
        return kC4Error;
    return tagToType[t];
}

void c4key_skipToken(C4KeyReader* r) noexcept {
    ((CollatableReader*)r)->skipTag();
}

bool c4key_readBool(C4KeyReader* r) noexcept {
    bool result = ((CollatableReader*)r)->peekTag() >= CollatableReader::kTrue;
    ((CollatableReader*)r)->skipTag();
    return result;
}

double c4key_readNumber(C4KeyReader* r) noexcept {
    try {
        return ((CollatableReader*)r)->readDouble();
    } catchExceptions()
    return nan("err");  // ????
}

C4SliceResult c4key_readString(C4KeyReader* r) noexcept {
    try {
        return sliceResult(((CollatableReader*)r)->readString());
    } catchExceptions()
    return {nullptr, 0};
}

C4SliceResult c4key_toJSON(const C4KeyReader* r) noexcept {
    if (!r || r->length == 0)
        return {nullptr, 0};
    string str = ((CollatableReader*)r)->toJSON();
    return sliceResult(str);
}


C4KeyValueList* c4kv_new() noexcept {
    return new c4KeyValueList;
}

void c4kv_add(C4KeyValueList *kv, C4Key *key, C4Slice value) noexcept {
    kv->keys.push_back(*key);
    kv->values.push_back(alloc_slice(value));
}

void c4kv_free(C4KeyValueList *kv) noexcept {
    delete kv;
}

void c4kv_reset(C4KeyValueList *kv) noexcept {
    kv->keys.clear();
    kv->values.clear();
}


