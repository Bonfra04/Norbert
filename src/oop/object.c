#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <sys/mman.h>

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
    memcpy(self->codePagePtr + 2, &self, sizeof(void *));
    memcpy(self->codePagePtr + 10, kTrampoline + 10, sizeof(kTrampoline) - 10);
    memcpy(self->codePagePtr + 16, &target, sizeof(void *));
    self->codePagePtr += sizeof(kTrampoline);
    return self->codePagePtr - sizeof(kTrampoline);
}

void Object_destroy(Object* self)
{
    int ret = munmap(self->codePage, self->codePageSize);
    assert(!ret);
    free(self);
}

void* Object_create(size_t size, int functionCount)
{
    functionCount += 1;

    Object *self = malloc(size);
    self->codePageSize = functionCount * sizeof(kTrampoline);
    self->codePage = mmap(NULL, self->codePageSize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    self->codePagePtr = self->codePage;

    self->destroy = Object_trampoline(self, Object_destroy, 0);

    return self;
}

void Object_prepare(Object* self)
{
    int ret = mprotect(self->codePage, self->codePageSize, PROT_READ | PROT_EXEC);
    assert(!ret);
}
