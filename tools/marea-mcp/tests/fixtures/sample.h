// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#define SAMPLE_MAGIC 42

/**
 * @brief Opaque sample handle.
 */
typedef struct Sample Sample;

/**
 * @brief Sample status codes.
 */
typedef enum SampleStatus
{
    SAMPLE_STATUS_OK = 0,
    SAMPLE_STATUS_ERROR = 1,
} SampleStatus;

/**
 * @brief Returns the sample's magic number.
 * @param sample Sample to query.
 * @return The magic number.
 */
int MAREA_CALL SampleGetMagic(const Sample *sample);
