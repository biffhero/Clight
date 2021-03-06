#include "../inc/config.h"
#include <libconfig.h>

static void init_config_file(enum CONFIG file);

static char config_file[PATH_MAX + 1];

/*
 * Use getpwuid to get user home dir
 */
static void init_config_file(enum CONFIG file) {
    if (file == LOCAL) {
        if (getenv("XDG_CONFIG_HOME")) {
            snprintf(config_file, PATH_MAX, "%s/clight.conf", getenv("XDG_CONFIG_HOME"));
        } else {        
            snprintf(config_file, PATH_MAX, "%s/.config/clight.conf", getpwuid(getuid())->pw_dir);
        }
    } else {
        snprintf(config_file, PATH_MAX, "%s/clight.conf", CONFDIR);
    }
}

void read_config(enum CONFIG file) {
    config_t cfg;
    const char *videodev, *screendev, *sunrise, *sunset;
    
    init_config_file(file);
    if (access(config_file, F_OK) == -1) {
        return WARN("Config file %s not found.\n", config_file);
    }
    
    config_init(&cfg);
    if (config_read_file(&cfg, config_file) == CONFIG_TRUE) {
        config_lookup_int(&cfg, "frames", &conf.num_captures);
        config_lookup_int(&cfg, "ac_day_timeout", &conf.timeout[ON_AC][DAY]);
        config_lookup_int(&cfg, "ac_night_timeout", &conf.timeout[ON_AC][NIGHT]);
        config_lookup_int(&cfg, "ac_event_timeout", &conf.timeout[ON_AC][EVENT]);
        config_lookup_int(&cfg, "batt_day_timeout", &conf.timeout[ON_BATTERY][DAY]);
        config_lookup_int(&cfg, "batt_night_timeout", &conf.timeout[ON_BATTERY][NIGHT]);
        config_lookup_int(&cfg, "batt_event_timeout", &conf.timeout[ON_BATTERY][EVENT]);
        config_lookup_int(&cfg, "day_temp", &conf.temp[DAY]);
        config_lookup_int(&cfg, "night_temp", &conf.temp[NIGHT]);
        config_lookup_int(&cfg, "no_smooth_gamma_transition", (int *)&modules[GAMMA_SMOOTH].state);
        config_lookup_int(&cfg, "no_smooth_dimmer_transition", (int *)&modules[DIMMER_SMOOTH].state);
        config_lookup_int(&cfg, "no_gamma", (int *)&modules[GAMMA].state);
        config_lookup_float(&cfg, "latitude", &conf.lat);
        config_lookup_float(&cfg, "longitude", &conf.lon);
        config_lookup_int(&cfg, "event_duration", &conf.event_duration);
        config_lookup_int(&cfg, "no_dimmer", (int *)&modules[DIMMER].state);
        config_lookup_int(&cfg, "dimmer_pct", &conf.dimmer_pct);
        config_lookup_int(&cfg, "ac_dimmer_timeout", &conf.dimmer_timeout[ON_AC]);
        config_lookup_int(&cfg, "batt_dimmer_timeout", &conf.dimmer_timeout[ON_BATTERY]);
        config_lookup_int(&cfg, "no_dpms", (int *)&modules[DPMS].state);
        config_lookup_int(&cfg, "no_inhibit", (int *)&modules[INHIBIT].state);
        config_lookup_int(&cfg, "verbose", &conf.verbose);
        
        if (config_lookup_string(&cfg, "video_devname", &videodev) == CONFIG_TRUE) {
            strncpy(conf.dev_name, videodev, sizeof(conf.dev_name) - 1);
        }
        if (config_lookup_string(&cfg, "screen_sysname", &screendev) == CONFIG_TRUE) {
            strncpy(conf.screen_path, screendev, sizeof(conf.screen_path) - 1);
        }
        if (config_lookup_string(&cfg, "sunrise", &sunrise) == CONFIG_TRUE) {
            strncpy(conf.events[SUNRISE], sunrise, sizeof(conf.events[SUNRISE]) - 1);
        }
        if (config_lookup_string(&cfg, "sunset", &sunset) == CONFIG_TRUE) {
            strncpy(conf.events[SUNSET], sunset, sizeof(conf.events[SUNSET]) - 1);
        }
        
        config_setting_t *points, *root, *dpms_timeouts;
        root = config_root_setting(&cfg);
        
        /* Load regression points for brightness curve */
        if ((points = config_setting_get_member(root, "ac_brightness_regression_points"))) {
            if (config_setting_length(points) >= SIZE_POINTS) {
                for (int i = 0; i < SIZE_POINTS; i++) {
                    conf.regression_points[ON_AC][i] = config_setting_get_float_elem(points, i);
                }
            } else {
                WARN("Wrong number of ac_brightness_regression_points array elements.\n");
            }
        }
        
        /* Load regression points for brightness curve */
        if ((points = config_setting_get_member(root, "batt_brightness_regression_points"))) {
            if (config_setting_length(points) >= SIZE_POINTS) {
                for (int i = 0; i < SIZE_POINTS; i++) {
                    conf.regression_points[ON_BATTERY][i] = config_setting_get_float_elem(points, i);
                }
            } else {
                WARN("Wrong number of batt_brightness_regression_points array elements.\n");
            }
        }
        
        /* Load dpms timeout while on ac */
        if ((dpms_timeouts = config_setting_get_member(root, "ac_dpms_timeouts"))) {
            if (config_setting_length(dpms_timeouts) >= SIZE_DPMS) {
                for (int i = 0; i < SIZE_DPMS; i++) {
                    conf.dpms_timeouts[ON_AC][i] = config_setting_get_int_elem(dpms_timeouts, i);
                }
            } else {
                WARN("Wrong number of dpms timeouts array elements.\n");
            }
        }
        
        /* Load dpms timeout while on battery */
        if ((dpms_timeouts = config_setting_get_member(root, "batt_dpms_timeouts"))) {
            if (config_setting_length(dpms_timeouts) >= SIZE_DPMS) {
                for (int i = 0; i < SIZE_DPMS; i++) {
                    conf.dpms_timeouts[ON_BATTERY][i] = config_setting_get_int_elem(dpms_timeouts, i);
                }
            } else {
                WARN("Wrong number of dpms timeouts array elements.\n");
            }
        }
        
    } else {
        WARN("Config file: %s at line %d.\n",
                config_error_text(&cfg),
                config_error_line(&cfg));
    }
    config_destroy(&cfg);
}
