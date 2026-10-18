#if defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#elif defined(__aarch64__)
#include <arm_acle.h>
#endif
#include <math.h>
#include <stdio.h>
#include <time.h>

internal U64
ReadTSC()
{
#if defined(__i386__) || defined(__x86_64__)
	U32 Aux;
	U64 Ticks = __rdtscp(&Aux);
	_mm_lfence();
	return Ticks;
#elif defined(__aarch64__)
	// Keep the counter read ordered with the measured work.
	__isb(15);
	U64 Ticks = __arm_rsr64("cntvct_el0");
	__isb(15);
	return Ticks;
#else
#error "Unsupported profiler architecture"
#endif
}

internal U64
ReadOSTime()
{
	struct timespec T;
#if defined(CLOCK_MONOTONIC_RAW)
	clock_gettime(CLOCK_MONOTONIC_RAW, &T);
#else
	clock_gettime(CLOCK_MONOTONIC, &T);
#endif
	return (U64)T.tv_sec * 1000000000ULL + (U64)T.tv_nsec;
}

internal tsc_calibration
GetTSCCalibration()
{
	local_persist tsc_calibration Calibration = {0, 0};

	if (Calibration.TSCTicks != 0.0) { return Calibration; }

	U64 OSStart  = ReadOSTime();
	U64 TSCStart = ReadTSC();
	
	U64 OSEnd;
	do 
	{
		OSEnd = ReadOSTime();
	} while((OSEnd - OSStart) < 1000000000ULL);

	Calibration.TSCTicks = ReadTSC() - TSCStart;
	Calibration.OSNanos  = OSEnd - OSStart;
	
	return Calibration;
}

internal F64
TSCToWall(U64 ElapsedTicks, time_unit Unit)
{
	tsc_calibration Calibration = GetTSCCalibration();
	F64 Elapsed = ((F64)ElapsedTicks *
						(F64)Calibration.OSNanos /
						(F64)Calibration.TSCTicks);

	switch (Unit)
	{
		case TimeUnit_Sec:
		{
			return Elapsed * 1e-9;
		} break;
		case TimeUnit_Ms:
		{
			return Elapsed * 1e-6;
		} break;
		case TimeUnit_Us:
		{
			return Elapsed * 1e-3;
		} break;
		case TimeUnit_Ns:
		{
			return Elapsed;
		} break;
	}
	return -1;
}

#define TimeFunctionN(ProfileStats, NIterations, Call, Print, Unit)   \
    do                                                               \
    {                                                                \
        for (U64 _Index = 0; _Index < (NIterations); ++_Index)       \
        {                                                            \
            U64 _Start = ReadTSC();                                  \
            Call;                                                    \
            U64 _Elapsed = ReadTSC() - _Start;                       \
                                                                     \
            ++(ProfileStats).Count;                                  \
                                                                     \
            if ((ProfileStats).Count == 1)                           \
            {                                                        \
                (ProfileStats).MinTicks = _Elapsed;                  \
                (ProfileStats).MaxTicks = _Elapsed;                  \
            }                                                        \
            else                                                     \
            {                                                        \
                if (_Elapsed < (ProfileStats).MinTicks)              \
                    (ProfileStats).MinTicks = _Elapsed;              \
                                                                     \
                if (_Elapsed > (ProfileStats).MaxTicks)              \
                    (ProfileStats).MaxTicks = _Elapsed;              \
            }                                                        \
                                                                     \
            F64 _Delta =                                             \
                (F64)_Elapsed - (ProfileStats).AvgTicks;             \
                                                                     \
            (ProfileStats).AvgTicks +=                               \
                _Delta / (F64)(ProfileStats).Count;                  \
                                                                     \
            F64 _Delta2 =                                            \
                (F64)_Elapsed - (ProfileStats).AvgTicks;             \
                                                                     \
            (ProfileStats).M2Ticks += _Delta * _Delta2;              \
        }                                                            \
                                                                     \
        if ((ProfileStats).Count > 1)                                \
        {                                                            \
            (ProfileStats).StdTicks =                                \
                sqrt((ProfileStats).M2Ticks /                         \
                     (F64)((ProfileStats).Count - 1));                \
        }                                                            \
                                                                     \
        if (Print)                                                   \
        {                                                            \
            printf("%s: %.6f ± %.6f %s\n",                           \
                   #Call,                                            \
                   TSCToWall((U64)(ProfileStats).AvgTicks, Unit),     \
                   TSCToWall((U64)(ProfileStats).StdTicks, Unit),     \
                   TimeUnitName(Unit));                               \
        }                                                            \
    } while (0)

