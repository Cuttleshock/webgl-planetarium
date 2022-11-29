#ifndef UTIL_H
#define UTIL_H

#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YLW "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define COLOR_RESET "\x1B[0m"

SDL_bool open_log(char *name);
void write_log(char *format, ...);
void my_srand(unsigned int seed);
int my_rand(void);
SDL_bool push_cleanup_fn(void (*new_cleanup_fn) (void));
void register_message_window(SDL_Window *new_msg_window);
void assert_or_debug(SDL_bool assertion, char *msg, const char *(*error_getter) (void));
void assert_or_cleanup(SDL_bool assertion, char *msg, const char *(*error_getter) (void));
void *my_malloc(size_t size);
void my_free(void *ptr);
SDL_bool get_executable_dir(char *buf, size_t bufsiz);
SDL_bool make_absolute_path(char *relative, char *buf, size_t bufsiz);

#endif // UTIL_H
