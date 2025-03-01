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

enum AppStatus { RUNNING, TERMINATED };

constexpr float SPEED = 3.0f;
constexpr int FIRST_TO_SCORE = 3;

class Paddle
{
    static constexpr float VERTICAL_BOUND = 1.712f;

    private:
        glm::vec3 position;
        float direction;
        GLuint texture_id;
        int score;
        bool is_player;
    
    public:
        static constexpr glm::vec3 INIT_POS = glm::vec3(3.06f, 0.0f, 0.0f);
        float width, height;
        glm::mat4 model_matrix;

        Paddle(glm::vec3 position, GLuint texture_id)
        {
            this->position = position;
            this->width = 0.472;
            this->height = 0.78f;
            this->direction = 0.0f;
            this->model_matrix = glm::mat4(1.0f);
            this->texture_id = texture_id;
            this->score = 0;
            this->is_player = true;
        }

        const GLuint& get_texture_id()
        {
            return this->texture_id;
        }

        glm::vec3 get_position()
        {
            return this->position;
        }

        float get_width()
        {
            return this->width;
        }

        float get_height()
        {
            return this->height;
        }

        void set_neutral()
        {
            this->direction = 0.0f;
        }

        void set_up()
        {
            this->direction = 1.0f;
        }

        void set_down()
        {
            this->direction = -1.0f;
        }

        void increase_score()
        {
            this->score++;
        }

        bool check_score()
        {
            return this->score >= FIRST_TO_SCORE;
        }

        void reset()
        {
            this->position.y = 0.0f;
            this->direction = 0.0f;
            this->score = 0;
        }

        void update(float delta_time)
        {
            if(this->is_player)
            {
                this->position.y += this->direction * SPEED * delta_time;
                this->position.y = std::max(-VERTICAL_BOUND, std::min(this->position.y, VERTICAL_BOUND));
            }
            else
            {
                float pos_change = this->direction * SPEED * delta_time;
                float new_pos = this->position.y + pos_change;

                if (new_pos >= VERTICAL_BOUND)
                {
                    this->set_down();
                    new_pos *= -1;
                }
                else if (new_pos <= -VERTICAL_BOUND)
                {
                    this->set_up();
                    new_pos *= -1;
                }

                this->position.y += pos_change;
            }
        }

        bool get_status() 
        {
            return this->is_player;
        }

        void toggle_playability()
        {
            this->is_player = !this->is_player;
        }

        friend std::ostream& operator<<(std::ostream& os, const Paddle& p)
        {
            return os << "Position:\n\tX: " << p.position.x << "\n\tY: " << p.position.y;
        }
};

class Ball
{
    static constexpr float VERTICAL_BOUND = 2.2f;
    static constexpr float HORIZONTAL_BOUND = 4.17f;

    private:
        glm::vec3 position;
        glm::vec3 direction;
        float width, height;
        int bounces;
        bool is_player_one;
        bool is_enabled;

        static float get_rand_radian()
        {
            float normalized = ((float) rand()) / ((float) RAND_MAX);
            return 2.0f * M_PI * normalized - M_PI / 2.0f;
        }

    public:
        static constexpr int MAX_AMOUNT = 3;
        glm::mat4 model_matrix;

        Ball()
        {
            this->model_matrix = glm::mat4(1.0f);
            this->position = glm::vec3(0.0f, 0.0f, 0.0f);
            this->direction = glm::vec3(0.0f, 0.0f, 0.0f);
            this->width = 0.472f;
            this->height = 0.78f;
            this->bounces = 0;
            this->is_player_one = true;
            this->is_enabled = false;

            this->set_random_direction();
        }

        float get_width()
        {
            return this->width;
        }

        float get_height()
        {
            return this->height;
        }

        glm::vec3 get_position()
        {
            return this->position;
        }

        void enable()
        {
            this->is_enabled = true;
        }

        void disable()
        {
            this->is_enabled = false;
        }

        bool get_status()
        {
            return this->is_enabled;
        }

        void set_random_direction()
        {
            float theta = get_rand_radian();
            this->direction.x = cosf(theta);
            this->direction.y = sinf(theta);
        }

        bool is_out_of_bounds(Paddle* p1, Paddle* p2)
        {
            if (this->position.x <= -HORIZONTAL_BOUND) 
            {
                p2->increase_score();
                return true;
            }

            if (this->position.x >= HORIZONTAL_BOUND)
            {
                p1->increase_score();
                return true;
            }

            return false;
        }

        void reset()
        {
            this->position = glm::vec3(0.0f, 0.0f, 0.0f);
            this->bounces = 0;
            this->set_random_direction();
        }

