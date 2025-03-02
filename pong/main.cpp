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
#include <time.h>
#include <stdlib.h>

#include "pong_lib.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr glm::vec3 WALL_INIT_SCALE = glm::vec3(8.0f, 0.64f, 0.0f),
                    SCREEN_INIT_SCALE  = glm::vec3(8.0f, 6.0f, 0.0f);

const glm::mat4 TOP_WALL_MODEL_MATRIX = glm::scale(
                    glm::translate(
                        IDENTITY_MATRIX,
                        glm::vec3(0.0f, 2.67f, 0.0f)
                    ),
                    WALL_INIT_SCALE
                 ),
                 LOW_WALL_MODEL_MATRIX = glm::scale(
                    glm::translate(
                        IDENTITY_MATRIX,
                        glm::vec3(0.0f, -2.67, 0.0f)
                    ),
                    WALL_INIT_SCALE
                 ),
                 SCREEN_MODEL_MATRIX = glm::scale(
                    IDENTITY_MATRIX,
                    SCREEN_INIT_SCALE 
                 );

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

constexpr float MILLISECONDS_IN_SECOND = 1000.0f;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
                LEVEL_OF_DETAIL    = 0, // mipmap reduction image level
                TEXTURE_BORDER     = 0; // this value MUST be zero

// Source: https://www.spriters-resource.com/nes/supermariobros/sheet/52571/
constexpr char PLAYER_ONE_FILEPATH[] = "content/player_one.png",
               PLAYER_TWO_FILEPATH[] = "content/player_two.png",
               BALL_ONE_FILEPATH[]   = "content/ball_one.png",
               BALL_TWO_FILEPATH[]   = "content/ball_two.png",
               WIN_ONE_FILEPATH[]    = "content/win_one.png",
               WIN_TWO_FILEPATH[]    = "content/win_two.png",
               WALL_FILEPATH[]       = "content/wall.png",
               BACKGROUND_FILEPATH[] = "content/background.png";

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

Paddle *player_one = nullptr, 
       *player_two = nullptr;
Ball   *balls      = nullptr;

glm::mat4 g_view_matrix,
          g_projection_matrix;

float g_previous_ticks = 0.0f;

GLuint g_ball_one_texture_id,
       g_ball_two_texture_id,
       g_wall_texture_id,
       g_win_one_texture_id,
       g_win_two_texture_id,
       g_background_texture_id;

bool g_pause = false;
bool g_won = false;

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
    // Initialize random generator
    srand(time(NULL));

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

    g_view_matrix       = IDENTITY_MATRIX;
    g_projection_matrix = glm::ortho(-4.0f, 4.0f, -3.0f, 3.0f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_ball_one_texture_id   = load_texture(BALL_ONE_FILEPATH);
    g_ball_two_texture_id   = load_texture(BALL_TWO_FILEPATH);
    g_win_one_texture_id    = load_texture(WIN_ONE_FILEPATH);
    g_win_two_texture_id    = load_texture(WIN_TWO_FILEPATH);
    g_wall_texture_id       = load_texture(WALL_FILEPATH);
    g_background_texture_id = load_texture(BACKGROUND_FILEPATH);

    player_one = new Paddle(
        -Paddle::INIT_POS,
        load_texture(PLAYER_ONE_FILEPATH)
    );
    player_two = new Paddle(
        Paddle::INIT_POS,
        load_texture(PLAYER_TWO_FILEPATH)
    );

    balls = new Ball[Ball::MAX_AMOUNT];
    balls[0].enable();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_q: 
                        g_app_status = TERMINATED;
                        break;
                    case SDLK_3:
                        // Set up three balls
                        if (!g_pause)
                        {
                            for (int i = 0; i < Ball::MAX_AMOUNT; i++)
                            {
                                balls[i].reset();
                                balls[i].enable();
                            }
                        }
                        break;
                    case SDLK_2:
                        // Set up two balls
                        if (!g_pause)
                        {
                            for (int i = 0; i < Ball::MAX_AMOUNT - 1; i++)
                            {
                                balls[i].reset();
                                balls[i].enable();
                            }
                            balls[2].disable();
                        }
                        break;
                    case SDLK_1:
                        // Set up one ball
                        if (!g_pause)
                        {
                            for (int i = 1; i < Ball::MAX_AMOUNT; i++) balls[i].disable();
                            balls[0].reset();
                            balls[0].enable();
                        }
                        break;
                    case SDLK_t:
                        // Switch Player 2 to CPU mode and start it down
                        player_two->toggle_playability();
                        break;
                    case SDLK_d:
                        // Print debug information
                        LOG("Player 1");
                        LOG(*player_one);

                        LOG("Player 2");
                        LOG(*player_two);

                        for (int i = 0; i < Ball::MAX_AMOUNT; i++)
                        {
                            if (balls[i].get_status())
                            {
                                LOG("Ball " << i + 1);
                                LOG(balls[i]);
                            }
                        }
                        break;
                    case SDLK_RETURN:
                        if (g_won)
                        {
                            player_one->reset();
                            player_two->reset();
                            for (int i = 0; i < Ball::MAX_AMOUNT; i++)
                            {
                                balls[i].reset();
                            }
                        }
                        else g_pause = !g_pause;
                        break;
                    default: 
                        break;
                }
            default:
                break;
        } 
    }

    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    // Check up and down movement for keys held down:
    // - Player 1 -> W, S
    // - Player 2 -> Up, Down
    if      (key_state[SDL_SCANCODE_W]) player_one->set_up();
    else if (key_state[SDL_SCANCODE_S]) player_one->set_down();
    else                                player_one->set_neutral();

    if (player_two->get_status())
    {
        if      (key_state[SDL_SCANCODE_UP])   player_two->set_up();
        else if (key_state[SDL_SCANCODE_DOWN]) player_two->set_down();
        else                                   player_two->set_neutral();
    }
}

