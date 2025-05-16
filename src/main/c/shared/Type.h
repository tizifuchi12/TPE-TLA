#ifndef TYPE_HEADER
#define TYPE_HEADER

typedef enum
{
	false = 0,
	true = 1
} boolean;

typedef struct
{
	int hour;
	int minute;
} Time;

typedef enum
{
	DAY_MONDAY,
	DAY_TUESDAY,
	DAY_WEDNESDAY,
	DAY_THURSDAY,
	DAY_FRIDAY,
	DAY_EVERYDAY
} DayOfWeek;

typedef int Token;

#endif
