#include <stdio.h>

#ifdef __WIN64__
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <SDL2/SDL.h>

#include "util.h"

static void (*cleanup_fn) (void) = NULL;
static SDL_Window *msg_window = NULL;

void register_cleanup_fn(void (*new_cleanup_fn) (void))
{
	cleanup_fn = new_cleanup_fn;
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
		if (cleanup_fn) {
			cleanup_fn();
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

