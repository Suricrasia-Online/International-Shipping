#define GL_GLEXT_PROTOTYPES why

#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<stdint.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include <math.h>
#include <complex.h>
#include <fftw3.h>

#include "floats.h"

#include "shader.h"
const char* vshader = "#version 450\nvec2 y=vec2(1.,-1);\nvec4 x[4]={y.yyxx,y.xyxx,y.yxxx,y.xxxx};void main(){gl_Position=x[gl_VertexID];}";

#define CANVAS_WIDTH 1920
#define CANVAS_HEIGHT 1080
#define SCANLINE_SIZE 10
#define WAVE_SAMPLES 1024
#define DEBUG

static gboolean check_escape(GtkWidget *widget, GdkEventKey *event)
{
	(void)widget;
	if (event->keyval == GDK_KEY_Escape) {
	asm volatile(".intel_syntax noprefix");
	asm volatile("push 231"); //exit_group
	asm volatile("pop rax");
	asm volatile("xor edi, edi");
	asm volatile("syscall");
	asm volatile(".att_syntax prefix");
	__builtin_unreachable();
		// gtk_main_quit();
	}
	return FALSE;
}

// uint32_t randomstate = 0x6b873edd;
// uint32_t randomstate = 0x31debeab;
uint32_t randomstate = 0x96fcd33;
float rand_float() {
	randomstate = randomstate ^ (randomstate << 13u);
	randomstate = randomstate ^ (randomstate >> 17u);
	randomstate = randomstate ^ (randomstate << 5u);
	randomstate *= 1685821657u;
	uint32_t intermediate = ( (randomstate & 0x007FFFFFu) | 0x3F800000u );
	return *((float*)(&intermediate)) - 1.0;
}

// I should've made this a sum of 12 random floats, subtracted by 6, but it's too late now :c
float rand_gauss() {
	float a, b, W;
	do {
		a = (rand_float()-0.5)*2.0;
		b = (rand_float()-0.5)*2.0;
		W = a*a+b*b;
	} while (W >= 1.0);
	return a * sqrt ((-2.0 * log (W)) / W);
	// return a * mult;
}

GLuint vao;
GLuint p;
GLuint waveTex;

fftwf_complex wavedata_in[WAVE_SAMPLES][WAVE_SAMPLES] __attribute__ ((__aligned__(16))) = {0};
fftwf_complex wavedata_out[WAVE_SAMPLES][WAVE_SAMPLES] __attribute__ ((__aligned__(16))) = {0};

bool rendered = false;
bool flipped = false;

float phillips_spectrum(float x, float y) {
	float scale = 200.0; //m? reciprocal meters?
	x *= scale; y *= scale; //m???
	float k = x*x+y*y; //m...??
	// float windspeed = 3.5; //m/s
	// float gravity = 9.8; //m/s^2
	float len = 1.25; //m
	// float windx = 0.0; //m/s
	// float windy = 1.0; //m/s
	// float dot = windx*x + windy*y; //m^2/s??
	if (k > WAVE_SAMPLES/2) return 0.0;
	return 0.1*exp(-1.0/(k*len*len))/(k*k) * pow(y,p1d40);
}

static gboolean
on_render (GtkGLArea *glarea, GdkGLContext *context)
{
	(void)context;
	if (rendered || gtk_widget_get_allocated_width((GtkWidget*)glarea) != CANVAS_WIDTH) return TRUE;
	if (!flipped) { gtk_gl_area_queue_render(glarea); flipped = true; return TRUE; }

	rendered = true;
	glUseProgram(p);
	glBindVertexArray(vao);
	glVertexAttrib1f(0, 0);
	glUniform1i(0, 0);
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, waveTex);

	for (int i = 0; i < WAVE_SAMPLES/2; i++) {
		for (int j = 0; j < WAVE_SAMPLES/2; j++) {
			// if (i > 500 || j > 500) break;
			float x = (float)i/WAVE_SAMPLES;
			float y = (float)j/WAVE_SAMPLES;
			// Simulating Ocean Water - Jerry Tessendorf
			wavedata_in[i][j] = M_SQRT1_2*(rand_gauss() + rand_gauss()*I)*phillips_spectrum(x, y);
		}
	}

	fftwf_plan plan = fftwf_plan_dft_2d(WAVE_SAMPLES, WAVE_SAMPLES, (fftwf_complex*)wavedata_in, (fftwf_complex*)wavedata_out, FFTW_BACKWARD, FFTW_MEASURE);
	fftwf_execute(plan);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, WAVE_SAMPLES, WAVE_SAMPLES, 0, GL_RG, GL_FLOAT, wavedata_out);

  glEnable(GL_SCISSOR_TEST);
  for (int i = 0; i < CANVAS_HEIGHT; i += SCANLINE_SIZE) {
	  glScissor(0,i,1920,SCANLINE_SIZE);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glFinish();
		while (gtk_events_pending()) gtk_main_iteration();
  }


	return TRUE;
}