        void update(float delta_time, Paddle* p)
        {
            float total_speed = SPEED + 0.1f * bounces;
            float p_width = p->get_width();
            float p_height = p->get_height();
            glm::vec3 p_pos = p->get_position();
            glm::vec3 b_pos = this->position + this->direction * total_speed * delta_time;

            bool old_col_x = (this->position.x <= p_pos.x + p_width) && 
                             (this->position.x + this->width  >= p_pos.x);
            bool old_col_y = (this->position.y - this->height <= p_pos.y) &&
                             (this->position.y >= p_pos.y - p_height);

            bool new_col_x = (b_pos.x <= p_pos.x + p_width) && 
                             (b_pos.x + this->width  >= p_pos.x);
            bool new_col_y = (b_pos.y - this->height <= p_pos.y) &&
                             (b_pos.y >= p_pos.y - p_height);

            if (new_col_x && new_col_y)
            {
                if (old_col_x != new_col_x) this->direction.x *= -1.0f;
                if (old_col_y != new_col_y) this->direction.y *= -1.0f;
                this->bounces++;
            }

            if ((b_pos.y >= VERTICAL_BOUND) ||
                (b_pos.y <= -VERTICAL_BOUND))
            {
                this->direction.y *= -1.0f;
                this->bounces++;
            } 

            this->position += this->direction * total_speed * delta_time;
        }

        friend std::ostream& operator<<(std::ostream& os, const Ball& b)
        {
            return os << "Position:\n\tX: " << b.position.x << "\n\tY: " << b.position.y
                      << "\nVelocity:\n\tX: " << b.direction.x << "\n\tY: " << b.direction.y
                                              << "\n\tSpeed: " << (SPEED + b.bounces * 0.1f)
                                              << "\n\tBounces: " << b.bounces;
        }
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
               WALL_FILEPATH[]       = "content/wall.png",
               WIN_ONE_FILEPATH[]    = "content/win_one.png",
               WIN_TWO_FILEPATH[]    = "content/win_two.png";

constexpr glm::vec3 PADDLE_INIT_SCALE = glm::vec3(0.64, 1.28f, 0.0f),
                    BALL_INIT_SCALE   = glm::vec3(0.32f, 0.32f, 0.0f),
                    WALL_INIT_SCALE   = glm::vec3(8.0f, 0.64f, 0.0f),
                    WIN_INIT_SCALE    = glm::vec3(8.0f, 6.0f, 0.0f);

const glm::mat4 TOP_WALL_MODEL_MATRIX = glm::scale(
                    glm::translate(
                        glm::mat4(1.0f),
                        glm::vec3(0.0f, 2.67f, 0.0f)
                    ),
                    WALL_INIT_SCALE
                ),
                LOW_WALL_MODEL_MATRIX = glm::scale(
                    glm::translate(
                        glm::mat4(1.0f),
                        glm::vec3(0.0f, -2.67, 0.0f)
                    ),
                    WALL_INIT_SCALE
                ),
                WIN_SCREEN_MODEL_MATRIX = glm::scale(
                    glm::mat4(1.0f),
                    WIN_INIT_SCALE 
                );

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

Paddle* player_one = nullptr;
Paddle* player_two = nullptr;

Ball* balls = nullptr;

glm::mat4 g_view_matrix,
          g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_cumulative_delta_time = 0.0f;

GLuint g_ball_texture_id,
       g_wall_texture_id,
       g_win_one_texture_id,
       g_win_two_texture_id;

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

    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-4.0f, 4.0f, -3.0f, 3.0f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_ball_texture_id = load_texture(BALL_FILEPATH);
    g_wall_texture_id = load_texture(WALL_FILEPATH);
    g_win_one_texture_id = load_texture(WIN_ONE_FILEPATH);
    g_win_two_texture_id = load_texture(WIN_TWO_FILEPATH);

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
                    case SDLK_t:
                        // Switch Player 2 to CPU mode and start it down
                        player_two->toggle_playability();
                        player_two->set_down();
                        break;
                    case SDLK_d:
                        // Printing debug information
                        LOG("Player 1");
                        LOG(*player_one);

                        LOG("Player 2");
                        LOG(*player_two);

                        LOG("Ball 1");
                        LOG(balls[0]);
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
    // Check if either player has won yet
    g_won = player_one->check_score() || player_two->check_score();

    /* Delta time calculations */
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    g_cumulative_delta_time += delta_time;

    /* Game logic */
    if (!g_pause || g_won)
    {
        player_one->update(delta_time);
        player_two->update(delta_time);

        if (balls[0].get_position().x <= 0) 
        {
            balls[0].update(delta_time, player_one);
        }
        else
        {
            balls[0].update(delta_time, player_two);
        }

        if (balls[0].is_out_of_bounds(player_one, player_two)) balls[0].reset();

        /* Model matrix reset */
        player_one->model_matrix = glm::mat4(1.0f);
        player_two->model_matrix = glm::mat4(1.0f);

        for (int i = 0; i < Ball::MAX_AMOUNT; i++)
        {
            if (balls[i].get_status()) balls[i].model_matrix = glm::mat4(1.0f);
        }

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
            if (balls[i].get_status())
            {
                balls[i].model_matrix = glm::scale(
                    glm::translate(
                        balls[i].model_matrix,
                        balls[i].get_position()
                    ),
                    BALL_INIT_SCALE
                );
            }
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
            draw_object(WIN_SCREEN_MODEL_MATRIX, g_win_one_texture_id);
        }
        else
        {
            draw_object(WIN_SCREEN_MODEL_MATRIX, g_win_two_texture_id);
        }
    }
    else
    {
        draw_object(player_one->model_matrix, player_one->get_texture_id());
        draw_object(player_two->model_matrix, player_two->get_texture_id());

        for (int i = 0; i < Ball::MAX_AMOUNT; i++)
        {
            if (balls[i].get_status()) draw_object(balls[i].model_matrix, g_ball_texture_id);
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

