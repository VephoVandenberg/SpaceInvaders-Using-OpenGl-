#define GLEW_STATIC

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdint.h>

#define i8  int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define WIDTH  224
#define HEIGHT 256

#define NUMBER_OF_BULLETS 150

const char *vertex_shader_source = "#version 330 core\n"
    "layout(location=0) in vec2 coordinate_input;\n"
    "layout(location=1) in vec2 texture_input;\n"
    "out vec2 texture_out;\n"
    "void main(void)\n"
    "{\n"
    "   gl_Position = vec4(coordinate_input, 0.0f, 1.0f);\n"
    "   texture_out = texture_input;\n"
    "}\n";

const char *fragment_shader_source = "#version 330 core\n"
    "in vec2 texture_out;\n"
    "out vec4 fragment_color;\n"
    "uniform sampler2D u_texture;\n"
    "void main(void)\n"
    "{\n"
    "   fragment_color = texture(u_texture, texture_out);\n"
    "}\n";

enum alien_type: u8
{
    ALIEN_DEAD = 0,
    TYPE_A = 1,
    TYPE_B = 2,
    TYPE_C = 3
};

struct buffer
{
    u32 width, height;
    u32 *data;
};

struct sprite
{
    u32 width, height;
    u8 *data;
};

struct alien
{
    u32 x, y;
    u8 type;
};

struct player
{
    u32 x, y;
    u32 life;
};

struct bullet
{
    u32 x, y;
    u32 dir;
};

struct game
{
    u32 width, height;
    u32 number_of_aliens;
    u32 number_of_bullets;
    alien *aliens;
    player main_player;
    bullet bullets[NUMBER_OF_BULLETS];
};

struct sprite_animation
{
    bool loop;
    u32 number_of_frames;
    u32 frame_duration;
    u32 time;
    sprite **frames;
};

void framebuffer_size_callback(GLFWwindow *window,
			       int width, int height);
void clear_buffer(buffer *screen_buffer, u32 color);
void draw_sprite(buffer *screen_buffer, sprite *sprite_buffer,
		 i32 x, i32 y,
		 u32 color);
u32 color_to_u32(u32 red, u32 green, u32 blue);
void key_callback(GLFWwindow *window,
		  int key, int scancode, int action, int mods);
bool sprite_overlap_check(sprite* sp_a, u32 xa, u32 ya,
			  sprite *sp_b, u32 xb, u32 yb);
void buffer_draw_text(buffer *screen_bufffer,
		      sprite text_spritesheet,
		      const char *text,
		      u32 x, u32 y,
		      u32 color);
void buffer_draw_number(buffer *screen_buffer,
			sprite number_spritesheet,
			u32 number,
			u32 x, u32 y,
			u32 color);

bool game_running = true;
bool fire_pressed = false;
int move_dir = 0;
u32 score = 0;
u32 credits = 0;

