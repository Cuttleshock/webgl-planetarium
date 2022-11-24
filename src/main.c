#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __WIN64__
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#ifdef __EMSCRIPTEN__
#include <webgl/webgl2.h>
#else
#include <glad/gl.h>
#endif

#include <SDL2/SDL.h>

#define WINDOW_W 640
#define WINDOW_H 480
#define FPS_CAP 60

#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YLW "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define COLOR_RESET "\x1B[0m"

SDL_Window *g_window;
SDL_GLContext g_glcontext;
GLsync g_transform_fence;

GLuint g_draw_vao;
GLuint g_physics_vao;

GLuint g_circle_vbo;
GLuint g_physics_vbo;

GLuint g_position_framebuffer[2];
GLuint g_position_texture[2];
int g_position_framebuffer_active = 0;

GLuint g_attraction_texture[2];
GLuint g_attraction_framebuffer[2];
int g_attraction_framebuffer_active = 0;

GLuint g_draw_program;
GLuint g_physics_program;
GLuint g_attraction_program;
GLuint g_fold_program;

#define POINTS_W 10
#define POINTS_H 6
#define CIRCLE_SIDES 10
// Used together, so have to be distinct
#define POSITION_TEX_UNIT_OFFSET 0
#define ATTRACTION_TEX_UNIT_OFFSET 1
// Used in a separate shader
#define FOLD_TEX_UNIT_OFFSET 0

void *my_malloc(size_t size)
{
	return malloc(size);
}

void my_free(void *ptr)
{
	free(ptr);
}

void free_all(void)
{
#ifdef DEBUG
	printf("Freeing all objects before exiting program\n");
#endif
	if (g_glcontext) {
		SDL_GL_DeleteContext(g_glcontext);
	}
	if (g_window) {
		SDL_DestroyWindow(g_window);
	}
	SDL_Quit();
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
		if (g_window) {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error: closing program", full_msg, g_window);
		} else {
			fprintf(stderr, RED "%s" COLOR_RESET "\n", full_msg);
		}
#endif
		my_free(full_msg);
		free_all();
		exit(1);
	}
}

#ifndef __EMSCRIPTEN__
SDL_bool have_gl_debug_output(int glad_gl_version)
{
	if (glad_gl_version >= GLAD_MAKE_VERSION(4, 3)) {
		return SDL_TRUE;
	} else if (!SDL_GL_ExtensionSupported("KHR_debug")) {
		return SDL_TRUE;
	} else {
		return SDL_FALSE;
	}
}
#endif

#ifdef __EMSCRIPTEN__
SDL_bool have_webgl_2(const char *gl_version_str)
{
	const char gles3[] = "OpenGL ES 3.0";
	if (strncmp(gl_version_str, gles3, sizeof(gles3) - 1) == 0) {
		return SDL_TRUE;
	} else {
		return SDL_FALSE;
	}
}
#endif

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

#if defined DEBUG && !defined __EMSCRIPTEN__
void gl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *user_param)
{
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

	printf("GL_DEBUG: severity %-12s source %-15s type %s\n", str_severity, str_source, str_type);
	printf("          %s\n", message);
}
#endif

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
	printf("%s\n", log_buffer);
	my_free(log_buffer);
}

void print_program_log(GLuint program)
{
	GLint log_length;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
	char *log_buffer = my_malloc(log_length + 1);
	log_buffer[log_length] = 0;
	glGetProgramInfoLog(program, log_length, NULL, log_buffer);
	printf("%s\n", log_buffer);
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

	return program;
}

void push_quit_event(void)
{
	SDL_Event quit_event;
	quit_event.type = SDL_QUIT;
	SDL_PushEvent(&quit_event);
}

SDL_bool update(Uint64 delta)
{
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
			case SDL_QUIT:
				return SDL_TRUE;
			case SDL_KEYDOWN:
				switch (e.key.keysym.scancode) {
					case SDL_SCANCODE_ESCAPE:
						push_quit_event();
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
	}

	return SDL_FALSE;
}

