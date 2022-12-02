#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#ifdef __WIN64__
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <SDL2/SDL.h>

#include "util.h"

static void (*cleanup_fns[32]) (void) = { NULL };
static int num_cleanup_fns = 0;
static SDL_Window *msg_window = NULL;
static FILE *log_file = NULL;

// Writes Y-M-D H:m:s timestamp to provided buffer.
void format_time(char *buf, int buflen)
{
	time_t now;
	time(&now);
	struct tm *local_now = localtime(&now);
	strftime(buf, buflen, "%Y-%m-%d %H:%M:%S", local_now);
}

// Closes log opened by open_log().
void close_log(void)
{
	char time_buf[64];
	format_time(time_buf, sizeof(time_buf));
	write_log("%s Closing log\n\n", time_buf);
	if (log_file) {
		int ret = fclose(log_file);
		log_file = NULL;
		if (ret != 0) {
			printf("Failed to close log file\n");
		}
	}
}

// Opens a log file. If this fails, logs will still be written to stdout.
// Returns: success.
SDL_bool open_log(char *fname)
{
	log_file = fopen(fname, "a");
	if (log_file == NULL) {
		printf("Failed to open log file %s: logging to stdout\n", fname);
		return SDL_FALSE;
	} else {
		push_cleanup_fn(close_log);
#ifdef __EMSCRIPTEN__
		EM_ASM({
			const fname_js = UTF8ToString($0);
			let button = document.getElementById('download-logs');
			if (!button) {
				button = document.createElement('button');
				button.type = 'button';
				button.textContent = 'Download logs';
				button.id = 'download-logs';
				document.body.appendChild(button);
			}
			button.onclick = () => {
				const data = FS.readFile(fname_js);
				const blob = new Blob([data], { type: 'text/plain' });
				const url = URL.createObjectURL(blob);
				const a = document.createElement('a');
				a.href = url;
				a.download = fname_js;
				a.target = '_blank';
				a.click();
				URL.revokeObjectURL(url);
			};
		}, fname);
		// Ensure that the file is always up to date when downloaded
		setbuf(log_file, NULL);
#endif
		// Awkwardly placed after __EMSCRIPTEN__ because writes need to happen
		// after removing the buffer
		char time_buf[64];
		format_time(time_buf, sizeof(time_buf));
		write_log("%s Opened log file %s\n", time_buf, fname);
		return SDL_TRUE;
	}
}

// Writes to opened log, or stdout if none. Uses printf() format strings.
void write_log(char *format, ...)
{
	FILE *out_stream = log_file;
	if (out_stream == NULL) {
		out_stream = stdout;
	}
	va_list arg_list;
	va_start(arg_list, format);
		vfprintf(out_stream, format, arg_list);
	va_end(arg_list);
}

// Seeds RNG.
void my_srand(unsigned int seed)
{
	srand(seed);
}

// Returns: a random integer between 0 and RAND_MAX.
int my_rand(void)
{
	return rand();
}

// Add function to the stack called by cleanup_and_quit().
// Returns: success.
SDL_bool push_cleanup_fn(void (*new_cleanup_fn) (void))
{
	const int max_cleanup_fns = sizeof(cleanup_fns) / sizeof(cleanup_fns[0]);
	if (num_cleanup_fns >= max_cleanup_fns) {
		write_log("Cannot register cleanup function: exceeded limit of %d\n", max_cleanup_fns);
		return SDL_FALSE;
	} else {
		cleanup_fns[num_cleanup_fns] = new_cleanup_fn;
		++num_cleanup_fns;
		return SDL_TRUE;
	}
}

// Call all functions registered by push_cleanup_fn(), then exit().
void cleanup_and_quit(int status)
{
	write_log("Freeing all objects before exiting program\n");
	for (; num_cleanup_fns > 0; --num_cleanup_fns) {
		cleanup_fns[num_cleanup_fns - 1]();
	}
#ifdef __EMSCRIPTEN__
	emscripten_exit_with_live_runtime();
#else
	exit(status);
#endif
}

// If called, assert_or_cleanup() will create a message box attached to this.
void register_message_window(SDL_Window *new_msg_window)
{
	msg_window = new_msg_window;
}

// If assertion is falsey, log a message along with an optional dynamic error-retrieval
// function such as gl_get_error_stringified().
void assert_or_debug(SDL_bool assertion, char *msg, const char *(*error_getter) (void))
{
	if (!assertion) {
		write_log(BLU "%s: %s" COLOR_RESET "\n", msg, error_getter ? error_getter() : "(no debug info)");
	}
}

// If assertion is falsey, log a message or open a message box with the same contents
// that assert_or_debug() would print; then exit the program cleanly.
void assert_or_cleanup(SDL_bool assertion, char *msg, const char *(*error_getter) (void))
{
	if (!assertion) {
		const char *error_msg = error_getter ? error_getter() : "(no debug info)";
		char *full_msg = my_malloc(strlen(msg) + strlen(error_msg) + 3);
		sprintf(full_msg, "%s: %s", msg, error_msg);
		if (msg_window) {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error: closing program", full_msg, msg_window);
		} else {
			write_log(RED "%s" COLOR_RESET "\n", full_msg);
		}
		my_free(full_msg);
		cleanup_and_quit(EXIT_FAILURE);
	}
}

// Allocates memory.
void *my_malloc(size_t size)
{
	return malloc(size);
}

// Free memory assigned by my_malloc().
void my_free(void *ptr)
{
	free(ptr);
}

// Writes the cwd of the executable to buf.
// Returns: success.
SDL_bool get_executable_dir(char *buf, size_t bufsiz)
{
	SDL_bool success = SDL_TRUE;
#ifdef __WIN64__
	DWORD getmodule_success = GetModuleFileNameA(NULL, buf, bufsiz);
	if (getmodule_success == 0) {
		success = SDL_FALSE;
	} else {
		char *last_slash = strrchr(buf, '\\');
		*last_slash = 0;
	}
#elif defined __EMSCRIPTEN__
	buf[0] = '.';
	buf[1] = 0;
#else
	ssize_t readlink_success = readlink("/proc/self/exe", buf, bufsiz);
	if (readlink_success == -1) {
		success = SDL_FALSE;
	} else {
		char *last_slash = strrchr(buf, '/');
		*last_slash = 0;
	}
#endif
	return success;
}

// Writes the absolute filepath to buf, given a filepath relative to the executable.
// Returns: success.
SDL_bool make_absolute_path(char *relative, char *buf, size_t bufsiz)
{
	if (get_executable_dir(buf, bufsiz)) {
		if (strlen(relative) + strlen(buf) + 2 > bufsiz) {
			return SDL_FALSE;
		} else {
#ifdef __WIN64__
			strcat(buf, "\\");
#else
			strcat(buf, "/");
#endif
			strcat(buf, relative);
			return SDL_TRUE;
		}
	} else {
		return SDL_FALSE;
	}
}

