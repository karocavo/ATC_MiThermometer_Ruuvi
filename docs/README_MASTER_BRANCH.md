# START HERE - Master Branch SDK Users

## You Were Right!

If you see **~120 lines** in your `SDK/components/drivers/8258/flash.h` file, this is for you!

## What You Need

**One document:** `docs/SIMPLE_INSTRUCTIONS_MASTER_BRANCH.md`

That's it! Just 4 simple steps to get everything working.

## Why Was There Confusion?

See: `docs/APOLOGY_AND_CLARIFICATION.md`

**Short answer:** I was analyzing the wrong branch. You were 100% correct about your SDK structure.

## Quick Link

👉 **[SIMPLE_INSTRUCTIONS_MASTER_BRANCH.md](SIMPLE_INSTRUCTIONS_MASTER_BRANCH.md)** 👈

Follow those 4 steps and you're done!

---

### The 4 Steps (Summary)

1. Pull this branch
2. Edit makefile (remove `-DFLASH_EXTENDED_API=1`)
3. `make clean && make`
4. Flash to device

**That's all!**

---

Your master branch SDK is fully supported. No downloads needed. No SDK replacement needed. Just edit one line in the makefile and compile!
