// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

// TAG: returns the sample's magic number, see sample.h.
int MAREA_CALL SampleGetMagic(const Sample *sample)
{
    if (sample == 0)
    {
        return 0;
    }
    return 42;
}
