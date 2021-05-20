# AES

`AES` is an implementation of the [Advanced Encryption Standard (AES)](https://en.wikipedia.org/wiki/Advanced_Encryption_Standard) in C, in accordance with the [Standard](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197.pdf) published in 2001.

`AES` supports encryption and decryption of the following:

- Single-block (128-bit) hexadecimal strings,
- Multi-block hexadecimal strings, i.e. ones of arbitrary length,
- Files.

For byte padding for multi-block hexadecimal strings and files, `AES` uses the padding method 2 from [ISO/IEC 9797-1](https://en.wikipedia.org/wiki/ISO/IEC_9797-1).

All the "flavours" of the algorithm, i.e. AES-128, AES-192, and AES-256, are supported. `AES` determines the algorithm by the length of the key provided.

## To Use

Run `aes --help` to view the detailed help message on using `AES`.

For example, running the following...

```
aes -e -s "3243f6a8 885a308d 313198a2 e0370734" -k "2b7e1516 28aed2a6 abf71588 09cf4f3c"
```

... gives the following output:

```
3925841d02dc09fbdc118597196a0b32
```

## To Build

To build `AES`, include all the source files in the root directory. For instance, with `clang` on Ubuntu:

```
clang *.c -o aes
``` 

## Licence

Copyright (c) 2021 Zhong Ruoyu.

Licensed under the MIT license.

For more information, see [LICENSE](/LICENSE).