void draw(void)
{
	glClearColor(0.15, 0.1, 0.3, 1.0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT);

	// Calculate all attractions between planets
	glActiveTexture(GL_TEXTURE0 + POSITION_TEX_UNIT_OFFSET);
		glBindTexture(GL_TEXTURE_2D, g_position_texture[g_position_framebuffer_active]);

	glUseProgram(g_attraction_program);
	glBindFramebuffer(GL_FRAMEBUFFER, g_attraction_framebuffer[0]);
	glViewport(0, 0, POINTS_W * POINTS_H, POINTS_W * POINTS_H);
	// Need a valid VAO but doesn't matter which
		glDrawArrays(GL_TRIANGLES, 0, 6);

	// Fold attractions into flat texture
	glUseProgram(g_fold_program);
		for (int fold_factor = 4; fold_factor < POINTS_W * POINTS_H * 4; fold_factor *= 4) {
			glActiveTexture(GL_TEXTURE0 + FOLD_TEX_UNIT_OFFSET);
				glBindTexture(GL_TEXTURE_2D, g_attraction_texture[g_attraction_framebuffer_active]);

			g_attraction_framebuffer_active = (g_attraction_framebuffer_active + 1) % 2;

			glBindFramebuffer(GL_FRAMEBUFFER, g_attraction_framebuffer[g_attraction_framebuffer_active]);
			glViewport(0, 0, (POINTS_W * POINTS_H + fold_factor - 1) / fold_factor, POINTS_W * POINTS_H);
				glDrawArrays(GL_TRIANGLES, 0, 6);
		}

	// Bind last frame's position texture to uniform slot
	glActiveTexture(GL_TEXTURE0 + POSITION_TEX_UNIT_OFFSET);
		glBindTexture(GL_TEXTURE_2D, g_position_texture[g_position_framebuffer_active]);

	// Bind flat attractions to uniform slot
	glActiveTexture(GL_TEXTURE0 + ATTRACTION_TEX_UNIT_OFFSET);
		glBindTexture(GL_TEXTURE_2D, g_attraction_texture[g_attraction_framebuffer_active]);

	glBindVertexArray(g_physics_vao);
	glUseProgram(g_physics_program);
	g_position_framebuffer_active = (g_position_framebuffer_active + 1) % 2;
	glBindFramebuffer(GL_FRAMEBUFFER, g_position_framebuffer[g_position_framebuffer_active]);
	glViewport(0, 0, POINTS_W * POINTS_H, 1);
		glBeginTransformFeedback(GL_POINTS);
			glDrawArrays(GL_POINTS, 0, POINTS_W * POINTS_H);
		glEndTransformFeedback();

		glBindBuffer(GL_ARRAY_BUFFER, g_physics_vbo);
			glCopyBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, GL_ARRAY_BUFFER, 0, 0, 6 * sizeof(GLfloat) * POINTS_W * POINTS_H);

	glBindVertexArray(g_draw_vao);
	glUseProgram(g_draw_program);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WINDOW_W, WINDOW_H);
		glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, CIRCLE_SIDES + 2, POINTS_W * POINTS_H);

	SDL_GL_SwapWindow(g_window);
}

void update_post_draw(Uint64 delta)
{
}

SDL_bool main_loop(Uint64 delta)
{
	SDL_bool loop_done = update(delta);
	draw();
	update_post_draw(delta);
	return loop_done;
}

#ifdef __EMSCRIPTEN__
void main_loop_emscripten(void)
{
	SDL_bool loop_done = main_loop(16);
	if (loop_done) {
		emscripten_cancel_main_loop();
		free_all();
	}
}
#endif