void update()
{
    // Check and store if either player has won yet
    g_won = player_one->check_score() || player_two->check_score();

    /* Delta time calculations */
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    /* Game logic */
    if (!g_pause || g_won)
    {
        player_one->update(delta_time);
        player_two->update(delta_time);

        for (int i = 0; i < Ball::MAX_AMOUNT; i++)
        {
            if (balls[i].get_status())
            {
                bool hit_paddle = false;
                if (balls[i].get_position().x <= 0) 
                {
                    hit_paddle = balls[i].update(delta_time, player_one);
                    if (hit_paddle) balls[i].set_player_one();
                }
                else
                {
                    hit_paddle = balls[i].update(delta_time, player_two);
                    if (hit_paddle) balls[i].set_player_two();
                }

                if (balls[i].is_out_of_bounds(player_one, player_two))
                {
                    for (int j = 0; j < Ball::MAX_AMOUNT; j++) balls[j].reset();
                    break;
                }
            }
        }

        /* Transformations */
        player_one->update_model_matrix();
        player_two->update_model_matrix();

        for (int i = 0; i < Ball::MAX_AMOUNT; i++)
        {
            if (balls[i].get_status()) balls[i].update_model_matrix();
        }
    }
}

void draw_object(const glm::mat4 &object_g_model_matrix, const GLuint &object_texture_id)
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
    if (g_won)
    {
        if (player_one->check_score()) 
        {
            draw_object(SCREEN_MODEL_MATRIX, g_win_one_texture_id);
        }
        else
        {
            draw_object(SCREEN_MODEL_MATRIX, g_win_two_texture_id);
        }
    }
    else
    {
        draw_object(SCREEN_MODEL_MATRIX, g_background_texture_id);
        draw_object(player_one->get_model_matrix(), player_one->get_texture_id());
        draw_object(player_two->get_model_matrix(), player_two->get_texture_id());

        for (int i = 0; i < Ball::MAX_AMOUNT; i++)
        {
            if (balls[i].get_status()) 
            {
                draw_object(balls[i].get_model_matrix(), 
                            balls[i].get_owner() ? g_ball_one_texture_id
                                                 : g_ball_two_texture_id
                );
            }
        }

        draw_object(TOP_WALL_MODEL_MATRIX, g_wall_texture_id);
        draw_object(LOW_WALL_MODEL_MATRIX, g_wall_texture_id);
    }

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
