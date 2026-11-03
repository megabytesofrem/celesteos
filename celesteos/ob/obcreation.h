#ifndef CELESTEOS_OBCREATION_H
#define CELESTEOS_OBCREATION_H

#include <ob/obheader.h>
#include <ob/obpool.h>

/**
 * Allocate an object in the specified pool without initializing it with data.
 */
ObHeader *ObAllocInPool(ObObjectPool *pool, ObObjectType type);

/**
 * Create an object in the specified pool, initializing it with the provided data.
 */
ObHeader *ObCreateInPool(ObObjectPool *pool, ObObjectType type, ObObjectData data);

/**
 * Retain a reference to the object, incrementing its reference count.
 */
void ObRetain(ObHeader *object);

/**
 * Release a reference to the object, decrementing its reference count.
 */
bool ObRelease(ObHeader *object);

/**
 * Destroy the object, releasing any associated resources.
 */
void ObDestroy(ObHeader *object);

#endif // CELESTEOS_OBCREATION_H
