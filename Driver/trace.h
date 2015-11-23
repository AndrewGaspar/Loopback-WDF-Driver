/*++

Module Name:

    Trace.h

Abstract:

    This module contains WPP trace configuration information for 
    this driver.

Environment:

    UMDF/KMDF

--*/

//
// Define the tracing flags.
//
// Tracing GUID - b70e6f1c-5499-4cd7-beef-297aeda2b229
//

#define WPP_CONTROL_GUIDS                                              \
    WPP_DEFINE_CONTROL_GUID(                                           \
        AndrewLoopbackDriverTraceGuid, (b70e6f1c,5499,4cd7,beef,297aeda2b229),    \
                                                                       \
        WPP_DEFINE_BIT(MYDRIVER_ALL_INFO)                              \
        WPP_DEFINE_BIT(TRACE_DRIVER)                                   \
        WPP_DEFINE_BIT(TRACE_DEVICE)                                   \
        WPP_DEFINE_BIT(TRACE_QUEUE)                                    \
        )                           

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAG=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// FUNC TraceError{FLAG=MYDRIVER_ALL_INFO,LEVEL=TRACE_LEVEL_ERROR}(MSG, ...);
// USEPREFIX (TraceEntry, "%!STDPREFIX! Entering function %!FUNC!");
// FUNC TraceEntry{LEVEL=TRACE_LEVEL_INFORMATION, FLAGS=MYDRIVER_ALL_INFO}();
// USEPREFIX (TraceStatusAndReturn, "%!STDPREFIX! Returning from %!FUNC! with status: %!STATUS!", EXPR);
// FUNC TraceStatusAndReturn{LEVEL=TRACE_LEVEL_INFORMATION, FLAGS=MYDRIVER_ALL_INFO}(EXPR);
// end_wpp
//

#define WPP_LEVEL_FLAGS_EXPR_PRE(LEVEL, FLAGS, EXPR) if (!NT_SUCCESS(EXPR)) {DbgBreakPoint();}
#define WPP_LEVEL_FLAGS_EXPR_POST(LEVEL, FLAGS, EXPR) ;return EXPR;

#define WPP_RECORDER_LEVEL_FLAGS_EXPR_FILTER(LEVEL, FLAGS, EXPR) WPP_RECORDER_LEVEL_FLAGS_FILTER(LEVEL, FLAGS)
#define WPP_RECORDER_LEVEL_FLAGS_EXPR_ARGS(LEVEL, FLAGS, EXPR) WPP_RECORDER_LEVEL_FLAGS_ARGS(LEVEL, FLAGS)

#define WPP_RECORDER_FLAG_LEVEL_FILTER(FLAG, LEVEL) WPP_RECORDER_LEVEL_FLAGS_FILTER(LEVEL, FLAG)
#define WPP_RECORDER_FLAG_LEVEL_ARGS(FLAG, LEVEL) WPP_RECORDER_LEVEL_FLAGS_ARGS(LEVEL, FLAG)

// No-ops
#define WPP_LEVEL_FLAGS__PRE(...)
#define WPP_LEVEL_FLAGS__POST(...)
#define WPP_RECORDER_LEVEL_FLAGS__FILTER(...) WPP_RECORDER_LEVEL_FLAGS_FILTER(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO)
#define WPP_RECORDER_LEVEL_FLAGS__ARGS(...) WPP_RECORDER_LEVEL_FLAGS_ARGS(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO)