static void on_realize(GtkGLArea *glarea)
{
	gtk_gl_area_make_current(glarea);

	// compile shader
	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);

	//let ppl pass the number of samples they want into the demo
	char* samples = getenv("SAMPLES");
	if (samples == NULL) samples = DEFAULT_SAMPLES;
	char defines[512];
	sprintf(defines, "#version 420\n#define SAMPLES %s\n", samples);

	const char* shader_frag_list[] = {defines, shader_frag_min};
	glShaderSource(f, 2, shader_frag_list, NULL);
	glCompileShader(f);

	#ifdef DEBUG
		GLint isCompiled = 0;
		glGetShaderiv(f, GL_COMPILE_STATUS, &isCompiled);
		if(isCompiled == GL_FALSE) {
			GLint maxLength = 0;
			glGetShaderiv(f, GL_INFO_LOG_LENGTH, &maxLength);

			char* error = malloc(maxLength);
			glGetShaderInfoLog(f, maxLength, &maxLength, error);
			printf("%s\n", error);

			exit(-10);
		}
	#endif

	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v, 1, &vshader, NULL);
	glCompileShader(v);

	#ifdef DEBUG
		GLint isCompiled2 = 0;
		glGetShaderiv(v, GL_COMPILE_STATUS, &isCompiled2);
		if(isCompiled2 == GL_FALSE) {
			GLint maxLength = 0;
			glGetShaderiv(v, GL_INFO_LOG_LENGTH, &maxLength);

			char* error = malloc(maxLength);
			glGetShaderInfoLog(v, maxLength, &maxLength, error);
			printf("%s\n", error);

			exit(-10);
		}
	#endif

	// link shader
	p = glCreateProgram();
	glAttachShader(p,v);
	glAttachShader(p,f);
	glLinkProgram(p);

	#ifdef DEBUG
		GLint isLinked = 0;
		glGetProgramiv(p, GL_LINK_STATUS, (int *)&isLinked);
		if (isLinked == GL_FALSE) {
			GLint maxLength = 0;
			glGetProgramiv(p, GL_INFO_LOG_LENGTH, &maxLength);

			char* error = malloc(maxLength);
			glGetProgramInfoLog(p, maxLength, &maxLength,error);
			printf("%s\n", error);

			exit(-10);
		}
	#endif

	glGenVertexArrays(1, &vao);

  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &waveTex);
  glBindTexture(GL_TEXTURE_2D, waveTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void _start() {
	asm volatile("sub $8, %rsp\n");

#ifdef GEN_BOATS
	srand(time(0));
	srand(rand());
	randomstate = rand();
	randomstate = rand();
	printf("%p\n", randomstate);
#endif

	typedef void (*voidWithOneParam)(int*);
	voidWithOneParam gtk_init_one_param = (voidWithOneParam)gtk_init;
	(*gtk_init_one_param)(NULL);

	GtkWidget *win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *glarea = gtk_gl_area_new();
	gtk_container_add(GTK_CONTAINER(win), glarea);

	g_signal_connect(win, "destroy", gtk_main_quit, NULL);
	g_signal_connect(win, "key_press_event", G_CALLBACK(check_escape), NULL);
	g_signal_connect(glarea, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(glarea, "render", G_CALLBACK(on_render), NULL);

	gtk_widget_show_all (win);

	gtk_window_fullscreen((GtkWindow*)win);
	GdkWindow* window = gtk_widget_get_window(win);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	GdkCursor* Cursor = gdk_cursor_new(GDK_BLANK_CURSOR);
#pragma GCC diagnostic pop
	gdk_window_set_cursor(window, Cursor);

	gtk_main();

	asm volatile(".intel_syntax noprefix");
	asm volatile("push 231"); //exit_group
	asm volatile("pop rax");
	// asm volatile("xor edi, edi");
	asm volatile("syscall");
	asm volatile(".att_syntax prefix");
	__builtin_unreachable();
	// return 0;
}