int main(int argc, char **argv)
{
    if (!glfwInit())
    {
	std::cout << "Could not initialize glfw" << std::endl;
    }

    
    GLFWwindow *window = glfwCreateWindow(2*WIDTH,
					  2*HEIGHT,
					  "FUCKING SPACE INVADERS",
					  0,
					  0);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);    
    if (glewInit() != GLEW_OK)
    {
	std::cout << "Could not initialize glew" << std::endl;
    }

    // Creating a shader program

    u32 vertex_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_id, 1, &vertex_shader_source, 0);
    glCompileShader(vertex_id);

    u32 fragment_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_id, 1, &fragment_shader_source, 0);
    glCompileShader(fragment_id);

    u32 shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_id);
    glAttachShader(shader_program, fragment_id);
    glLinkProgram(shader_program);

    glDeleteShader(vertex_id);
    glDeleteShader(fragment_id);        

    // Creating an 'area' where will be drawn a game
    float screen_vertices[16] =
    {
	// vertices    // texture coords
	-1.0f, -1.0f,  0.0f, 0.0f, // bottom left
	 1.0f, -1.0f,  1.0f, 0.0f, // bottom right 
	 1.0f,  1.0f,  1.0f, 1.0f, // top right
	-1.0f,  1.0f,  0.0f, 1.0f  // top left  
    };

    u32 indicies[6] =
    {
	0, 1, 3,
	1, 2, 3
    };

    u32  vao, vbo, ebo;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), screen_vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicies), indicies, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    
    u32 screen_color = color_to_u32(0, 0, 0);
    buffer screen_buffer;
    screen_buffer.width = WIDTH;
    screen_buffer.height = HEIGHT;
    screen_buffer.data = new u32[screen_buffer.width*screen_buffer.height];
    clear_buffer(&screen_buffer, screen_color);

    sprite bullet_sprite;
    bullet_sprite.width = 1;
    bullet_sprite.height = 3;
    bullet_sprite.data = new u8[bullet_sprite.height]
    {
	    1,
	    1,
	    1
    };

    sprite text_spritesheet;
    text_spritesheet.width = 5;
    text_spritesheet.height = 7;
    text_spritesheet.data = new uint8_t[65*35]
    {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,
        0,1,0,1,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,1,0,1,0,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,0,1,0,1,0,
        0,0,1,0,0,0,1,1,1,0,1,0,1,0,0,0,1,1,1,0,0,0,1,0,1,0,1,1,1,0,0,0,1,0,0,
        1,1,0,1,0,1,1,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,1,1,0,1,0,1,1,
        0,1,1,0,0,1,0,0,1,0,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,1,0,0,0,1,0,1,1,1,1,
        0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,
        1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,
        0,0,1,0,0,1,0,1,0,1,0,1,1,1,0,0,0,1,0,0,0,1,1,1,0,1,0,1,0,1,0,0,1,0,0,
        0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,1,1,1,1,1,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,
        0,0,0,1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,

        0,1,1,1,0,1,0,0,0,1,1,0,0,1,1,1,0,1,0,1,1,1,0,0,1,1,0,0,0,1,0,1,1,1,0,
        0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,1,1,0,
        0,1,1,1,0,1,0,0,0,1,0,0,0,0,1,0,0,1,1,0,0,1,0,0,0,1,0,0,0,0,1,1,1,1,1,
        1,1,1,1,1,0,0,0,0,1,0,0,0,1,0,0,0,1,1,0,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        0,0,0,1,0,0,0,1,1,0,0,1,0,1,0,1,0,0,1,0,1,1,1,1,1,0,0,0,1,0,0,0,0,1,0,
        1,1,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,0,1,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        1,1,1,1,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,1,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,

        0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,
        0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,
        0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,
        1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,
        0,1,1,1,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,
        0,1,1,1,0,1,0,0,0,1,1,0,1,0,1,1,1,0,1,1,1,0,1,0,0,1,0,0,0,1,0,1,1,1,0,

        0,0,1,0,0,0,1,0,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,1,1,0,0,0,1,
        1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,1,1,1,0,
        1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,
        1,1,1,1,1,1,0,0,0,0,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,1,0,0,0,0,1,1,1,1,1,
        1,1,1,1,1,1,0,0,0,0,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,0,1,0,1,1,1,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,
        0,1,1,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,1,1,0,
        0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        1,0,0,0,1,1,0,0,1,0,1,0,1,0,0,1,1,0,0,0,1,0,1,0,0,1,0,0,1,0,1,0,0,0,1,
        1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,1,1,1,1,
        1,0,0,0,1,1,1,0,1,1,1,0,1,0,1,1,0,1,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,
        1,0,0,0,1,1,0,0,0,1,1,1,0,0,1,1,0,1,0,1,1,0,0,1,1,1,0,0,0,1,1,0,0,0,1,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,1,0,1,1,0,0,1,1,0,1,1,1,1,
        1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,1,0,1,0,0,1,0,0,1,0,1,0,0,0,1,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,0,0,1,1,1,0,1,0,0,0,1,0,0,0,0,1,0,1,1,1,0,
        1,1,1,1,1,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,
        1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,1,0,1,0,0,0,1,0,0,
        1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,1,0,1,1,0,1,0,1,1,1,0,1,1,1,0,0,0,1,
        1,0,0,0,1,1,0,0,0,1,0,1,0,1,0,0,0,1,0,0,0,1,0,1,0,1,0,0,0,1,1,0,0,0,1,
        1,0,0,0,1,1,0,0,0,1,0,1,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,
        1,1,1,1,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1,1,1,1,1,

        0,0,0,1,1,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1,1,
        0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0,
        1,1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,1,1,0,0,0,
        0,0,1,0,0,0,1,0,1,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,
        0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };

    sprite number_spritesheet = text_spritesheet;
    number_spritesheet.data += 16 * 35;    

    sprite alien_sprites[6];

    alien_sprites[0].width = 8;
    alien_sprites[0].height = 8;
    alien_sprites[0].data = new u8[64]
    {
        0,0,0,1,1,0,0,0, // ...@@...
	0,0,1,1,1,1,0,0, // ..@@@@..
	0,1,1,1,1,1,1,0, // .@@@@@@.
	1,1,0,1,1,0,1,1, // @@.@@.@@
	1,1,1,1,1,1,1,1, // @@@@@@@@
	0,1,0,1,1,0,1,0, // .@.@@.@.
	1,0,0,0,0,0,0,1, // @......@
	0,1,0,0,0,0,1,0  // .@....@.
    };
    
    alien_sprites[1].width = 8;
    alien_sprites[1].height = 8;
    alien_sprites[1].data = new u8[64]
    {
        0,0,0,1,1,0,0,0, // ...@@...
	0,0,1,1,1,1,0,0, // ..@@@@..
	0,1,1,1,1,1,1,0, // .@@@@@@.
	1,1,0,1,1,0,1,1, // @@.@@.@@
	1,1,1,1,1,1,1,1, // @@@@@@@@
	0,0,1,0,0,1,0,0, // ..@..@..
	0,1,0,1,1,0,1,0, // .@.@@.@.
	1,0,1,0,0,1,0,1  // @.@..@.@
    };

    alien_sprites[2].width = 11;
    alien_sprites[2].height = 8;
    alien_sprites[2].data = new u8[88]
    {
	   0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
	    0,0,0,1,0,0,0,1,0,0,0, // ...@...@...
	    0,0,1,1,1,1,1,1,1,0,0, // ..@@@@@@@..
	    0,1,1,0,1,1,1,0,1,1,0, // .@@.@@@.@@.
	    1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
	    1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
	    1,0,1,0,0,0,0,0,1,0,1, // @.@.....@.@
	    0,0,0,1,1,0,1,1,0,0,0  // ...@@.@@...
    };

    alien_sprites[3].width = 11;
    alien_sprites[3].height = 8;
    alien_sprites[3].data = new u8[88]
    {
	    0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
	    1,0,0,1,0,0,0,1,0,0,1, // @..@...@..@
	    1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
	    1,1,1,0,1,1,1,0,1,1,1, // @@@.@@@.@@@
	    1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
	    0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
	    0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
	    0,1,0,0,0,0,0,0,0,1,0  // .@.......@.
    };

    alien_sprites[4].width = 12;
    alien_sprites[4].height = 8;
    alien_sprites[4].data = new u8[96]
    {
	    0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
	    0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
	    1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
	    1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
	    1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
	    0,0,0,1,1,0,0,1,1,0,0,0, // ...@@..@@...
	    0,0,1,1,0,1,1,0,1,1,0,0, // ..@@.@@.@@..
	    1,1,0,0,0,0,0,0,0,0,1,1  // @@........@@
    };


    alien_sprites[5].width = 12;
    alien_sprites[5].height = 8;
    alien_sprites[5].data = new u8[96]
    {
	    0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
	    0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
	    1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
	    1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
	    1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
	    0,0,1,1,1,0,0,1,1,1,0,0, // ..@@@..@@@..
	    0,1,1,0,0,1,1,0,0,1,1,0, // .@@..@@..@@.
	    0,0,1,1,0,0,0,0,1,1,0,0  // ..@@....@@..
    };

    sprite alien_death_sprite;
    alien_death_sprite.width = 13;
    alien_death_sprite.height = 7;
    alien_death_sprite.data = new u8[91]
    {
	    0,1,0,0,1,0,0,0,1,0,0,1,0, // .@..@...@..@.
	    0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
	    0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
	    1,1,0,0,0,0,0,0,0,0,0,1,1, // @@.........@@
	    0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
	    0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
	    0,1,0,0,1,0,0,0,1,0,0,1,0  // .@..@...@..@.
    };

    sprite player_sprite;
    player_sprite.width = 11;
    player_sprite.height = 7;
    player_sprite.data = new u8[player_sprite.width*player_sprite.height]
    {
            0,0,0,0,0,1,0,0,0,0,0, // .....@.....
	    0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
	    0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
	    0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
	    1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
	    1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
	    1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
    };

    sprite_animation alien_animation[3];

    for (u8 i = 0; i < 3; i++)
    {
	alien_animation[i].loop = true;
	alien_animation[i].number_of_frames = 2;
	alien_animation[i].frame_duration = 10;
	alien_animation[i].time = 0;

	alien_animation[i].frames = new sprite*[2];
	alien_animation[i].frames[0] = &alien_sprites[2*i];
	alien_animation[i].frames[1] = &alien_sprites[2*i + 1];
	
    }
    
    // Setting up game
    game game_entity;
    game_entity.width = WIDTH;
    game_entity.height = HEIGHT;
    game_entity.number_of_aliens = 55;
    game_entity.number_of_bullets = 0;
    game_entity.aliens = new alien[game_entity.number_of_aliens];    

    game_entity.main_player.x = 112 - 5;
    game_entity.main_player.y = 32;

    game_entity.main_player.life = 3;

    for (u32 yi = 0; yi < 5; yi++)
    {
	for (u32 xi = 0; xi < 11; xi++)
	{
	    alien *temp_alien = &game_entity.aliens[yi*11 + xi];
	    temp_alien->type = (5 - yi) / 2 + 1;

	    sprite temp_sprite = alien_sprites[2*(temp_alien->type - 1)];
	    
	    game_entity.aliens[yi*11 + xi].x = 16*xi + 20 + (alien_death_sprite.width - temp_sprite.width)/2;
	    game_entity.aliens[yi*11 + xi].y = 17*yi + 128;	   	    
	}
    }

    u8 *death_counters = new u8[game_entity.number_of_aliens];
    for (u32 i = 0; i < game_entity.number_of_aliens; i++)
    {
	death_counters[i] = 10;
    }
    
    u32 background_texture;
    glGenTextures(1, &background_texture);
    glBindTexture(GL_TEXTURE_2D, background_texture);
    glTexImage2D(GL_TEXTURE_2D, 0,
		 GL_RGB8,
		 screen_buffer.width, screen_buffer.height,
		 0,
		 GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
		 screen_buffer.data);    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glUseProgram(shader_program);
    u32 location = glGetUniformLocation(shader_program, "u_texture");
    glUniform1i(location, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);   

    i32 player_move_dir = 0;
    while (!glfwWindowShouldClose(window) && game_running)
    {
	clear_buffer(&screen_buffer, screen_color);

	// Score

	buffer_draw_text(&screen_buffer, text_spritesheet, "SCORE",
			 4, game_entity.height - text_spritesheet.height - 7,
			 color_to_u32(255, 255, 255));

	buffer_draw_number(&screen_buffer, number_spritesheet, score,
			   4 + 2*number_spritesheet.width, game_entity.height - 2*number_spritesheet.height - 12,
			   color_to_u32(255, 255, 255));
	
	
	char credit_text[16];
	sprintf(credit_text, "CREDIT %021u", credits);
	buffer_draw_text(&screen_buffer, text_spritesheet, credit_text,
			 164, 7,
			 color_to_u32(255, 255, 255));
	
	// Draw
	for (u32 ai = 0; ai < game_entity.number_of_aliens; ai++)
	{
	    if (!death_counters[ai])
	    {
		continue;
	    }
	    
	    alien temp_alien = game_entity.aliens[ai];
	    if (temp_alien.type == ALIEN_DEAD)
	    {
		draw_sprite(&screen_buffer, &alien_death_sprite,
			    temp_alien.x, temp_alien.y,
			    color_to_u32(255, 255, 0));
	    }
	    else
	    {
		sprite_animation animation = alien_animation[temp_alien.type - 1];
		u32 current_frame = animation.time / animation.frame_duration;
		sprite *temp_sprite = animation.frames[current_frame];
		draw_sprite(&screen_buffer, temp_sprite,
			    temp_alien.x, temp_alien.y,
			    color_to_u32(255, 0, 0));
	    }
	}

	for (u32 bi = 0; bi < game_entity.number_of_bullets; bi++)
	{
	    bullet temp_bullet = game_entity.bullets[bi];
	    draw_sprite(&screen_buffer, &bullet_sprite,
			temp_bullet.x, temp_bullet.y,
			color_to_u32(255, 255, 128));
	}

	draw_sprite(&screen_buffer, &player_sprite,
		    game_entity.main_player.x, game_entity.main_player.y,
		    color_to_u32(0, 255, 0));

	// update animations

	for (u32 i = 0; i < 3; i++)
	{
	    alien_animation[i].time++;
	    if (alien_animation[i].time == alien_animation[i].number_of_frames*alien_animation[i].frame_duration)
	    {
		alien_animation[i].time = 0;
	    }
	}              

	glTexSubImage2D(GL_TEXTURE_2D,
			0, 0, 0,
			screen_buffer.width, screen_buffer.height,
			GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
			screen_buffer.data);	
	
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glfwSwapBuffers(window);

	// Dead aliens handling
	
	for (u32 ai = 0; ai < game_entity.number_of_aliens; ai++)
	{
	    alien temp_alien = game_entity.aliens[ai];
	    if (temp_alien.type == ALIEN_DEAD && death_counters[ai])
	    {
		death_counters[ai]--;
	    }
	}

	// Bullets handling
	for (u32 bi = 0; bi < game_entity.number_of_bullets;)
	{
	    game_entity.bullets[bi].y += game_entity.bullets[bi].dir;
	    if (game_entity.bullets[bi].y >= game_entity.height ||
		game_entity.bullets[bi].y < bullet_sprite.height)
	    {
		game_entity.bullets[bi] = game_entity.bullets[game_entity.number_of_bullets - 1];
		game_entity.number_of_bullets--;
		continue;
	    }

	    // Check hit
	    for (u32 ai = 0; ai < game_entity.number_of_aliens;  ai++)
	    {
		alien temp_alien = game_entity.aliens[ai];
		if (temp_alien.type == ALIEN_DEAD)
		{
		    continue;
		}

		sprite_animation animation = alien_animation[temp_alien.type - 1];
		u32 current_frame = animation.time/animation.frame_duration;
		sprite *temp_sprite = animation.frames[current_frame];
		bool overlap = sprite_overlap_check(&bullet_sprite, game_entity.bullets[bi].x, game_entity.bullets[bi].y,
						    temp_sprite, temp_alien.x, temp_alien.y);
		if (overlap)
		{
		    game_entity.aliens[ai].type = ALIEN_DEAD;
		    game_entity.aliens[ai].x -= (alien_death_sprite.width - temp_sprite->width)/2;
		    game_entity.bullets[bi] = game_entity.bullets[game_entity.number_of_bullets - 1];
		    game_entity.number_of_bullets--;
		    score += 10*(4-game_entity.aliens[ai].type);
		    continue;
		}
		
	    }

	    bi++;
	}

	// Player handling
	int player_move_dir = 2*move_dir;
	if (player_move_dir)
	{
	    if (game_entity.main_player.x + player_sprite.width + player_move_dir >= game_entity.width)
	    {
		game_entity.main_player.x = game_entity.width - player_sprite.width;
	    }
	    else if ((int)game_entity.main_player.x + player_move_dir <= 0)
	    {
		game_entity.main_player.x = 0;
	    }
	    else
	    {
		game_entity.main_player.x += player_move_dir;
	    }
	}

	// Process events
	if (fire_pressed && (game_entity.number_of_bullets < NUMBER_OF_BULLETS))
	{
	    game_entity.bullets[game_entity.number_of_bullets].x = game_entity.main_player.x + player_sprite.width/2;
	    game_entity.bullets[game_entity.number_of_bullets].y = game_entity.main_player.y + player_sprite.height;
	    game_entity.bullets[game_entity.number_of_bullets].dir = 2;
	    game_entity.number_of_bullets++;
	}
	
	fire_pressed = false;

	      
	glfwPollEvents();
    }
    return 0;
}

