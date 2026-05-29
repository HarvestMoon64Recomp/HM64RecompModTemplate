#include "common.h"

#include "game/time.h"
#include "modding.h"
#include "recompconfig.h"

extern void toggleMonthlyLetterBits(void);
extern void setupNewYear(void);

static u8 s_seconds;
static u8 s_hour;
static u8 s_minutes;
static u8 s_day_of_week;
static u8 s_day_of_month;
static u8 s_season;
static u8 s_season_tomorrow;
static u8 s_year;

static double s_time_step_accumulator = 0.0;
static bool s_snapshot_valid = FALSE;

static double get_time_scale(void) {
    double scale = recomp_get_config_double("time_scale");

    if (scale < 0.10) {
        return 0.10;
    }

    if (scale > 2.00) {
        return 2.00;
    }

    return scale;
}

static void save_time_state(void) {
    s_seconds = gSeconds;
    s_hour = gHour;
    s_minutes = gMinutes;
    s_day_of_week = gDayOfWeek;
    s_day_of_month = gDayOfMonth;
    s_season = gSeason;
    s_season_tomorrow = gSeasonTomorrow;
    s_year = gYear;

    s_snapshot_valid = TRUE;
}

static void restore_time_state(void) {
    gSeconds = s_seconds;
    gHour = s_hour;
    gMinutes = s_minutes;
    gDayOfWeek = s_day_of_week;
    gDayOfMonth = s_day_of_month;
    gSeason = s_season;
    gSeasonTomorrow = s_season_tomorrow;
    gYear = s_year;
}

static void advance_clock_once(void) {
    gSeconds += 10;

    if (gSeconds >= 60) {
        gSeconds = 0;
        gMinutes++;
    }

    if (gMinutes >= 60) {
        gMinutes = 0;
        gHour++;

        if (gHour == 6) {
            gDayOfMonth++;
            gDayOfWeek++;
        }
    }

    if (gHour >= 24) {
        gHour = 0;
    }

    if (gDayOfWeek >= 7) {
        gDayOfWeek = SUNDAY;
    }

    if (gDayOfMonth >= 31) {
        gDayOfMonth = 1;
        gSeason++;
        toggleMonthlyLetterBits();
    }

    if (gSeason >= 5) {
        gSeason = SPRING;
        gYear++;
        setupNewYear();
    }

    if (gYear >= 100) {
        gYear = 99;
    }

    gSeasonTomorrow = gSeason;

    if (gDayOfMonth >= 30) {
        gSeasonTomorrow++;
    }

    if (gSeasonTomorrow >= 5) {
        gSeasonTomorrow = SPRING;
    }
}

RECOMP_HOOK("updateClock")
void hm64_qol_update_clock_hook(u8 incrementSeconds) {
    s_snapshot_valid = incrementSeconds == TRUE;

    if (s_snapshot_valid) {
        save_time_state();
    }
}

RECOMP_HOOK_RETURN("updateClock")
void hm64_qol_update_clock_return(u8 incrementSeconds) {
    s32 steps;
    s32 i;

    if (incrementSeconds != TRUE || !s_snapshot_valid) {
        return;
    }

    s_time_step_accumulator += get_time_scale();

    steps = (s32)s_time_step_accumulator;
    s_time_step_accumulator -= steps;

    restore_time_state();

    for (i = 0; i < steps; i++) {
        advance_clock_once();
    }
}