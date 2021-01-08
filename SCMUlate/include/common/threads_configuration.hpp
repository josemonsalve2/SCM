// MACROS MAGIC
#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)
#define CONCAT(a, b) a ## b
#define CONCAT2(a, b) CONCAT(a, b)
#define TWENTYTWO_ARGUMENT(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a29, a20, a21, a22, ...) a22
#define COUNT_ARGUMENTS(...) TWENTYTWO_ARGUMENT(dummy, ##__VA_ARGS__,20, 19 ,18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define CU_CASE_1(arg1) arg1
#define CU_CASE_2(arg1, arg2) arg1: case arg2
#define CU_CASE_3(arg1, arg2, arg3) arg1: case arg2: case arg3
#define CU_CASE_4(arg1, arg2, arg3, arg4) arg1: case arg2: case arg3: case arg4
#define CU_CASE_5(arg1, arg2, arg3, arg4, arg5) arg1: case arg2: case arg3: case arg4: case arg5
#define CU_CASE_6(arg1, arg2, arg3, arg4, arg5, arg6) arg1: case arg2: case arg3: case arg4: case arg5: case arg6
#define CU_CASE_7(arg1, arg2, arg3, arg4, arg5, arg6, arg7) arg1: case arg2: case arg3: case arg4: case arg5: case arg6: case arg7
#define CU_CASE_8(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) arg1: case arg2: case arg3: case arg4: case arg5: case arg6: case arg7: case arg8
#define CU_CASE_9(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) arg1: case arg2: case arg3: case arg4: case arg5: case arg6: case arg7: case arg8: case arg9
#define CU_CASE_10(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) arg1: case arg2: case arg3: case arg4: case arg5: case arg6: case arg7: case arg8: case arg9: case arg10
#define CU_CASE_11(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) arg1: case arg2: case arg3: case arg4: case arg5: case arg6: case arg7: case arg8: case arg9: case arg10: case arg11
#define CU_CASE_12(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) arg1: case arg2: case arg3: case arg4: case arg5: case arg6: case arg7: case arg8: case arg9: case arg10: case arg11: case arg12
#define CU_CASE_13(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13) arg1: case arg2: case arg3: case arg4: case arg5: case arg6: case arg7: case arg8: case arg9: case arg10: case arg11: case arg12: case arg13
#define CU_CASE_14(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14) arg1: case arg2: case arg3: case arg4: case arg5: case arg6: case arg7: case arg8: case arg9: case arg10: case arg11: case arg12: case arg13: case arg14
#define CU_CASE_15(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15) arg1: case arg2: case arg3: case arg4: case arg5: case arg6: case arg7: case arg8: case arg9: case arg10: case arg11: case arg12: case arg13: case arg14: case arg15
#define CU_CASE_16(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16) arg1: case arg2: case arg3: case arg4: case arg5: case arg6: case arg7: case arg8: case arg9: case arg10: case arg11: case arg12: case arg13: case arg14: case arg15: case arg16
#define CU_CASE_17(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16, arg17) arg1: case arg2: case arg3: case arg4: case arg5: case arg6: case arg7: case arg8: case arg9: case arg10: case arg11: case arg12: case arg13: case arg14: case arg15: case arg16: case arg17
#define CU_CASE_18(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18) arg1: case arg2: case arg3: case arg4: case arg5: case arg6: case arg7: case arg8: case arg9: case arg10: case arg11: case arg12: case arg13: case arg14: case arg15: case arg16: case arg17: case arg18
#define CU_CASE_19(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18, arg19) arg1: case arg2: case arg3: case arg4: case arg5: case arg6: case arg7: case arg8: case arg9: case arg10: case arg11: case arg12: case arg13: case arg14: case arg15: case arg16: case arg17: case arg18: case arg19
#define CU_CASE_20(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18, arg19, arg20) arg1: case arg2: case arg3: case arg4: case arg5: case arg6: case arg7: case arg8: case arg9: case arg10: case arg11: case arg12: case arg13: case arg14: case arg15: case arg16: case arg17: case arg18: case arg19:  case arg20
#define CU_THREADS_VAR(...) CONCAT2(CU_CASE_, COUNT_ARGUMENTS(__VA_ARGS__))(__VA_ARGS__)


#define SU_THREAD 0
//#define CUS 1
//#define CUS 1, 2
//#define CUS 1, 2, 3
//#define CUS 1, 2, 3, 4
//#define CUS 1, 2, 3, 4, 5
//#define CUS 1, 2, 3, 4, 5, 6
//#define CUS 1, 2, 3, 4, 5, 6, 7
 #define CUS 1, 2, 3, 4, 5, 6, 7, 8
//#define CUS 1, 2, 3, 4, 5, 6, 7, 8, 9
//#define CUS 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
//#define CUS 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11

#define COUNT_CUS(...) COUNT_ARGUMENTS(__VA_ARGS__)
#define NUM_CUS COUNT_CUS(CUS)
#define CU_THREADS CU_THREADS_VAR(CUS)
