#include <time.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#ifdef __EMSCRIPTEN__
#include <webgl/webgl2.h>
#else
#include "glad_gl.h"
#endif

#include <SDL2/SDL.h>

#include "util.h"
#include "opengl_util.h"

#define WINDOW_W 640
#define WINDOW_H 480
#define FPS_CAP 60

SDL_Window *g_window;
SDL_GLContext g_glcontext;

GLuint g_draw_vao;

GLuint g_circle_vbo;
GLuint g_colour_vbo;

GLuint g_motion_framebuffer[2];
GLuint g_motion_texture[2];
int g_motion_framebuffer_active = 0;

GLuint g_impulse_texture[2];
GLuint g_impulse_framebuffer[2];
int g_impulse_framebuffer_active = 0;

GLuint g_draw_program;
GLuint g_motion_program;
GLuint g_intersection_program;
GLuint g_attraction_program;
GLuint g_fold_program;

int g_num_planets = 0;

SDL_bool g_dragging_camera = SDL_FALSE;
GLfloat g_camera[2] = { 0.0, 0.0 };

#define MAX_PLANETS 128
#define POINT_RADIUS 0.02
#define CIRCLE_SIDES 10
// Used together, so have to be distinct
#define POSITION_TEX_UNIT_OFFSET 0
#define ATTRACTION_TEX_UNIT_OFFSET 1
// Used in a separate shader
#define FOLD_TEX_UNIT_OFFSET 0

void destroy_window(void)
{
	SDL_DestroyWindow(g_window);
}

void delete_glcontext(void)
{
	SDL_GL_DeleteContext(g_glcontext);
}

void create_planet(GLfloat x, GLfloat y, GLfloat dx, GLfloat dy, GLfloat r, GLfloat g, GLfloat b)
{
	if (g_num_planets >= MAX_PLANETS) {
		assert_or_debug(SDL_FALSE, "Attempted to create planet over limit", NULL);
		return;
	}

	GLfloat vbo_data[] = {
		r, g, b, 1.0,
	};
	glBindBuffer(GL_ARRAY_BUFFER, g_colour_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, g_num_planets * sizeof(vbo_data), sizeof(vbo_data), vbo_data);

	glBindTexture(GL_TEXTURE_2D, g_motion_texture[g_motion_framebuffer_active]);
		GLfloat position_data[] = { x, y, dx, dy };
		glTexSubImage2D(GL_TEXTURE_2D, 0, g_num_planets, 0, 1, 1, GL_RGBA, GL_FLOAT, position_data);

	++g_num_planets;
}

void push_quit_event(void)
{
	SDL_Event quit_event;
	quit_event.type = SDL_QUIT;
	SDL_PushEvent(&quit_event);
}

// Make planet at *absolute* position (x, y)
void create_random_planet(Sint32 x, Sint32 y)
{
	GLfloat x_relative = (GLfloat)(x) * 2.0 / WINDOW_W - 1.0;
	GLfloat y_relative = 1.0 - (GLfloat)(y) * 2.0 / WINDOW_H;
	GLfloat dx = (1.0 - (GLfloat)(x - g_camera[0]) * 2.0 / WINDOW_W) * 0.003;
	GLfloat dy = ((GLfloat)(y - g_camera[1]) * 2.0 / WINDOW_H - 1.0) * 0.003;
	GLfloat r = (my_rand() % 256) * (1.0 / 256.0);
	GLfloat g = (my_rand() % 256) * (1.0 / 256.0);
	GLfloat b = (my_rand() % 256) * (1.0 / 256.0);

	create_planet(x_relative, y_relative, dx, dy, r, g, b);
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
			case SDL_MOUSEBUTTONDOWN:
				switch (e.button.button) {
					case SDL_BUTTON_RIGHT:
						create_random_planet(e.button.x + g_camera[0], e.button.y + g_camera[1]);
						break;
					case SDL_BUTTON_LEFT:
						g_dragging_camera = SDL_TRUE;
						break;
					default:
						break;
				}
				break;
			case SDL_MOUSEBUTTONUP:
				switch (e.button.button) {
					case SDL_BUTTON_LEFT:
						g_dragging_camera = SDL_FALSE;
						break;
					default:
						break;
				}
				break;
			case SDL_MOUSEMOTION:
				if (g_dragging_camera) {
					g_camera[0] -= e.motion.xrel;
					g_camera[1] -= e.motion.yrel;
					glUniform2f(
						glGetUniformLocation(g_draw_program, "camera"),
						2.0 * (GLfloat)(g_camera[0]) / WINDOW_W,
						-2.0 * (GLfloat)(g_camera[1]) / WINDOW_H
					);
				}
				break;
			default:
				break;
		}
	}

	return SDL_FALSE;
}

