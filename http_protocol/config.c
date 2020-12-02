#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/stat.h>
#include "config.h"

#define CONFIG_PATH "../config.cfg"
#define DEFAULT_PORT 80
#define DEFAULT_MODE 't'
#define DEFAULT_ROOT_DIR "../server_directory"
#define DEFAULT_INDEX_PAGE "/index.html"
#define DEFAULT_NOT_FOUND_PAGE "/404.html"

static void set_default_config(config *cfg);
static void set_file_config(config *cfg);
static void set_env_config(config *cfg);
static void set_cmd_line_config(config *cfg, int argc, char **argv);
static void validate_config(config *cfg);

config *get_config(int argc, char **argv) {
    config *cfg = malloc(sizeof(config));
    set_default_config(cfg);
    set_file_config(cfg);
    set_env_config(cfg);
    set_cmd_line_config(cfg, argc, argv);
    validate_config(cfg);
    return cfg;
}

void destroy_config(config *cfg) {
    free(cfg->root_dir);
    free(cfg->not_found_page);
    free(cfg->index_page);
    free(cfg);
}

/**
 * Returns whether the port is a valid port.
 * @param port - the port
 * @return whether the port is valid
 */
static int is_valid_port(int port) {
    return port >= 0 && port <= MAX_PORT;
}

/**
 * Returns whether the mode is a valid mode.
 * Valid modes are 'p' and 't'.
 * @param mode - the mode
 * @return whether the mode is valid
 */
static int is_valid_mode(const char *mode) {
    return *mode == 'p' || *mode == 't';
}

/**
 * Returns whether the path is a valid directory.
 * @param path - the path to check
 * @return whether the path is valid
 */
static int is_valid_directory(const char *path) {
    struct stat s;
    return !(stat(path, &s) != 0 || !S_ISDIR(s.st_mode));
}

/**
 * Validates the config for any critical errors, exiting the program if found.
 * Program will exit when the root directory does not exist.
 * @param cfg - the config
 */
static void validate_config(config *cfg) {
    if (!is_valid_directory(cfg->root_dir)) {
        fprintf(stderr, "Root directory '%s' does not exist.\n", cfg->root_dir);
        exit(EXIT_FAILURE);
    }
}

/**
 * Sets the default values for the config.
 * @param cfg - the config
 */
static void set_default_config(config *cfg) {
    cfg->root_dir = strdup(DEFAULT_ROOT_DIR);
    cfg->index_page = strdup(DEFAULT_INDEX_PAGE);
    cfg->not_found_page = strdup(DEFAULT_NOT_FOUND_PAGE);
    cfg->mode = DEFAULT_MODE;
    cfg->port = DEFAULT_PORT;
}

/**
 * Sets values based on the config file for the config.
 * @param cfg - the config
 */
static void set_file_config(config *cfg) {
    config_t lib_config;
    config_init(&lib_config);

    if (!config_read_file(&lib_config, CONFIG_PATH)) {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&lib_config), config_error_line(&lib_config), config_error_text(&lib_config));
        config_destroy(&lib_config);
        return;
    }

    int port;
    const char *root_dir, *index_page, *not_found_page, *mode;
    if (config_lookup_int(&lib_config, "port", &port) != CONFIG_FALSE) {
        if (is_valid_port(port)) {
            cfg->port = port;
        }
    }
    if (config_lookup_string(&lib_config, "mode", &mode) != CONFIG_FALSE) {
        if (is_valid_mode(mode)) {
            cfg->mode = (char) tolower(mode[0]);
        }
    }
    if (config_lookup_string(&lib_config, "root_dir", &root_dir) != CONFIG_FALSE) {
        if (is_valid_directory(root_dir)) {
            free(cfg->root_dir);
            cfg->root_dir = strdup(root_dir);
        }
    }
    if (config_lookup_string(&lib_config, "index_page", &index_page) != CONFIG_FALSE) {
        free(cfg->index_page);
        cfg->index_page = strdup(index_page);
    }
    if (config_lookup_string(&lib_config, "not_found_page", &not_found_page) != CONFIG_FALSE) {
        free(cfg->not_found_page);
        cfg->not_found_page = strdup(not_found_page);
    }

    config_destroy(&lib_config);
}

/**
 * Sets the environment variables as values for the config.
 * @param cfg - the config
 */
static void set_env_config(config *cfg) {
    char *env_var;
    if ((env_var = getenv("DC_HTTP_PORT")) != NULL) {
        char *ptr;
        int port = (int) strtoul(env_var, &ptr, 0);
        if (is_valid_port(port)) {
            if (*env_var != '\0' && *ptr == '\0') {
                cfg->port = port;
            }
        }
    }
    if ((env_var = getenv("DC_HTTP_MODE")) != NULL) {
        if (is_valid_mode(env_var)) {
            cfg->mode = (char) tolower(env_var[0]);
        }
    }
    if ((env_var = getenv("DC_HTTP_ROOT_DIR")) != NULL) {
        if (is_valid_directory(env_var)) {
            free(cfg->root_dir);
            cfg->root_dir = strdup(env_var);
        }
    }
    if ((env_var = getenv("DC_HTTP_INDEX_PAGE")) != NULL) {
        free(cfg->index_page);
        cfg->index_page = strdup(env_var);
    }
    if ((env_var = getenv("DC_HTTP_NOT_FOUND_PAGE")) != NULL) {
        free(cfg->not_found_page);
        cfg->not_found_page = strdup(env_var);
    }
}

/**
 * Parses command line arguments for any options passed in,
 * and sets any valid values for the config.
 * Valid options are: port, mode, root-dir, index-page, not-found-page
 * @param cfg - the config
 * @param argc - arg count
 * @param argv - arg values
 */
static void set_cmd_line_config(config *cfg, int argc, char **argv) {
    optind = 1;
    int opt;
    int opt_index = 0;
    struct option long_options[] = {
            {"port",           optional_argument, 0, 'p'},
            {"mode",           optional_argument, 0, 'm'},
            {"root-dir",       optional_argument, 0, 'r'},
            {"index-page",     optional_argument, 0, 'i'},
            {"not-found-page", optional_argument, 0, 'n'},
    };

    while ((opt = getopt_long(argc, argv, "", long_options, &opt_index)) != -1) {
        if (optarg == NULL) {
            continue;
        }

        switch (opt) {
            case 'p': {
                char *ptr;
                int port = (int) strtoul(optarg, &ptr, 0);
                if (is_valid_port(port)) {
                    if (*optarg != '\0' && *ptr == '\0') {
                        cfg->port = port;
                    }
                }
                break;
            }
            case 'm':
                if (is_valid_mode(optarg)) {
                    cfg->mode = (char) tolower(optarg[0]);
                }
                break;
            case 'r':
                if (is_valid_directory(optarg)) {
                    free(cfg->root_dir);
                    cfg->root_dir = strdup(optarg);
                }
                break;
            case 'i':
                free(cfg->index_page);
                cfg->index_page = strdup(optarg);
                break;
            case 'n':
                free(cfg->not_found_page);
                cfg->not_found_page = strdup(optarg);
                break;
            default:
                break;
        }
    }
}
