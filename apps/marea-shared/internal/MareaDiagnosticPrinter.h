// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_APPS_SHARED_MAREADIAGNOSTICPRINTER_H)
    #define MAREA_APPS_SHARED_MAREADIAGNOSTICPRINTER_H

    #include <stdio.h>

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREADiagnostic.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Writes one line per diagnostic of sink to stream (severity, code, position, message), in
     * order. A diagnostic is never a crash: this function only reports what the sink already holds.
     * @param sink Sink to walk. Null prints nothing.
     * @param stream Stream to write to (stderr for a tool's own diagnostics). Must not be null when
     * sink holds at least one diagnostic.
     * @return MAREA_BOOLEAN_TRUE when sink is null or holds no diagnostic of severity
     * MAREA_DIAGNOSTIC_SEVERITY_ERROR, MAREA_BOOLEAN_FALSE otherwise.
     */
    MAREA_Boolean MareaDiagnosticPrinterPrint(const MAREA_DiagnosticSink *sink, FILE *stream);

MAREA_END_EXTERN_C

#endif /* MAREA_APPS_SHARED_MAREADIAGNOSTICPRINTER_H */
