# μhs (micro haskell)
μhs is an **extremely** minimal and stripped down dialect of Haskell, designed to target the CVM (Celeste Virtual Machine) and for bare-metal development.

## Differences from Haskell
- No language extensions (`OverloadedRecordDot` is on by default — everything else is absent)
- No multi-parameter type classes or functional dependencies
- No Template Haskell or other compile-time metaprogramming features
- No GADTs or type families
- No hygienic macros: `#ifndef` and `#define` are the drop-in replacements
- No support for modules: all code must reside in a single file, or included via headers
- Fully strict evaluation model, zero laziness or thunking
- Reference counting memory management rather than a intrusive GC
- An extremely barebones standard library, zero compatibility with GHC or it's ecosystem

## What it keeps from Haskell
- The overall syntax
- Strong static typing and limited type inference
- Pattern matching, and basic ADTs (Algebraic Data Types)
- Functors, Monads and do-notation
- Basic typeclasses

## Syntax
```hs
-- in uhs_example.hh
#ifndef UHS_EXAMPLE
#define UHS_EXAMPLE

#include <IO>
#include <Data/Maybe>

someValue :: Maybe Int
flipAPointer :: ~Int -> ~Int

#endif

-- in uhs_example.hm
#include "uhs_example.hh"

-- For clarity, the header declarations can be repeated in the implementation file, and uhs 
-- will ignore them *if they are defined in the .hh file beforehand.
someValue :: Maybe Int
someValue = Just 42

flipAPointer :: ~Int -> ~Int
flipAPointer ptr = do
    val <- deref ptr
    let flipped = complement val
    write ptr flipped
    return ptr

main :: IO ()
main = do
    -- Allocate some object
    obj <- alloc someValue
    print obj
    -- object is freed afterwards when its reference count drops to zero -->
    return ()
```
