#pragma once

#include <time.h>
#include <stdlib.h>

constexpr glm::mat4 IDENTITY_MATRIX = glm::mat4(1.0f);

constexpr float SPEED = 3.0f;

constexpr float STANDARD_WIDTH = 0.472f;
constexpr float STANDARD_HEIGHT = 0.78f;

constexpr int FIRST_TO_SCORE = 3;

class Paddle
{
    public:
        static constexpr glm::vec3 INIT_SCALE = glm::vec3(0.64, 1.28f, 0.0f);
        static constexpr glm::vec3 INIT_POS = glm::vec3(3.08f, 0.0f, 0.0f);
        static constexpr float VERTICAL_BOUND = 1.712f;

    private:
        glm::mat4 model_matrix;
        glm::vec3 position;
        float direction;
        int score;
        GLuint texture_id;
        bool is_player;
    
    public:
        Paddle(glm::vec3 position, GLuint texture_id)
        {
            this->position = position;
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

        glm::mat4 get_model_matrix()
        {
            return this->model_matrix;
        }

        int get_score()
        {
            return this->score;
        }

        void update_model_matrix()
        {
            this->model_matrix = glm::scale(
                glm::translate(
                    IDENTITY_MATRIX,
                    this->position
                ),
                INIT_SCALE
            );
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

        bool get_status() 
        {
            return this->is_player;
        }

        void toggle_playability()
        {
            this->is_player = !this->is_player;
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
                if (this->direction == 0.0f) this->set_down();

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

        friend std::ostream& operator<<(std::ostream& os, const Paddle& p)
        {
            return os << "Position:\n\tX: " << p.position.x << "\n\tY: " << p.position.y;
        }
};

class Ball
{
    public:
        static constexpr glm::vec3 INIT_SCALE = glm::vec3(0.32f, 0.32f, 0.0f);
        static constexpr float VERTICAL_BOUND = 2.2f;
        static constexpr float HORIZONTAL_BOUND = 4.17f;
        static constexpr int MAX_AMOUNT = 3;

    private:
        glm::mat4 model_matrix;
        glm::vec3 position;
        glm::vec3 direction;
        int bounces;
        bool is_player_one;
        bool is_enabled;

        static float get_rand_radian()
        {
            float normalized = ((float) rand()) / ((float) RAND_MAX);
            return 2.0f * M_PI * normalized - M_PI / 2.0f;
        }

    public:
        Ball()
        {
            this->model_matrix = IDENTITY_MATRIX;
            this->position = glm::vec3(0.0f, 0.0f, 0.0f);
            this->direction = glm::vec3(0.0f, 0.0f, 0.0f);
            this->bounces = 0;
            this->is_player_one = true;
            this->is_enabled = false;

            this->set_random_direction();
        }

        glm::vec3 get_position()
        {
            return this->position;
        }

        glm::mat4 get_model_matrix()
        {
            return this->model_matrix;
        }

        void update_model_matrix()
        {
            this->model_matrix = glm::scale(
                glm::translate(
                    IDENTITY_MATRIX,
                    this->position
                ),
                INIT_SCALE
            );
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

        bool get_owner()
        {
            return this->is_player_one;
        }

        void set_player_one()
        {
            this->is_player_one = true;
        }

        void set_player_two()
        {
            this->is_player_one = false;
        }

        void set_random_direction()
        {
            float theta = get_rand_radian();
            this->direction.x = cosf(theta);
            this->direction.y = sinf(theta);

            if (this->direction.x <= 0) this->is_player_one = false;
            else this->is_player_one = true;

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

        bool update(float delta_time, Paddle* p)
        {
            float total_speed = SPEED + 0.1f * bounces;
            glm::vec3 p_pos = p->get_position();
            glm::vec3 b_pos = this->position + this->direction * total_speed * delta_time;

            bool old_col_x = (this->position.x <= p_pos.x + STANDARD_WIDTH) && 
                             (this->position.x + STANDARD_WIDTH  >= p_pos.x);
            bool old_col_y = (this->position.y - STANDARD_HEIGHT <= p_pos.y) &&
                             (this->position.y >= p_pos.y - STANDARD_HEIGHT);

            bool new_col_x = (b_pos.x <= p_pos.x + STANDARD_WIDTH) && 
                             (b_pos.x + STANDARD_WIDTH  >= p_pos.x);
            bool new_col_y = (b_pos.y - STANDARD_HEIGHT <= p_pos.y) &&
                             (b_pos.y >= p_pos.y - STANDARD_HEIGHT);

            bool hit_paddle = false;

            if (new_col_x && new_col_y)
            {
                if (old_col_x != new_col_x) this->direction.x *= -1.0f;
                if (old_col_y != new_col_y) this->direction.y *= -1.0f;
                this->bounces++;
                hit_paddle = true;
            }

            if ((b_pos.y >= VERTICAL_BOUND) ||
                (b_pos.y <= -VERTICAL_BOUND))
            {
                this->direction.y *= -1.0f;
                this->bounces++;
            } 

            this->position += this->direction * total_speed * delta_time;
            return hit_paddle;
        }

        friend std::ostream& operator<<(std::ostream& os, const Ball& b)
        {
            return os << "Position:\n\tX: " << b.position.x << "\n\tY: " << b.position.y
                      << "\nVelocity:\n\tX: " << b.direction.x << "\n\tY: " << b.direction.y
                                              << "\n\tSpeed: " << (SPEED + b.bounces * 0.1f)
                                              << "\n\tBounces: " << b.bounces;
        }
};