void clear_buffer(buffer *screen_buffer, u32 color)
{
    for (u32 color_iterator = 0; color_iterator < screen_buffer->width*screen_buffer->height; color_iterator++)
    {
	screen_buffer->data[color_iterator] = color;
    }
}

u32 color_to_u32(u32 red, u32 green, u32 blue)
{
    return ((red << 24) | (green << 16) | (blue << 8)) | 255;
}

void framebuffer_size_callback(GLFWwindow *window,
			       int width, int height)
{
    glViewport(0, 0, width, height);
}

void draw_sprite(buffer *screen_buffer, sprite *sprite_buffer,
		 i32 x, i32 y, u32 color)
{
    for (u32 yi = 0; yi < sprite_buffer->height; yi++)
    {
	for (u32 xi = 0; xi < sprite_buffer->width; xi++)
	{
	    u32 position_y = sprite_buffer->height - 1 + y - yi;
	    u32 position_x = x + xi;
	    if (sprite_buffer->data[yi*sprite_buffer->width + xi] &&
		position_x < screen_buffer->width && position_y < screen_buffer->height)
	    {
		screen_buffer->data[position_y*screen_buffer->width + position_x] = color;
	    }
	}
    }
}

bool sprite_overlap_check(sprite* sp_a, u32 xa, u32 ya,
			  sprite *sp_b, u32 xb, u32 yb)
{
    if (xa < xb + sp_b->width && xb < xa + sp_a->width &&
	ya < yb + sp_b->height && yb < ya + sp_a->height)
    {
	return true;
    }

    return false;
}

