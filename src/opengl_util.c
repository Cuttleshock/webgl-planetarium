#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <webgl/webgl2.h>
#else
#include "glad_gl.h"
#endif

#include <SDL2/SDL.h>

#include "util.h"

int floats_per_pixel(GLenum format)
{
	switch (format) {
		case GL_RG:
			return 2;
		default:
			return 0;
	}
}

int pixel_format_buffer_size(int x, int y, int width, int height, GLenum format, GLenum type)
{
	int pixel_size = floats_per_pixel(format);
	if (pixel_size == 0) {
		write_log("Unsupported pixel format: %d\n", format);
		return 0;
	}

	if (type != GL_FLOAT) {
		write_log("Unsupported pixel data type: %d\n", type);
		return 0;
	}

	// Suggest 8 chars per float: sufficient for values in the tens or hundreds
	return (width - x) * (height - y) * pixel_size * 8;
}

int pixel_read_buffer_size(int x, int y, int width, int height, GLenum format)
{
	int pixel_size = floats_per_pixel(format);
	if (pixel_size == 0) {
		write_log("Unsupported pixel format: %d\n", format);
		return 0;
	}

	return (width - x) * (height - y) * pixel_size;
}

void format_screenshot(int x, int y, int width, int height, GLenum format, GLenum type, char *buf, int buflen, GLfloat *pixel_buf, int pixel_buflen)
{
	int pixel_size = floats_per_pixel(format);
	if (pixel_size == 0) {
		write_log("Unsupported pixel format for screenshot: %d\n", format);
		return;
	}

	if (type != GL_FLOAT) {
		write_log("Unsupported pixel data type for screenshot: %d\n", type);
		return;
	}

	if (pixel_buflen < pixel_read_buffer_size(x, y, width, height, format)) {
		write_log("Insufficient pixel buffer size for screenshot\n");
		return;
	}

	glReadPixels(x, y, width, height, format, type, pixel_buf);
	int ptr = 0;

	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			int offset = (i * width + j) * pixel_size;
			ptr += snprintf(&buf[ptr], buflen - ptr - 2, "%1.2f %1.2f, ", pixel_buf[offset], pixel_buf[offset + 1]);
			if (ptr >= buflen - 2) {
				write_log("Screenshot truncated at pixel (%d, %d)\n", j, i);
				return;
			}
		}
		ptr += sprintf(&buf[ptr], "\n");
	}
}

void format_screenshot_alloc(int x, int y, int width, int height, GLenum format, GLenum type)
{
	int buflen = pixel_format_buffer_size(x, y, width, height, format, type);
	if (buflen == 0) {
		return;
	}

	int pixel_buflen = pixel_read_buffer_size(x, y, width, height, format);
	if (pixel_buflen == 0) {
		return;
	}

	char *buf = my_malloc(buflen);
	GLfloat *pixel_buf = my_malloc(pixel_buflen * sizeof(GLfloat));
	format_screenshot(x, y, width, height, format, type, buf, buflen, pixel_buf, pixel_buflen);
	write_log("%s\n", buf);
	my_free(pixel_buf);
	my_free(buf);
}

SDL_bool have_gl_debug_output(int glad_gl_version)
{
#ifdef __EMSCRIPTEN__
	return SDL_FALSE;
#else
	if (glad_gl_version >= GLAD_MAKE_VERSION(4, 3)) {
		return SDL_TRUE;
	} else if (!SDL_GL_ExtensionSupported("KHR_debug")) {
		return SDL_TRUE;
	} else {
		return SDL_FALSE;
	}
#endif // __EMSCRIPTEN__
}

SDL_bool have_webgl_2(const char *gl_version_str)
{
#ifdef __EMSCRIPTEN__
	const char gles3[] = "OpenGL ES 3.0";
	if (strncmp(gl_version_str, gles3, sizeof(gles3) - 1) == 0) {
		return SDL_TRUE;
	} else {
		return SDL_FALSE;
	}
#else
	return SDL_FALSE;
#endif // __EMSCRIPTEN__
}

