/**
* Author: Amani Hernandez (amh9766)
* Assignment: Simple 2D Scene
* Date due: 2025-02-15, 11:59pm
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

// Sources:
//      * Earth - https://nssdc.gsfc.nasa.gov/photo_gallery/photogallery-earth.html
//      * Moon - https://nssdc.gsfc.nasa.gov/photo_gallery/photogallery-moon.html
//      * Sun - https://nssdc.gsfc.nasa.gov/photo_gallery/photogallery-solar.html
//      * Universe - https://science.nasa.gov/image-detail/nustar160728a-2/
constexpr char EARTH_FILEPATH[]      = "content/earth.png",
               MOON_FILEPATH[]       = "content/moon.png",
               SUN_FILEPATH[]        = "content/sun.png",
               UNIVERSE_FILEPATH[]   = "content/universe.jpg";

constexpr float EARTH_ORBIT_RADIUS = 2.50f,
                MOON_ORBIT_RADIUS  = 0.75f;

constexpr glm::vec3 EARTH_INIT_SCALE    = glm::vec3(0.75f, 0.75f, 0.0f),
                    MOON_INIT_SCALE     = glm::vec3(0.25f, 0.25f, 0.25f),
                    SUN_INIT_SCALE      = glm::vec3(4.0f, 4.0f, 0.0f),
                    UNIVERSE_INIT_SCALE = glm::vec3(8.0f, 8.0f, 0.0f);

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_earth_matrix,
          g_moon_matrix,
          g_sun_matrix,
          g_universe_matrix,
          g_view_matrix,
          g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_cumulative_delta_time = 0.0f;

glm::vec3 g_sun_rotation   = glm::vec3(0.0f, 0.0f, 0.0f),
          g_earth_position = glm::vec3(0.0f, 0.0f, 0.0f),
          g_moon_position  = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint g_earth_texture_id,
       g_moon_texture_id,
       g_sun_texture_id,
       g_universe_texture_id;


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

    g_display_window = SDL_CreateWindow("Simple Scene",
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

    g_earth_matrix      = glm::mat4(1.0f);
    g_moon_matrix       = glm::mat4(1.0f);
    g_sun_matrix        = glm::mat4(1.0f);
    g_universe_matrix   = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-4.0f, 4.0f, -3.0f, 3.0f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_earth_texture_id       = load_texture(EARTH_FILEPATH);
    g_moon_texture_id        = load_texture(MOON_FILEPATH);
    g_sun_texture_id         = load_texture(SUN_FILEPATH);
    g_universe_texture_id    = load_texture(UNIVERSE_FILEPATH);

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
    float earth_theta    = g_cumulative_delta_time;
    float moon_theta     = -earth_theta * 2.0f;
    float sun_theta      = earth_theta * 1.5f;
    float universe_theta = earth_theta * 0.2f;

    g_earth_position.x = EARTH_ORBIT_RADIUS * cosf(earth_theta);
    g_earth_position.y = EARTH_ORBIT_RADIUS * sinf(earth_theta);

    g_moon_position.x = MOON_ORBIT_RADIUS * cosf(moon_theta);
    g_moon_position.y = MOON_ORBIT_RADIUS * sinf(moon_theta);

    /* Model matrix reset */
    g_earth_matrix      = glm::mat4(1.0f);
    g_moon_matrix       = glm::mat4(1.0f);
    g_sun_matrix        = glm::mat4(1.0f);
    g_universe_matrix   = glm::mat4(1.0f);

    /* Transformations */
    g_earth_matrix = glm::translate(g_earth_matrix, g_earth_position);
    g_earth_matrix = glm::scale(g_earth_matrix, EARTH_INIT_SCALE);

    g_moon_matrix = glm::translate(g_moon_matrix, g_earth_position);
    g_moon_matrix = glm::translate(g_moon_matrix, g_moon_position);
    g_moon_matrix = glm::scale(g_moon_matrix, MOON_INIT_SCALE);

    glm::vec3 g_sun_scale = SUN_INIT_SCALE * (1.0f - 0.1f*cosf(sun_theta));
    g_sun_matrix = glm::scale(g_sun_matrix, g_sun_scale);

    g_universe_matrix = glm::rotate(g_universe_matrix, 
                                      universe_theta,
                                      glm::vec3(0.0f, 0.0f, 1.0f));
    g_universe_matrix = glm::scale(g_universe_matrix, UNIVERSE_INIT_SCALE);
}


void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
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
    draw_object(g_universe_matrix, g_universe_texture_id);
    draw_object(g_earth_matrix, g_earth_texture_id);
    draw_object(g_moon_matrix, g_moon_texture_id);
    draw_object(g_sun_matrix, g_sun_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


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

