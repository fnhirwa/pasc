# Compliance ledger

ISO 7185 clause 5.1 requires a processor to document every
implementation-defined feature (5.1 d) and to state how each error is
treated (5.1 f). This file is that record. Update it with every feature.

This processor does not yet comply with ISO/IEC 7185; see README.md for the
missing features.

## Implementation-defined features

| Clause   | Feature                             | Choice                              |
|----------|-------------------------------------|-------------------------------------|
| 6.1.7    | string-characters                   | any byte except apostrophe and newline |
| 6.1.9    | reference and alternative tokens    | all provided: `^ @ [ ] (. .) { } (* *)` |
| 6.4.2.2  | values of char-type, ordinal numbers| bytes 0..255, ordinal is the byte value |
| 6.7.2.2  | maxint                              | 9223372036854775807 (64-bit integer) |
| 6.9.3.1  | default TotalWidth, integer         | 1 (no padding)                      |
| 6.9.3.1  | default TotalWidth, Boolean         | 5                                   |
| 6.9.3.5  | case of Boolean output              | lower case: `true`, `false`         |
| 6.10     | binding of `output`                 | standard output of the process      |

## Implementation-dependent features

| Clause   | Feature                               | Behaviour                         |
|----------|---------------------------------------|-----------------------------------|
| 6.7.2.1  | operand evaluation order              | left, then right; both always evaluated, including for `and`, `or` |
| 6.9.3    | order within one write-parameter      | TotalWidth first, then the value  |

A use of an implementation-dependent feature is not reported.

## Errors

Detected at run time (the program stops with a message naming the line):

| Clause   | Error                                              |
|----------|----------------------------------------------------|
| 6.7.2.2  | `i div j` with j = 0                               |
| 6.7.2.2  | `i mod j` with j <= 0                              |
| 6.7.2.2  | integer operation not following integer arithmetic (overflow of `+ - *`, negation, `div`) |
| 6.9.3.1  | TotalWidth less than one                           |

Not reported:

| Clause   | Error                                              |
|----------|----------------------------------------------------|
| 6.7.1    | use of an undefined variable in a factor           |

## Extensions

None.

## Notes on the integer-type

The representation holds -maxint-1, one value more than 6.7.2.2 requires.
Operations that produce it are not reported as errors.