void resolve_intersections()
{
	glActiveTexture(GL_TEXTURE0 + POSITION_TEX_UNIT_OFFSET);
		glBindTexture(GL_TEXTURE_2D, g_motion_texture[g_motion_framebuffer_active]);

	glUseProgram(g_intersection_program);
	glBindFramebuffer(GL_FRAMEBUFFER, g_impulse_framebuffer[g_impulse_framebuffer_active]);
	glViewport(0, 0, g_num_planets, g_num_planets);
	// Need a valid VAO but doesn't matter which
		glDrawArrays(GL_TRIANGLES, 0, 6);
}

void calculate_gravity(void)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
		glActiveTexture(GL_TEXTURE0 + POSITION_TEX_UNIT_OFFSET);
			glBindTexture(GL_TEXTURE_2D, g_motion_texture[g_motion_framebuffer_active]);

		glUseProgram(g_attraction_program);
		glBindFramebuffer(GL_FRAMEBUFFER, g_impulse_framebuffer[g_impulse_framebuffer_active]);
		glViewport(0, 0, g_num_planets, g_num_planets);
		// Need a valid VAO but doesn't matter which
			glDrawArrays(GL_TRIANGLES, 0, 6);
	glBlendFunc(GL_ONE, GL_ZERO); // default values
	glDisable(GL_BLEND);
}

void fold_gravity_texture(void)
{
	glUseProgram(g_fold_program);
		for (int fold_factor = 4; fold_factor < g_num_planets * 4; fold_factor *= 4) {
			glActiveTexture(GL_TEXTURE0 + FOLD_TEX_UNIT_OFFSET);
				glBindTexture(GL_TEXTURE_2D, g_impulse_texture[g_impulse_framebuffer_active]);

			g_impulse_framebuffer_active = (g_impulse_framebuffer_active + 1) % 2;

			glBindFramebuffer(GL_FRAMEBUFFER, g_impulse_framebuffer[g_impulse_framebuffer_active]);

			glClearColor(0.0, 0.0, 0.0, 0.0);
			glViewport(0, 0, g_num_planets, g_num_planets);
				glClear(GL_COLOR_BUFFER_BIT);

			glViewport(0, 0, (g_num_planets + fold_factor - 1) / fold_factor, g_num_planets);
				glDrawArrays(GL_TRIANGLES, 0, 6);
		}
}

void resolve_motion(GLfloat delta)
{
	// Bind last frame's position texture to uniform slot
	glActiveTexture(GL_TEXTURE0 + POSITION_TEX_UNIT_OFFSET);
		glBindTexture(GL_TEXTURE_2D, g_motion_texture[g_motion_framebuffer_active]);

	// Bind flat attractions to uniform slot
	glActiveTexture(GL_TEXTURE0 + ATTRACTION_TEX_UNIT_OFFSET);
		glBindTexture(GL_TEXTURE_2D, g_impulse_texture[g_impulse_framebuffer_active]);

	glUseProgram(g_motion_program);
	g_motion_framebuffer_active = (g_motion_framebuffer_active + 1) % 2;
	glBindFramebuffer(GL_FRAMEBUFFER, g_motion_framebuffer[g_motion_framebuffer_active]);
	glViewport(0, 0, g_num_planets, 1);
		glUniform1f(glGetUniformLocation(g_motion_program, "time_step"), delta);
		glDrawArrays(GL_LINES, 0, 2);
}

void gpu_update(Uint64 delta)
{
	resolve_intersections();
	calculate_gravity();
	fold_gravity_texture();
	resolve_motion((GLfloat)(delta) / 1000.0);
}

void draw(void)
{
	glClearColor(0.15, 0.1, 0.3, 1.0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WINDOW_W, WINDOW_H);
		glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(g_draw_vao);
	glUseProgram(g_draw_program);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WINDOW_W, WINDOW_H);
		glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, CIRCLE_SIDES + 2, g_num_planets);

	SDL_GL_SwapWindow(g_window);
}

SDL_bool main_loop(Uint64 delta)
{
	SDL_bool loop_done = update(delta);
	gpu_update(delta);
	draw();
	return loop_done;
}

