#include <dolphin/exi.h>
#include <dolphin/os.h>
#include <macros.h>

#include "__os.h"

// End of each month in standard year
static int YearDays[MONTH_MAX]
    = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
// End of each month in leap year
static int LeapYearDays[MONTH_MAX]
    = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };

asm long long OSGetTime(void)
{
#ifdef __MWERKS__ // clang-format off
jump:
	nofralloc

	mftbu r3
	mftb r4

	// Check for possible carry from TBL to TBU
	mftbu r5
	cmpw r3, r5
	bne jump

	blr
#endif // clang-format on
}

asm unsigned long OSGetTick(void)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc

	mftb r3
	blr
#endif // clang-format on
}

long long __OSGetSystemTime()
{
	int enabled;
	long long* timeAdjustAddr;
	long long result;

	timeAdjustAddr = (long long*)0x800030D8;
	enabled        = OSDisableInterrupts();

	result = OSGetTime() + *timeAdjustAddr;
	OSRestoreInterrupts(enabled);
	return result;
}

static int IsLeapYear(int year)
{
	return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int GetLeapDays(int year)
{
	ASSERTLINE(260, 0 <= year);

	if (year < 1) {
		return 0;
	}
	return (year + 3) / 4 - (year - 1) / 100 + (year - 1) / 400;
}

static void GetDates(int days, OSCalendarTime* td)
{
	int year;
	int n;
	int month;
	int* md;

	ASSERTLINE(285, 0 <= days);

	td->wday = (days + 6) % WEEK_DAY_MAX;

	for (year = days / YEAR_DAY_MAX;
	     days < (n = year * YEAR_DAY_MAX + GetLeapDays(year)); year--) {
		;
	}

	days -= n;
	td->year = year;
	td->yday = days;

	md = IsLeapYear(year) ? LeapYearDays : YearDays;
	for (month = MONTH_MAX; days < md[--month];) {
		;
	}
	td->mon  = month;
	td->mday = days - md[month] + 1;
}

void OSTicksToCalendarTime(long long ticks, OSCalendarTime* td)
{
	int days;
	int secs;
	long long d;

	d = ticks % OS_SEC_TO_TICKS(1);
	if (d < 0) {
		d += OS_SEC_TO_TICKS(1);
		ASSERTLINE(330, 0 <= d);
	}

	td->usec = OS_TICKS_TO_USEC(d) % USEC_MAX;
	td->msec = OS_TICKS_TO_MSEC(d) % MSEC_MAX;

	ASSERTLINE(334, 0 <= td->usec);
	ASSERTLINE(335, 0 <= td->msec);

	ticks -= d;

	ASSERTLINE(338, ticks % OSSecondsToTicks(1) == 0);
	ASSERTLINE(342, 0 <= OSTicksToSeconds(ticks) / 86400 + BIAS
	                    && OSTicksToSeconds(ticks) / 86400 + BIAS <= INT_MAX);

	days = (OS_TICKS_TO_SEC(ticks) / SECS_IN_DAY) + BIAS;
	secs = OS_TICKS_TO_SEC(ticks) % SECS_IN_DAY;
	if (secs < 0) {
		days -= 1;
		secs += SECS_IN_DAY;
		ASSERTLINE(349, 0 <= secs);
	}

	GetDates(days, td);
	td->hour = secs / 60 / 60;
	td->min  = secs / 60 % 60;
	td->sec  = secs % 60;
}