void gl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *user_param)
{
#ifdef __EMSCRIPTEN__
	return;
#else
	const char *str_source;
	const char *str_type;
	const char *str_severity;

#define CASE(X) case GL_DEBUG_SOURCE_##X: str_source = #X; break;
	switch (source) {
		CASE(API);
		CASE(APPLICATION);
		CASE(OTHER);
		CASE(SHADER_COMPILER);
		CASE(THIRD_PARTY);
		CASE(WINDOW_SYSTEM);
		default:
			str_source = "[unknown]";
			break;
	}
#undef CASE

#define CASE(X) case GL_DEBUG_TYPE_##X: str_type = #X; break;
	switch (type) {
		CASE(DEPRECATED_BEHAVIOR);
		CASE(ERROR);
		CASE(MARKER);
		CASE(OTHER);
		CASE(PERFORMANCE);
		CASE(POP_GROUP);
		CASE(PORTABILITY);
		CASE(PUSH_GROUP);
		CASE(UNDEFINED_BEHAVIOR);
		default:
			str_type = "[unknown]";
			break;
	}
#undef CASE

#define CASE(X, COLOR) case GL_DEBUG_SEVERITY_##X: str_severity = COLOR #X COLOR_RESET; break;
	switch (severity) {
		CASE(HIGH, RED);
		CASE(LOW, GRN);
		CASE(MEDIUM, YLW);
		CASE(NOTIFICATION, BLU);
		default:
			str_severity = "[unknown]";
			break;
	}
#undef CASE

	write_log("GL_DEBUG: severity %-12s source %-15s type %s\n", str_severity, str_source, str_type);
	write_log("          %s\n", message);
#endif // __EMSCRIPTEN__
}

const char *gl_get_error_stringified(void)
{
#define CASE(X) case GL_##X: return #X;
	switch (glGetError()) {
		CASE(NO_ERROR);
		CASE(INVALID_ENUM);
		CASE(INVALID_VALUE);
		CASE(INVALID_OPERATION);
		CASE(INVALID_FRAMEBUFFER_OPERATION);
		CASE(OUT_OF_MEMORY);
		default:
			return "[not a valid OpenGL error flag]";
	}
#undef CASE
}

void print_shader_log(GLuint shader)
{
	GLint log_length;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
	char *log_buffer = my_malloc(log_length + 1);
	log_buffer[log_length] = 0;
	glGetShaderInfoLog(shader, log_length, NULL, log_buffer);
	write_log("%s\n", log_buffer);
	my_free(log_buffer);
}

void print_program_log(GLuint program)
{
	GLint log_length;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
	char *log_buffer = my_malloc(log_length + 1);
	log_buffer[log_length] = 0;
	glGetProgramInfoLog(program, log_length, NULL, log_buffer);
	write_log("%s\n", log_buffer);
	my_free(log_buffer);
}

// Return value: a shader handle, or 0 on failure
GLuint load_shader(char *fname, GLenum shader_type)
{
	char full_path[256] = {0};
	GLchar *buffer = NULL;

	SDL_bool filepath_success = make_absolute_path(fname, full_path, 256);
	if (!filepath_success) {
		return 0;
	}

	// Open file
	FILE *file = fopen(full_path, "r");
	if (file == NULL) {
		return 0;
	}

	// Determine size of shader source
	int fseek_err = fseek(file, 0, SEEK_END); // TODO: fseeko(), ftello() safer
	if (fseek_err != 0) {
		fclose(file);
		return 0;
	}
	long fsize = ftell(file);
	buffer = my_malloc(fsize + 1);
	buffer[fsize] = 0;
	rewind(file);

	// Copy source into buffer
	size_t bytes_read = fread(buffer, 1, fsize + 1, file);
	if (ferror(file) != 0 || bytes_read != fsize || feof(file) == 0) {
		my_free(buffer);
		fclose(file);
		return 0;
	}

	fclose(file);

	// Compile shader
	GLuint shader = glCreateShader(shader_type);
	glShaderSource(shader, 1, (const char * const *)&buffer, NULL);
	glCompileShader(shader);

	my_free(buffer);

	GLint compile_status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
	if (compile_status != GL_TRUE) {
		print_shader_log(shader);
		return 0;
	}

	// Success!
	return shader;
}

GLuint create_shader_program(int num_shaders, GLuint *shaders, int num_outs, char **outs, int num_transforms, const char * const *transforms)
{
	GLuint program = glCreateProgram();

	for (int i = 0; i < num_shaders; ++i) {
		glAttachShader(program, shaders[i]);
	}
	for (int i = 0; i < num_outs; ++i) {
#ifdef __EMSCRIPTEN__
		glBindAttribLocation(program, i, outs[i]);
		if (i == 0) {
			glEnableVertexAttribArray(i);
		}
#else
		glBindFragDataLocation(program, i, outs[i]);
#endif
	}
	glTransformFeedbackVaryings(program, num_transforms, transforms, GL_INTERLEAVED_ATTRIBS);
	glLinkProgram(program);

	for (int i = 0; i < num_shaders; ++i) {
		glDetachShader(program, shaders[i]);
		glDeleteShader(shaders[i]);
	}

	GLint link_status;
	glGetProgramiv(program, GL_LINK_STATUS, &link_status);
	if (link_status != GL_TRUE) {
		print_program_log(program);
		return 0;
	}

	GLint validate_status;
	glValidateProgram(program);
	glGetProgramiv(program, GL_VALIDATE_STATUS, &validate_status);
	if (validate_status != GL_TRUE) {
		print_program_log(program);
		return 0;
	}

	return program;
}
