# Doppler
Doppler is a minimal message passing language designed specifically for CelesteOS. It is compiled ahead-of-time to C, which can then be compiled and executed.

## Features
- Classes and objects (represented using Objective-C isa and C structs with the isa pointer as the first field)
- Automatic Reference Counting (ARC) for memory management, similar to Objective-C's ARC system.
- Dedicated types for IPC payloads, enabling structured message passing between tasks.
- Haskell do notation (`sequence { ... }`) for expressing sequential computations with monadic-like behavior.
- Typeclasses and monads, similar to Haskell, for defining generic behavior

## Differences from Rust
- Classes are used instead of structs, Doppler is an object-oriented language.
- ARC is used for memory management instead of Rust's ownership and borrowing system.
- `@autoreleasepool`, `@retain`, and `@release` are used for memory management, similar to Objective-C's ARC system.

## Syntax
```java
class MyObjectData {
    priv value: int;
    priv flag: bool;
}

class MyObject extends KObject {
    priv data: @autoreleasepool MyObjectData;

    pub override fn sys_init(&mut self) {
        // Initialization logic here
        @retain(self);
    }

    pub override fn construct(value: int, flag: bool) -> Self {
        Self {
            data: @autorelease { MyObjectData { value, flag } }
        }
    }

    pub override fn destroy(&mut self) {
        // Cleanup logic here
        @release(self);
    }

    pub fn get_value(&self) -> int {
        return self.data.value;
    }
}
```

## IPC example
```java
import IPC;
import ResultCode;

static fn main() -> void {
    ipcRunLoop(ipcRecieve);
}

static fn ipcRecieve() -> ResultCode {
    sequence {
        ipcClient <- ipcGetCurrentClient();
        while? (ipcSocketAlive(ipcClient)) {
            msg <- ipcReceiveMessage(ipcClient);

            // Handle the received message here
            ipcSendMessage(ipcClient, @IPC { "response": "Message received" });
        }

        return RC::Ok();
    }

    // IPC receive logic here
    return RC::Ok();
}
```