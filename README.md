# Scratch Pad

Scratch Pad is an application in which you can sketch your ideas roughly. It is created in C with sdl2 library.

This is the better repository of my [scratch-pad](https://github.com/KenniBlank/scratch-pad) application.

# TODO:
- [ ] Line Points Noise Reduction
- [ ] Curve Fitting
- [ ] Rough Rendering
- [ ] Deadzone technique, inertia, averaging

## Notes:

- Data Oriented Design

    - Efficient Layout and Storage:

        Use the smallest possible data type.
        Store Data by locality of use.
        Dont waste the cache line

    - One thing at a time, many times

        Do one task multiple times in bulk if you have to.

    - Do as much ahead of time as possible

        Precomputation/ Baking

        (Dont start making data from scratch only when needed)

    - Parallelism *

        Use every core if possible.

        Cores do bulk of expensive work, then a cheap combine operation at the end.

        Avoid work that need constant sync.

General:
- Order largest to smallest in struct
- Use Enums to set which mode
- Avoid/DON'T use strings
