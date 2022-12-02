#ifndef OPENGL_UTIL_H
#define OPENGL_UTIL_H

int pixel_format_buffer_size(int x, int y, int width, int height, GLenum format, GLenum type);
int pixel_read_buffer_size(int x, int y, int width, int height, GLenum format);
void format_screenshot(int x, int y, int width, int height, GLenum format, GLenum type, char *buf, int buflen, GLfloat *pixel_buf, int pixel_buflen);
void format_screenshot_alloc(int x, int y, int width, int height, GLenum format, GLenum type);
SDL_bool have_gl_debug_output(int glad_gl_version);
SDL_bool have_webgl_2(const char *gl_version_str);
void gl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *user_param);
const char *gl_get_error_stringified(void);
void print_shader_log(GLuint shader);
void print_program_log(GLuint program);
GLuint load_shader(char *fname, GLenum shader_type);
GLuint create_shader_program(int num_shaders, GLuint *shaders, int num_outs, char **outs, int num_transforms, const char * const *transforms);

#endif // OPENGL_UTIL_H
