# Doppler
Doppler is a high level Java-like programming language designed for CelesteOS.

## Features
- Java syntax for familiar object-oriented programming concepts
- Classes and objects (C structs with the `isa` pointer as the first field)
- Automatic Reference Counting (ARC) for memory management, similar to Objective-C's ARC system.
- Dedicated types for IPC payloads, enabling structured message passing between tasks.

## Differences from Java
- Doppler uses C-style structs with an `isa` pointer for object representation, unlike Java's garbage-collected objects.
- Doppler uses `new` and `delete` keywords from C++, because there is no GC.
- Doppler supports both structs and classes, unlike Java.
- Doppler uses opt-in iOS 2 style reference counting for memory management, unlike Java's garbage collector.
- Doppler supports unsigned integer types and raw pointers.
- Doppler transpiles to C, suitable for bare-metal or low-level system programming.

## Syntax
```java
struct MyObjectData {
    private int value;
    private bool flag;
}

class MyObject extends KObject {
    private MyObjectData ^data;

    @override public void sys_init() {
        // Initialization logic here
        @retain(self);
    }

    @override public KObject construct(int value, bool flag) {
        return new KObject(value, flag)
    }

    @override public void destroy() {
        // Cleanup logic here
        @release(self);
    }

    public int get_value()  {
        return self.data.value;
    }
}
```

## Opt in Reference Counting

Doppler uses an opt-in reference counting system similar to iOS 2's ARC. Memory management is handled using the following directives:
- `@autoreleasepool`: Defines a scope in which objects are automatically released at the end of the block.
- `@retain(self)`: Increases the reference count of the object.
- `@release(self)`: Decreases the reference count of the object, potentially freeing it if the count reaches zero.

```java
class MyObjectExample extends Object {
    private MyObject ^obj; /* ^ specifies reference counted object */

    public void construct() {
        @autoreleasepool {
            MyObject obj = new MyObject(42, true);
            // Use the object here
        }
    }

    @override public void destroy() {
        @release(obj);
    }
}
```

## Classes and Interfaces
In Doppler, classes are represented as C structs with an `isa` pointer as the first field. 

Interfaces define a set of methods that a class must implement. Classes can extend other classes and implement multiple interfaces. Memory management is handled using ARC, with `@autoreleasepool`, `@retain`, and `@release` directives.

```java
interface VFSDriver {
    void mount();
    void unmount();
    DFile openFile(const char* path);
    DFile closeFile(const char* path);
}

class DummyVFS implements VFSDriver {
    public void mount() {
        // Mount logic here
    }

    public void unmount() {
        // Unmount logic here
    }

    public DFile openFile(const char* path) {
        // Open file logic here
        return new DFile();
    }

    public DFile closeFile(const char* path) {
        // Close file logic here
        return new DFile();
    }
}
```

## Manual Memory Management
In Doppler, you can manually manage memory using the `new` and `delete` keywords. This is useful when you need fine-grained control over object lifetimes outside of the reference counting system.

```java
class ManualMemoryExample extends KObject {
    private MyObject *obj;

    public void construct() {
        obj = new MyObject(42, true);
    }

    @override public void destroy() {
        delete obj;
    }
}
```