#ifdef __EMSCRIPTEN__
void main_loop_emscripten(void)
{
	SDL_bool loop_done = main_loop(16);
	if (loop_done) {
		emscripten_cancel_main_loop();
		cleanup_and_quit(EXIT_SUCCESS);
	}
}
#endif

int main(int argc, char *argv[])
{
	my_srand(time(NULL));
#ifdef DEBUG
	open_log("planetarium.log");
#endif

	assert_or_cleanup(
		SDL_Init(SDL_INIT_VIDEO) == 0,
		"Failed to init SDL subsystems",
		SDL_GetError
	);
	push_cleanup_fn(SDL_Quit);

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
	push_cleanup_fn(destroy_window);
#ifndef DEBUG
	register_message_window(g_window);
#endif

	g_glcontext = SDL_GL_CreateContext(g_window);
	assert_or_cleanup(g_glcontext != NULL, "Failed to create OpenGL context", SDL_GetError);
	push_cleanup_fn(delete_glcontext);

#ifdef __EMSCRIPTEN__
	const GLubyte *gl_version_str = glGetString(GL_VERSION);
	write_log("%s\n", gl_version_str);
	assert_or_cleanup(have_webgl_2((const char *) gl_version_str), "This browser does not support WebGL 2.0", gl_get_error_stringified);
#else
	int gl_version = gladLoadGL((GLADloadfunc) SDL_GL_GetProcAddress);
	assert_or_cleanup(gl_version != 0, "Failed to load OpenGL functions", NULL);
	write_log("OpenGL version %d.%d\n", GLAD_VERSION_MAJOR(gl_version), GLAD_VERSION_MINOR(gl_version));

	#ifdef DEBUG
		if (!have_gl_debug_output(gl_version)) {
			write_log("GL_DEBUG_OUTPUT unavailable\n");
		} else {
			write_log("Enabling GL_DEBUG_OUTPUT\n");
			glEnable(GL_DEBUG_OUTPUT);
			glDebugMessageCallback(gl_debug_message_callback, NULL);
		}
	#endif
#endif

	glGenVertexArrays(1, &g_draw_vao);
	glBindVertexArray(g_draw_vao);

	// Ensure buffers that won't immediately be overwritten are set to zero
	GLfloat zeroes[4 * MAX_PLANETS];
	for (int i = 0; i < sizeof(zeroes) / sizeof(zeroes[0]); ++i) {
		zeroes[i] = 0.0;
	}

	glGenBuffers(1, &g_circle_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, g_circle_vbo);
		glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(float) * (CIRCLE_SIDES + 2), NULL, GL_STATIC_DRAW);

	glGenBuffers(1, &g_colour_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, g_colour_vbo);
		glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(GLfloat) * MAX_PLANETS, zeroes, GL_DYNAMIC_DRAW);

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
	// WebGL exhibits undefined behaviour if a buffer in use is also bound as TFBO
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

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
		glUniform1f(glGetUniformLocation(g_draw_program, "planet_r"), POINT_RADIUS);

		GLint in_vertex = glGetAttribLocation(g_draw_program, "vert_displacement");
		GLint in_color_draw = glGetAttribLocation(g_draw_program, "color");

		glEnableVertexAttribArray(in_vertex);
		glEnableVertexAttribArray(in_color_draw);
		glVertexAttribDivisor(in_color_draw, 1);

		glBindBuffer(GL_ARRAY_BUFFER, g_circle_vbo);
			glVertexAttribPointer(in_vertex, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, g_colour_vbo);
			glVertexAttribPointer(in_color_draw, 4, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint motion_shaders[2]; // vertex, fragment

	motion_shaders[0] = load_shader("shaders/line.vert", GL_VERTEX_SHADER);
	assert_or_cleanup(motion_shaders[0] != 0, "Failed to load motion resolution vertex shader", NULL);

	motion_shaders[1] = load_shader("shaders/resolve_motion.frag", GL_FRAGMENT_SHADER);
	assert_or_cleanup(motion_shaders[1] != 0, "Failed to load motion resolution fragment shader", NULL);

	char *outs_update = "out_position";
	g_motion_program = create_shader_program(2, motion_shaders, 1, &outs_update, 0, NULL);
	assert_or_cleanup(g_motion_program != 0, "Failed to create update shader program", gl_get_error_stringified);

	glUseProgram(g_motion_program);
		glUniform1i(glGetUniformLocation(g_motion_program, "positions"), POSITION_TEX_UNIT_OFFSET);
		glUniform1i(glGetUniformLocation(g_motion_program, "attractions"), ATTRACTION_TEX_UNIT_OFFSET);

	// Flat n * 1 texture of all planet positions
	glGenTextures(2, g_motion_texture);
	glGenFramebuffers(2, g_motion_framebuffer);

	for (int i = 0; i < 2; ++i) {
		glBindTexture(GL_TEXTURE_2D, g_motion_texture[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, MAX_PLANETS, 1, 0, GL_RGBA, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glBindFramebuffer(GL_FRAMEBUFFER, g_motion_framebuffer[i]);
		glViewport(0, 0, MAX_PLANETS, 1);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_motion_texture[i], 0);
			assert_or_cleanup(
				glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
				"Planet position framebuffer incomplete",
				gl_get_error_stringified
			);
			glClearColor(0.0, 0.0, 0.0, 0.0);
			glClear(GL_COLOR_BUFFER_BIT);
	}

	GLuint intersection_shaders[2]; // vertex, fragment

	intersection_shaders[0] = load_shader("shaders/quad.vert", GL_VERTEX_SHADER);
	assert_or_cleanup(intersection_shaders[0] != 0, "Failed to load quad.vert", NULL);

	intersection_shaders[1] = load_shader("shaders/resolve_intersections.frag", GL_FRAGMENT_SHADER);
	assert_or_cleanup(intersection_shaders[1] != 0, "Failed to load resolve_intersections.frag", NULL);

	char *intersection_out = "out_impulse";
	g_intersection_program = create_shader_program(2, intersection_shaders, 1, &intersection_out, 0, NULL);
	assert_or_cleanup(g_intersection_program != 0, "Failed to link quad.vert and resolve_intersections.frag", gl_get_error_stringified);

	glUseProgram(g_intersection_program);
		glUniform1i(glGetUniformLocation(g_intersection_program, "positions"), POSITION_TEX_UNIT_OFFSET);
		glUniform1f(glGetUniformLocation(g_intersection_program, "planet_r"), POINT_RADIUS);

	GLuint attraction_shaders[2]; // vertex, fragment

	attraction_shaders[0] = load_shader("shaders/quad.vert", GL_VERTEX_SHADER);
	assert_or_cleanup(attraction_shaders[0] != 0, "Failed to load quad.vert", NULL);

	attraction_shaders[1] = load_shader("shaders/calc_particle_attractions.frag", GL_FRAGMENT_SHADER);
	assert_or_cleanup(attraction_shaders[1] != 0, "Failed to load calc_particle_attractions.frag", NULL);

	char *attraction_out = "out_attraction";
	g_attraction_program = create_shader_program(2, attraction_shaders, 1, &attraction_out, 0, NULL);
	assert_or_cleanup(g_attraction_program != 0, "Failed to link quad.vert and calc_particle_attractions.frag", gl_get_error_stringified);

	glUseProgram(g_attraction_program);
		glUniform1i(glGetUniformLocation(g_attraction_program, "positions"), POSITION_TEX_UNIT_OFFSET);
		glUniform1f(glGetUniformLocation(g_attraction_program, "planet_r"), POINT_RADIUS);

	// n * n array of all planet pairs, with second for double-buffered summing
	glGenTextures(2, g_impulse_texture);
	glGenFramebuffers(2, g_impulse_framebuffer);

	for (int i = 0; i < 2; ++i) {
		glBindTexture(GL_TEXTURE_2D, g_impulse_texture[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, MAX_PLANETS, MAX_PLANETS, 0, GL_RG, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glBindFramebuffer(GL_FRAMEBUFFER, g_impulse_framebuffer[i]);
		glViewport(0, 0, MAX_PLANETS, MAX_PLANETS);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_impulse_texture[i], 0);
			assert_or_cleanup(
				glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
				"Attraction matrix framebuffer incomplete",
				gl_get_error_stringified
			);
			glClearColor(0.0, 0.0, 0.0, 0.0);
			glClear(GL_COLOR_BUFFER_BIT);
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

	create_planet(0.0, 0.0, 0.0, 0.0, 0.0, 0.8, 0.2);

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
			write_log("%2.2f FPS\n", (1000.0 * (float) FPS_CAP) / ((float) recent_total));
		}
		frame_number = (frame_number + 1) % FPS_CAP;
#endif
		if (f2_start > f1_end) {
			SDL_Delay(f2_start - f1_end);
		}
	}
#endif

	cleanup_and_quit(EXIT_SUCCESS);
	return EXIT_SUCCESS; // Unreachable but keeps compilers happy
}
