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

#define WIDTH  448
#define HEIGHT 512

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
    u32 type;
};

struct player
{
    u32 x, y;
    u32 life;
};

struct game
{
    u32 width, height;
    u32 number_of_aliens;
    alien *aliens;
    player main_player;
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

int main(int argc, char **argv)
{
    if (!glfwInit())
    {
	std::cout << "Could not initialize glfw" << std::endl;
    }

    
    GLFWwindow *window = glfwCreateWindow(WIDTH,
					  HEIGHT,
					  "FUCKING SPACE INVADERS",
					  0,
					  0);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
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

    sprite alien_sprite0;
    alien_sprite0.width = 11;
    alien_sprite0.height = 8;
    alien_sprite0.data = new u8[alien_sprite0.width*alien_sprite0.height]
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

    sprite alien_sprite1;
    alien_sprite1.width = 11;
    alien_sprite1.height = 8;
    alien_sprite1.data = new u8[alien_sprite1.width*alien_sprite1.height]
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

    sprite_animation *alien_animation = new sprite_animation;
    alien_animation->loop = true;
    alien_animation->number_of_frames = 2;
    alien_animation->frame_duration = 10;
    alien_animation->time = 0;

    alien_animation->frames = new sprite*[2];
    alien_animation->frames[0] = &alien_sprite0;
    alien_animation->frames[1] = &alien_sprite1;
    
    // Setting up game
    game game_entity;
    game_entity.width = WIDTH;
    game_entity.height = HEIGHT;
    game_entity.number_of_aliens = 55;
    game_entity.aliens = new alien[game_entity.number_of_aliens];    

    game_entity.main_player.x = 2*105;
    game_entity.main_player.y = 2*32;

    game_entity.main_player.life = 3;

    for (u32 yi = 0; yi < 5; yi++)
    {
	for (u32 xi = 0; xi < 11; xi++)
	{
	    game_entity.aliens[yi*11 + xi].x = 2*(16*xi + 20);
	    game_entity.aliens[yi*11 + xi].y = 2*(17*yi + 128);
	}
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

    i32 player_move_dir = 1;
    while (!glfwWindowShouldClose(window))
    {
	clear_buffer(&screen_buffer, screen_color);
	// animating

	for (u32 ai = 0; ai < game_entity.number_of_aliens; ai++)
	{
	    alien temp_alien = game_entity.aliens[ai];
	    u32 current_frame = alien_animation->time/alien_animation->frame_duration;
	    sprite *temp_sprite = alien_animation->frames[current_frame];
	    draw_sprite(&screen_buffer, temp_sprite,
			temp_alien.x, temp_alien.y,
			color_to_u32(255, 0, 0));
	}

	if (game_entity.main_player.x + player_sprite.width + player_move_dir >= game_entity.width - 1)
	{
	    game_entity.main_player.x = game_entity.width - player_sprite.width - player_move_dir - 1;
	    player_move_dir *= -1;
	}
	// NOTE: Get deep into casting stuff
	else if ((int)game_entity.main_player.x + player_move_dir <= 0)
	{
	    game_entity.main_player.x = 0;
	    player_move_dir *= -1;
	}
	else
	{
	    game_entity.main_player.x += player_move_dir;
	}
	draw_sprite(&screen_buffer, &player_sprite,
		    game_entity.main_player.x, game_entity.main_player.y,
		    color_to_u32(0, 255, 0));
	
	alien_animation->time++;
	if (alien_animation->time == alien_animation->number_of_frames*alien_animation->frame_duration)
	{
	    if (alien_animation->loop)
	    {
		alien_animation->time = 0;
	    }
	    else
	    {
		delete alien_animation;
		alien_animation = 0;
	    }
	}
	
	glTexSubImage2D(GL_TEXTURE_2D,
			0, 0, 0,
			screen_buffer.width, screen_buffer.height,
			GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
			screen_buffer.data);
	
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);	
	
	glfwSwapBuffers(window);
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