int main(int argc, char *argv[])
{
	srand(time(NULL));

	assert_or_cleanup(
		SDL_Init(SDL_INIT_VIDEO) == 0,
		"Failed to init SDL subsystems",
		SDL_GetError
	);

#ifdef __EMSCRIPTEN__
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	g_window = SDL_CreateWindow(
		"Particle gravity",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		WINDOW_W,
		WINDOW_H,
		SDL_WINDOW_OPENGL
	);
	assert_or_cleanup(g_window != NULL, "Failed to open window", SDL_GetError);

	g_glcontext = SDL_GL_CreateContext(g_window);
	assert_or_cleanup(g_glcontext != NULL, "Failed to create OpenGL context", SDL_GetError);

#ifdef __EMSCRIPTEN__
	const GLubyte *gl_version_str = glGetString(GL_VERSION);
	printf("%s\n", gl_version_str);
	assert_or_cleanup(have_webgl_2((const char *) gl_version_str), "This browser does not support WebGL 2.0", gl_get_error_stringified);
#else
	int gl_version = gladLoadGL((GLADloadfunc) SDL_GL_GetProcAddress);
	assert_or_cleanup(gl_version != 0, "Failed to load OpenGL functions", NULL);
	printf("OpenGL version %d.%d\n", GLAD_VERSION_MAJOR(gl_version), GLAD_VERSION_MINOR(gl_version));

	#ifdef DEBUG
		if (!have_gl_debug_output(gl_version)) {
			printf("GL_DEBUG_OUTPUT unavailable\n");
		} else {
			printf("Enabling GL_DEBUG_OUTPUT\n");
			glEnable(GL_DEBUG_OUTPUT);
			glDebugMessageCallback(gl_debug_message_callback, NULL);
		}
	#endif
#endif

	glGenVertexArrays(1, &g_physics_vao);
	glGenVertexArrays(1, &g_draw_vao);
	glBindVertexArray(g_physics_vao);

	GLuint tfbo;
	glGenBuffers(1, &tfbo);
	glBindBuffer(GL_ARRAY_BUFFER, tfbo);
	glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(GLfloat) * POINTS_W * POINTS_H, NULL, GL_DYNAMIC_COPY);

	glGenBuffers(1, &g_circle_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, g_circle_vbo);
	glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(float) * (CIRCLE_SIDES + 2), NULL, GL_STATIC_DRAW);

	glGenBuffers(1, &g_physics_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, g_physics_vbo);
	glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(GLfloat) * POINTS_W * POINTS_H, NULL, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0); // Prevent writing to currently-bound VBO

	GLuint init_shaders[2]; // vertex, fragment

	init_shaders[0] = load_shader("shaders/init_particles.vert", GL_VERTEX_SHADER);
	assert_or_cleanup(init_shaders[0] != 0, "Failed to load init vert shader", NULL);

	init_shaders[1] = load_shader("shaders/init_particles.frag", GL_FRAGMENT_SHADER);
	assert_or_cleanup(init_shaders[1] != 0, "Failed to load init frag shader", NULL);

	const char *init_strings[] = { "init_speed", "init_color" };
	GLuint init_program = create_shader_program(2, init_shaders, 0, NULL, 2, init_strings);
	assert_or_cleanup(init_program != 0, "Failed to create init program", gl_get_error_stringified);

	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, g_physics_vbo);
	glUseProgram(init_program);
		glEnable(GL_RASTERIZER_DISCARD);
			glBeginTransformFeedback(GL_POINTS);
				glDrawArrays(GL_POINTS, 0, POINTS_W * POINTS_H);
			glEndTransformFeedback();
		glDisable(GL_RASTERIZER_DISCARD);

	GLuint circle_shaders[2]; // vertex, fragment

	circle_shaders[0] = load_shader("shaders/init_circle.vert", GL_VERTEX_SHADER);
	assert_or_cleanup(circle_shaders[0] != 0, "Failed to load circle init vert shader", NULL);

	circle_shaders[1] = load_shader("shaders/init_circle.frag", GL_FRAGMENT_SHADER);
	assert_or_cleanup(circle_shaders[1] != 0, "Failed to load circle init frag shader", NULL);

	const char *init_circle_tfs[] = { "position" };
	GLuint init_circle_program = create_shader_program(2, circle_shaders, 0, NULL, 1, init_circle_tfs);
	assert_or_cleanup(init_circle_program != 0, "Failed to create circle init program", gl_get_error_stringified);

	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, g_circle_vbo);
	glUseProgram(init_circle_program);
		glUniform1i(glGetUniformLocation(init_circle_program, "num_sides"), CIRCLE_SIDES);

		glEnable(GL_RASTERIZER_DISCARD);
			glBeginTransformFeedback(GL_POINTS);
				glDrawArrays(GL_POINTS, 0, CIRCLE_SIDES + 2);
			glEndTransformFeedback();
		glDisable(GL_RASTERIZER_DISCARD);

	GLuint draw_shaders[2]; // vertex, fragment

	draw_shaders[0] = load_shader("shaders/particles.vert", GL_VERTEX_SHADER);
	assert_or_cleanup(draw_shaders[0] != 0, "Failed to load draw vertex shader", NULL);

	draw_shaders[1] = load_shader("shaders/particles.frag", GL_FRAGMENT_SHADER);
	assert_or_cleanup(draw_shaders[1] != 0, "Failed to load draw fragment shader", NULL);

	char *outs = "out_color";
	g_draw_program = create_shader_program(2, draw_shaders, 1, &outs, 0, NULL);
	assert_or_cleanup(g_draw_program != 0, "Failed to create draw shader program", gl_get_error_stringified);

	glUseProgram(g_draw_program);
	glBindVertexArray(g_draw_vao);
		glUniform1i(glGetUniformLocation(g_draw_program, "positions"), POSITION_TEX_UNIT_OFFSET);

		GLint in_vertex = glGetAttribLocation(g_draw_program, "vert_displacement");
		GLint in_color_draw = glGetAttribLocation(g_draw_program, "color");

		glEnableVertexAttribArray(in_vertex);
		glEnableVertexAttribArray(in_color_draw);
		glVertexAttribDivisor(in_color_draw, 1);

		glBindBuffer(GL_ARRAY_BUFFER, g_circle_vbo);
			glVertexAttribPointer(in_vertex, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, g_physics_vbo);
			glVertexAttribPointer(in_color_draw, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)(2 * sizeof(GLfloat)));

	GLuint physics_shaders[2]; // vertex, fragment

	physics_shaders[0] = load_shader("shaders/update_particles.vert", GL_VERTEX_SHADER);
	assert_or_cleanup(physics_shaders[0] != 0, "Failed to load update vertex shader", NULL);

	physics_shaders[1] = load_shader("shaders/update_particles.frag", GL_FRAGMENT_SHADER);
	assert_or_cleanup(physics_shaders[1] != 0, "Failed to load update fragment shader", NULL);

	char *outs_update = "out_position";
	const char *transforms[] = { "speed_feedback", "color_feedback" };
	g_physics_program = create_shader_program(2, physics_shaders, 1, &outs_update, 2, transforms);
	assert_or_cleanup(g_physics_program != 0, "Failed to create update shader program", gl_get_error_stringified);

	glUseProgram(g_physics_program);
	glBindVertexArray(g_physics_vao);
		glUniform1i(glGetUniformLocation(g_physics_program, "positions"), POSITION_TEX_UNIT_OFFSET);
		glUniform1i(glGetUniformLocation(g_physics_program, "attractions"), ATTRACTION_TEX_UNIT_OFFSET);
		glUniform1f(glGetUniformLocation(g_physics_program, "num_planets"), (GLfloat)(POINTS_W * POINTS_H));

		GLint in_speed = glGetAttribLocation(g_physics_program, "speed");
		GLint in_color = glGetAttribLocation(g_physics_program, "color");

		glEnableVertexAttribArray(in_speed);
		glEnableVertexAttribArray(in_color);

		glBindBuffer(GL_ARRAY_BUFFER, g_physics_vbo);
			glVertexAttribPointer(in_speed, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
			glVertexAttribPointer(in_color, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)(2 * sizeof(GLfloat)));

	// This is here to stay for the rest of the program
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tfbo);

	GLuint init_tex_position_shaders[2]; // vertex, fragment

	init_tex_position_shaders[0] = load_shader("shaders/quad.vert", GL_VERTEX_SHADER);
	assert_or_cleanup(init_tex_position_shaders[0] != 0, "Failed to load quad.vert", NULL);

	init_tex_position_shaders[1] = load_shader("shaders/init_particles_tex.frag", GL_FRAGMENT_SHADER);
	assert_or_cleanup(init_tex_position_shaders[1] != 0, "Failed to load init_particles_tex.frag", NULL);

	char *init_tex_position_out = "out_position";
	GLuint init_tex_position_program = create_shader_program(2, init_tex_position_shaders, 1, &init_tex_position_out, 0, NULL);
	assert_or_cleanup(init_tex_position_program != 0, "Failed to link quad.vert and init_particles_tex.frag", gl_get_error_stringified);

	// Flat n * 1 texture of all planet positions
	glGenTextures(2, g_position_texture);
	glGenFramebuffers(2, g_position_framebuffer);

	for (int i = 0; i < 2; ++i) {
		glBindTexture(GL_TEXTURE_2D, g_position_texture[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, POINTS_W * POINTS_H, 1, 0, GL_RG, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, g_position_framebuffer[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_position_texture[i], 0);
			assert_or_cleanup(
				glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
				"Planet position framebuffer incomplete",
				gl_get_error_stringified
			);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	glUseProgram(init_tex_position_program);
	glBindFramebuffer(GL_FRAMEBUFFER, g_position_framebuffer[0]);
	// Need a valid VAO but doesn't matter which
		glUniform1i(glGetUniformLocation(init_tex_position_program, "width"), POINTS_W);
		glUniform1i(glGetUniformLocation(init_tex_position_program, "height"), POINTS_H);

		glDrawArrays(GL_TRIANGLES, 0, 6);

	GLuint attraction_shaders[2]; // vertex, fragment

	attraction_shaders[0] = load_shader("shaders/quad.vert", GL_VERTEX_SHADER);
	assert_or_cleanup(attraction_shaders[0] != 0, "Failed to load quad.vert", NULL);

	attraction_shaders[1] = load_shader("shaders/calc_particle_attractions.frag", GL_FRAGMENT_SHADER);
	assert_or_cleanup(attraction_shaders[1] != 0, "Failed to load calc_particle_attractions.frag", NULL);

	char *attraction_out = "out_distance";
	g_attraction_program = create_shader_program(2, attraction_shaders, 1, &attraction_out, 0, NULL);
	assert_or_cleanup(g_attraction_program != 0, "Failed to link quad.vert and calc_particle_attractions.frag", gl_get_error_stringified);

	glUseProgram(g_attraction_program);
		glUniform1i(glGetUniformLocation(g_attraction_program, "positions"), POSITION_TEX_UNIT_OFFSET);

	// n * n array of all planet pairs, with second for double-buffered summing
	glGenTextures(2, g_attraction_texture);
	glGenFramebuffers(2, g_attraction_framebuffer);

	for (int i = 0; i < 2; ++i) {
		glBindTexture(GL_TEXTURE_2D, g_attraction_texture[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, POINTS_W * POINTS_H, POINTS_W * POINTS_H, 0, GL_RG, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, g_attraction_framebuffer[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_attraction_texture[i], 0);
			assert_or_cleanup(
				glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
				"Attraction matrix framebuffer incomplete",
				gl_get_error_stringified
			);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	GLuint fold_shaders[2];

	fold_shaders[0] = load_shader("shaders/quad.vert", GL_VERTEX_SHADER);
	assert_or_cleanup(fold_shaders[0] != 0, "Failed to load quad.vert", NULL);

	fold_shaders[1] = load_shader("shaders/fold_texture.frag", GL_FRAGMENT_SHADER);
	assert_or_cleanup(fold_shaders[1] != 0, "Failed to load fold_texture.frag", NULL);

	char *fold_out = "out_sum";
	g_fold_program = create_shader_program(2, fold_shaders, 1, &fold_out, 0, NULL);
	assert_or_cleanup(g_fold_program != 0, "Failed to link quad.vert and fold_texture.frag", gl_get_error_stringified);

	glUseProgram(g_fold_program);
		glUniform1i(glGetUniformLocation(g_fold_program, "inputs"), FOLD_TEX_UNIT_OFFSET);

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(main_loop_emscripten, 0, EM_TRUE);
#else
	Uint64 f0_start = SDL_GetTicks64() - 1000 / FPS_CAP; // Last frame
	Uint64 f1_start; // Current frame
	Uint64 f1_end;
	Uint64 f2_start; // Next frame

#ifdef DEBUG
	Uint64 frame_number = 0; // Which frame are we on
	Uint64 recent_delays[FPS_CAP] = { 0 }; // Sliding window of frame times
	for (int i = 0; i < FPS_CAP; ++i) {
		recent_delays[i] = 1000 / FPS_CAP;
	}
	Uint64 recent_total = (1000 / FPS_CAP) * FPS_CAP; // Sum of array
#endif

	SDL_bool loop_done = SDL_FALSE;

	while (!loop_done) {
		f1_start = SDL_GetTicks64();
		f2_start = f1_start + 1000 / FPS_CAP;
		loop_done = main_loop(f1_start - f0_start);
		f1_end = SDL_GetTicks64();
		f0_start = f1_start;
#ifdef DEBUG
		recent_total -= recent_delays[frame_number];
		recent_delays[frame_number] = f1_end - f1_start;
		recent_total += recent_delays[frame_number];
		if (frame_number == 0) {
			printf("%2.2f FPS\n", (1000.0 * (float) FPS_CAP) / ((float) recent_total));
		}
		frame_number = (frame_number + 1) % FPS_CAP;
#endif
		if (f2_start > f1_end) {
			SDL_Delay(f2_start - f1_end);
		}
	}
#endif

	free_all();
	return 0;
}
