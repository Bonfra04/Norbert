#define _GNU_SOURCE
#include <sys/mman.h>

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "object.h"

static const unsigned char kTrampoline[] = {
    // MOV RDI, 0x0
    0x48, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // JMP [RIP + 0]
    0xff, 0x25, 0x00, 0x00, 0x00, 0x00,
    // DQ 0x0
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void* Object_trampoline(Object* self, void* target, int argCount)
{
    unsigned char opcode[][2] = {
        { 0x48, 0xbf },
        { 0x48, 0xbe },
        { 0x48, 0xba },
        { 0x48, 0xb9 },
        { 0x49, 0xb8 },
        { 0x49, 0xb9 },
    };
    memcpy(self->codePagePtr, opcode[argCount], sizeof(opcode[argCount]));
    memcpy(self->codePagePtr + 2, &self, sizeof(void*));
    memcpy(self->codePagePtr + 10, kTrampoline + 10, sizeof(kTrampoline) - 10);
    memcpy(self->codePagePtr + 16, &target, sizeof(void*));
    self->codePagePtr += sizeof(kTrampoline);
    return self->codePagePtr - sizeof(kTrampoline);
}

void Object_delete(Object* self)
{
    void (*destructor)(Object*) = (void(*)(Object*))self->codePage;
    destructor(self);
    int ret = munmap(self->codePage, self->codePageSize);
    assert(!ret);
    free(self);
}

static void Object_destruct()
{
}

static void setDestructor(Object* self, void (*destructor)(Object*))
{
    memcpy(self->codePage, kTrampoline, sizeof(kTrampoline));
    memcpy(self->codePage + 2, &self, sizeof(void*));
    memcpy(self->codePage + 16, &destructor, sizeof(void*));
}

void* Object_new(size_t size, int functionCount, void (*destructor)(Object* self))
{
    functionCount += 2; // delete, destructor

    Object* self = malloc(size);
    self->codePageSize = functionCount * sizeof(kTrampoline);
    self->codePage = mmap(NULL, self->codePageSize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    self->codePagePtr = self->codePage;

    setDestructor(self, destructor);
    self->codePagePtr += sizeof(kTrampoline);
    self->delete = Object_trampoline(self, Object_delete, 0);
    self->destruct = Object_destruct;

    return self;
}

void Object_prepare(Object* self)
{
    int ret = mprotect(self->codePage, self->codePageSize, PROT_READ | PROT_EXEC);
    assert(!ret);
}

static void updateTrampoline(Object* self)
{
    for(unsigned char* t = self->codePage; t < self->codePagePtr; t += sizeof(kTrampoline))
    {
        memcpy(t + 2, &self, sizeof(void*));
    }
}

void* Object_fromSuper(Object* super, size_t superSize, size_t size, int functionCount, void (*destructor)(Object* self))
{
    functionCount += 2; // destructor

    Object* self = malloc(size);
    memcpy(self, super, superSize);
    self->codePageSize = self->codePageSize + functionCount * sizeof(kTrampoline);
    self->codePage = mremap(super->codePage, super->codePageSize, self->codePageSize, MREMAP_MAYMOVE);
    self->codePagePtr = self->codePage + super->codePageSize;
    free(super);

    mprotect(self->codePage, self->codePageSize, PROT_READ | PROT_WRITE);

    setDestructor(self, destructor);
    updateTrampoline(self);

    return self;
}

void* Object_override(void* target, void* superTrampoline)
{
    memcpy((unsigned char*)superTrampoline + 16, &target, sizeof(void*));
    return superTrampoline;
}
