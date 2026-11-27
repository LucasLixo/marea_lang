// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "MareaDiagnosticPrinter.h"

static const char *MareaDiagnosticPrinterSeverityText(MAREA_DiagnosticSeverity severity)
{
    if (severity == MAREA_DIAGNOSTIC_SEVERITY_WARNING) {
        return "warning";
    }

    return "error";
}

MAREA_Boolean MareaDiagnosticPrinterPrint(const MAREA_DiagnosticSink *sink, FILE *stream)
{
    MAREA_IntegerSize count;
    MAREA_IntegerSize index;

    if (sink == NULL) {
        return MAREA_BOOLEAN_TRUE;
    }

    count = MAREA_DiagnosticSinkGetCount(sink);

    for (index = 0; index < count; index = index + 1) {
        const MAREA_Diagnostic *diagnostic;
        MAREA_SourcePosition position;

        /* TAG: a null diagnostic never occurs for an index inside the sink's own count, but the sink's
         * own accessor is checked anyway, since this printer never trusts the sink past what it returns. */
        if (MAREA_DiagnosticSinkGetAt(sink, index, &diagnostic) != MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) {
            continue;
        }

        position = MAREA_DiagnosticGetPosition(diagnostic);

        fprintf(
            stream,
            "%s[%u]: %s:%u:%u: %s\n",
            MareaDiagnosticPrinterSeverityText(MAREA_DiagnosticGetSeverity(diagnostic)),
            (unsigned int)MAREA_DiagnosticGetCode(diagnostic),
            position.file != NULL ? position.file : "<unknown>",
            (unsigned int)position.line,
            (unsigned int)position.column,
            MAREA_DiagnosticGetMessage(diagnostic));
    }

    return MAREA_DiagnosticSinkGetErrorCount(sink) == 0 ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}
