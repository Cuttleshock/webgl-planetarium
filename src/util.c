#include <stdio.h>
#include <stdarg.h>

#ifdef __WIN64__
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <SDL2/SDL.h>

#include "util.h"

static void (*cleanup_fns[32]) (void) = { NULL };
static int num_cleanup_fns = 0;
static SDL_Window *msg_window = NULL;
static FILE *log_file = NULL;

void close_log(void)
{
	if (log_file) {
		int ret = fclose(log_file);
		log_file = NULL;
		if (ret != 0) {
			printf("Failed to close log file\n");
		}
	}
}

SDL_bool open_log(char *fname)
{
	printf("Opening log file %s\n", fname);
	log_file = fopen(fname, "a");
	if (log_file == NULL) {
		return SDL_FALSE;
	} else {
		push_cleanup_fn(close_log);
		return SDL_TRUE;
	}
}

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
	fputc('\n', out_stream);
}

void my_srand(unsigned int seed)
{
	srand(seed);
}

int my_rand(void)
{
	return rand();
}

SDL_bool push_cleanup_fn(void (*new_cleanup_fn) (void))
{
	const int max_cleanup_fns = sizeof(cleanup_fns) / sizeof(cleanup_fns[0]);
	if (num_cleanup_fns >= max_cleanup_fns) {
		printf("Cannot register cleanup function: exceeded limit of %d\n", max_cleanup_fns);
		return SDL_FALSE;
	} else {
		cleanup_fns[num_cleanup_fns] = new_cleanup_fn;
		++num_cleanup_fns;
		return SDL_TRUE;
	}
}

void register_message_window(SDL_Window *new_msg_window)
{
	msg_window = new_msg_window;
}

void assert_or_debug(SDL_bool assertion, char *msg, const char *(*error_getter) (void))
{
#ifdef DEBUG
	if (!assertion) {
		printf(BLU "%s: %s" COLOR_RESET "\n", msg, error_getter ? error_getter() : "(no debug info)");
	}
#endif
}

void assert_or_cleanup(SDL_bool assertion, char *msg, const char *(*error_getter) (void))
{
	if (!assertion) {
		const char *error_msg = error_getter ? error_getter() : "(no debug info)";
		char *full_msg = my_malloc(strlen(msg) + strlen(error_msg) + 3);
		sprintf(full_msg, "%s: %s", msg, error_msg);
#ifdef DEBUG
		fprintf(stderr, RED "%s" COLOR_RESET "\n", full_msg);
#else
		if (msg_window) {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error: closing program", full_msg, msg_window);
		} else {
			fprintf(stderr, RED "%s" COLOR_RESET "\n", full_msg);
		}
#endif
		my_free(full_msg);
		for (; num_cleanup_fns > 0; --num_cleanup_fns) {
			cleanup_fns[num_cleanup_fns - 1]();
		}
		exit(1);
	}
}

void *my_malloc(size_t size)
{
	return malloc(size);
}

void my_free(void *ptr)
{
	free(ptr);
}

// Return value: success
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

// Return value: success
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

