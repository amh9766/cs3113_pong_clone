/**
* Author: Amani Hernandez (amh9766)
* Assignment: Pong Clone
* Date due: 2025-3-01, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

class Collision {
    private:
        float left, right, bottom, top;
        
    public:
        Collision(float left, float bottom, float right, float top)
        {
            this->left = left;
            this->bottom = bottom;
            this->right = right;
            this->top = top;
        }

        bool intersects_with(Collision& collision)
        {
            return (collision.left < this->right) &&
                   (collision.right > this->left) &&
                   (collision.bottom > this->top) &&
                   (collision.top < this->bottom);
        }
};

class Paddle {
    private:
        //Collision hitbox;
        glm::vec3 position;
        GLuint texture_id;
        int score;
        bool is_cpu;
    
    public:
        glm::mat4 model_matrix;

        Paddle(glm::vec3 position, GLuint texture_id)
        {
            //this->hitbox = Collision(0.0f, 0.0f, 0.0f, 0.0f);
            this->position = position;
            this->model_matrix = glm::mat4(1.0f);
            this->texture_id = texture_id;
            this->score = 0;
            this->is_cpu = false;
        }

        const GLuint& get_texture_id()
        {
            return this->texture_id;
        }

        glm::vec3& get_position()
        {
            return this->position;
        }

        void move(bool is_up, float theta)
        {
            if(is_cpu)
            {
                this->position.y = cosf(theta);
            }
            else
            {
                this->position.y += (is_up ? 1.0f : -1.0f);
            }

        }

        void toggle_cpu()
        {
            this->is_cpu = !this->is_cpu;
        }
};

class Ball
{
    private:
        //Collision hitbox;
        glm::vec3 position;
        glm::vec3 direction;
        bool is_player_one;
        bool is_enabled;

    public:
        static constexpr int MAX_AMOUNT = 3;
        glm::mat4 model_matrix;

        Ball()
        {
            //this->hitbox = Collision(0.0f, 0.0f, 0.0f, 0.0f);
            this->model_matrix = glm::mat4(1.0f);
            this->position = glm::vec3(1.0f, 1.0f, 0.0f);
            this->direction = glm::vec3(1.0f, 1.0f, 0.0f);
            this->is_player_one = true;
            this->is_enabled = false;
        }

        void toggle();
        void calculate_direction();
        void move();

};

constexpr int WINDOW_WIDTH  = 960,
              WINDOW_HEIGHT = 720;

constexpr float BG_RED     = 0.0f,
                BG_GREEN   = 0.0f,
                BG_BLUE    = 0.0f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
                LEVEL_OF_DETAIL    = 0, // mipmap reduction image level
                TEXTURE_BORDER     = 0; // this value MUST be zero

// Source: https://www.spriters-resource.com/nes/supermariobros/sheet/52571/
constexpr char PLAYER_ONE_FILEPATH[] = "content/player_one.png",
               PLAYER_TWO_FILEPATH[] = "content/player_two.png",
               BALL_FILEPATH[]       = "content/ball.png",
               WALL_FILEPATH[]       = "content/wall.png";

constexpr glm::vec3 PADDLE_INIT_SCALE = glm::vec3(0.64, 1.28f, 0.0f),
                    BALL_INIT_SCALE   = glm::vec3(0.64f, 0.64f, 0.0f),
                    WALL_INIT_SCALE   = glm::vec3(8.0f, 0.64f, 0.0f);

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

Paddle* player_one = nullptr;
Paddle* player_two = nullptr;

Ball* balls = nullptr;

glm::mat4 g_wall_model_matrix,
          g_view_matrix,
          g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_cumulative_delta_time = 0.0f;

GLuint g_ball_texture_id,
       g_wall_texture_id;

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}


void initialise()
{
    // Initialise video
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Pong Clone",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_wall_model_matrix       = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-4.0f, 4.0f, -3.0f, 3.0f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_ball_texture_id = load_texture(BALL_FILEPATH);
    g_wall_texture_id = load_texture(WALL_FILEPATH);

    player_one = new Paddle(
        glm::vec3(-3.02f, 0.0f, 0.0f),
        load_texture(PLAYER_ONE_FILEPATH)
    );
    player_two = new Paddle(
        glm::vec3(3.08f, 0.0f, 0.0f),
        load_texture(PLAYER_TWO_FILEPATH)
    );

    balls = new Ball[Ball::MAX_AMOUNT];

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}


void update()
{
    /* Delta time calculations */
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    g_cumulative_delta_time += delta_time;

    /* Game logic */

    /* Model matrix reset */
    player_one->model_matrix = glm::mat4(1.0f);
    player_two->model_matrix = glm::mat4(1.0f);

    for (int i = 0; i < Ball::MAX_AMOUNT; i++)
    {
        balls[i].model_matrix = glm::mat4(1.0f);
    }

    g_wall_model_matrix = glm::mat4(1.0f);

    /* Transformations */
    player_one->model_matrix = glm::scale(
        glm::translate(
            player_one->model_matrix,
            player_one->get_position()
        ),
        PADDLE_INIT_SCALE
    );

    player_two->model_matrix = glm::scale(
        glm::translate(
            player_two->model_matrix,
            player_two->get_position()
        ),
        PADDLE_INIT_SCALE
    );

    for (int i = 0; i < Ball::MAX_AMOUNT; i++)
    {
        balls[i].model_matrix = glm::scale(
            balls[i].model_matrix,
            BALL_INIT_SCALE
        );
    }

    g_wall_model_matrix = glm::scale(
        g_wall_model_matrix,
        WALL_INIT_SCALE
    );
}


void draw_object(glm::mat4 &object_g_model_matrix, const GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        // Triangle 1
        -0.5f, -0.5f, // Lower left
        0.5f, -0.5f,  // Lower right
        0.5f, 0.5f,   // Upper right
        // Triangle 2
        -0.5f, -0.5f, // Lower left
        0.5f, 0.5f,   // Upper right
        -0.5f, 0.5f   // Upper left
    };

    // Textures
    float texture_coordinates[] =
    {
        // Triangle 1
        0.0f, 1.0f, // Lower left
        1.0f, 1.0f, // Lower right
        1.0f, 0.0f, // Upper right
        // Triangle 2
        0.0f, 1.0f, // Lower left
        1.0f, 0.0f, // Upper right
        0.0f, 0.0f, // Upper left
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
                          0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(player_one->model_matrix, player_one->get_texture_id());
    draw_object(player_two->model_matrix, player_two->get_texture_id());

    for (int i = 0; i < Ball::MAX_AMOUNT; i++)
    {
        draw_object(balls[i].model_matrix, g_ball_texture_id);
    }

    draw_object(g_wall_model_matrix, g_wall_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown()
{ 
    delete player_one;
    delete player_two;

    delete [] balls;

    SDL_Quit(); 
}


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}

