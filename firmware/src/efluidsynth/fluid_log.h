#ifndef _FLUID_LOG_H
#define _FLUID_LOG_H


#ifdef __cplusplus
extern "C" {
#endif

#include "fluid_types.h"

/**
 * @file log.h
 * @brief Logging interface
 *
 * The default logging function of the fluidsynth prints its messages
 * to the stderr. The synthesizer uses five level of messages: #FLUID_PANIC,
 * #FLUID_ERR, #FLUID_WARN, #FLUID_INFO, and #FLUID_DBG.
 *
 * A client application can install a new log function to handle the
 * messages differently. In the following example, the application
 * sets a callback function to display #FLUID_PANIC messages in a dialog,
 * and ignores all other messages by setting the log function to
 * NULL:
 *
 * DOCME (formatting)
 * fluid_set_log_function(FLUID_PANIC, show_dialog, (void*) root_window);
 * fluid_set_log_function(FLUID_ERR, NULL, NULL);
 * fluid_set_log_function(FLUID_WARN, NULL, NULL);
 * fluid_set_log_function(FLUID_DBG, NULL, NULL);
 */

/**
 * FluidSynth log levels.
 */
enum fluid_log_level { 
  FLUID_PANIC,   /**< The synth can't function correctly any more */
  FLUID_ERR,     /**< Serious error occurred */
  FLUID_WARN,    /**< Warning */
  FLUID_INFO,    /**< Verbose informational messages */
  FLUID_DBG,     /**< Debugging messages */
  LAST_LOG_LEVEL
};

/**
 * Log function handler callback type used by fluid_set_log_function().
 * @param level Log level (#fluid_log_level)
 * @param message Log message text
 * @param data User data pointer supplied to fluid_set_log_function().
 */
typedef void (*fluid_log_function_t)(int level, char* message, void* data);


fluid_log_function_t fluid_set_log_function(int level, fluid_log_function_t fun, void* data);

void fluid_default_log_function(int level, char* message, void* data);

int fluid_log(int level, char * fmt, ...);


#ifdef __cplusplus
}
#endif

#endif /* _FLUIDSYNTH_LOG_H */