void key_callback(GLFWwindow *window,
		  int key, int scancode, int action, int mods)
{
    switch(key)
    {
	case GLFW_KEY_ESCAPE:
	{
	    if (action == GLFW_PRESS)
	    {
		game_running = false;
	    }
	}break;

	case GLFW_KEY_RIGHT:
	{
	    if (action == GLFW_PRESS)
	    {
		move_dir += 1;
	    }
	    else if (action == GLFW_RELEASE)
	    {
		move_dir -= 1;
	    }		
	}break;

	case GLFW_KEY_LEFT:
	{
	    if (action == GLFW_PRESS)
	    {
		move_dir -= 1;		
	    }
	    else if (action == GLFW_RELEASE)
	    {
		move_dir += 1;
	    }
	}break;

	case GLFW_KEY_SPACE:
	{
	    if (action == GLFW_RELEASE)
	    {
		fire_pressed = true;
	    }
	}break;
	
	default:
	{
	    
	}break;
    }
}

void buffer_draw_text(buffer *screen_buffer,
		      sprite text_spritesheet,
		      const char *text,
		      u32 x, u32 y,
		      u32 color)
{
    u32 xp = x;
    u32 stride = text_spritesheet.width*text_spritesheet.height;

    sprite temp_spritesheet = text_spritesheet;
    for (const char *char_pointer = text; *char_pointer; char_pointer++)
    {
	char character = *char_pointer - 32;
	if (character < 0 || character >= 65)
	{
	    continue;	    
	}

	temp_spritesheet.data = text_spritesheet.data + character*stride;
	draw_sprite(screen_buffer, &temp_spritesheet, xp, y, color);
	xp += text_spritesheet.width + 1;
    }
}

void buffer_draw_number(buffer *screen_buffer,
			sprite number_spritesheet,
			u32 number,
			u32 x, u32 y,
			u32 color)
{
    u8 digits[64];
    u32 number_of_digits = 0;

    u32 current_number = number;

    do
    {
	digits[number_of_digits++] = current_number%10;
	current_number /= 10;
    }
    while(current_number > 0);

    u32 xp = x;
    u32 stride = number_spritesheet.width*  number_spritesheet.height;
    sprite temp_spritesheet = number_spritesheet;
    for (u32 i = 0; i < number_of_digits; i++)
    {
	u8 digit = digits[number_of_digits - i - 1];
	temp_spritesheet.data = number_spritesheet.data + digit*stride;
	draw_sprite(screen_buffer, &temp_spritesheet, xp, y, color);
	xp += number_spritesheet.width + 1;
    }
}
