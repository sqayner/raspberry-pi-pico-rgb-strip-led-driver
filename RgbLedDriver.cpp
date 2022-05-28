#include <stdio.h>

#include <string.h>
#include <iostream>
#include <stdexcept>
#include <regex>

#define _USE_MATH_DEFINES
#include <math.h>

#include "pico/stdlib.h"

#include "hardware/pwm.h"
#include "hardware/clocks.h"

#include "pico/time.h"

#include "pico/multicore.h"

struct RGB
{
    int r;
    int g;
    int b;
};

#define LED_R 9
#define LED_G 11
#define LED_B 7

#define SUFFIX 0x0a // LF
#define DELIMITER '='

int count_top = 1000;
int count = 0;

int ms = 1;

int slice_num_led_r;
int slice_num_led_g;
int slice_num_led_b;

char userInput;
std::string command = "";

pwm_config config;

bool going_up = true;

int in_line_current_led;
int in_line_current_led_count;

bool finish_fade_2 = true;

int level_led_r = 0;
int level_led_g = 0;
int level_led_b = 0;

int fade_2_degree = 90 * 20;
int fade_2_sinout;

struct RGB color;

std::string current_mode;

repeating_timer current_timer;

void error(const char *e)
{
    printf("%s\n", e);
}

struct RGB color_converter(int hexValue)
{
    struct RGB rgbColor;
    rgbColor.r = ((hexValue >> 16) & 0xFF);
    rgbColor.g = ((hexValue >> 8) & 0xFF);
    rgbColor.b = ((hexValue)&0xFF);
    return (rgbColor);
}

void fade()
{
    if (going_up)
    {
        ++count;
        if (count >= count_top)
        {
            count = count_top;
            going_up = false;
        }
    }
    else
    {
        --count;
        if (count <= 0)
        {
            count = 0;
            going_up = true;
        }
    }

    pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, count);
    pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, count);
    pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, count);
}

void fade_2()
{
    if (going_up)
    {
        ++fade_2_degree;
        if (fade_2_degree == 270 * 20)
            fade_2_sinout = 0;
        else
        {
            float rad = (M_PI * (fade_2_degree / 20)) / 180;
            fade_2_sinout = (sin(rad) * 500) + 500;
        }
    }
    else
    {
        --fade_2_degree;
        if (fade_2_degree == 90 * 20)
            fade_2_sinout = count_top;
        else
        {
            float rad = (M_PI * (fade_2_degree / 20)) / 180;
            fade_2_sinout = (sin(rad) * 500) + 500;
        }
    }

    if (finish_fade_2)
    {
        if (level_led_r > 0 && level_led_g > 0 && level_led_b > 0)
        {
            level_led_g = fade_2_sinout;
            pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, level_led_g);
            if (level_led_g == 0)
            {
                fade_2_degree = 90 * 20;
                going_up = true;
            }
        }
        else if (level_led_r > 0 && level_led_g == 0 && level_led_b > 0)
        {
            level_led_b = fade_2_sinout;
            pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, level_led_b);
            if (level_led_b == 0)
            {
                fade_2_degree = 270 * 20;
                going_up = false;
            }
        }
        else if (level_led_r > 0 && level_led_g != count_top && level_led_b == 0)
        {
            level_led_g = fade_2_sinout;
            pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, level_led_g);
            if (level_led_g == count_top)
            {
                fade_2_degree = 90 * 20;
                going_up = true;
            }
        }
        else if (level_led_r != 0 && level_led_g == count_top && level_led_b == 0)
        {
            level_led_r = fade_2_sinout;
            pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, level_led_r);
            if (level_led_r == 0)
            {
                fade_2_degree = 270 * 20;
                going_up = false;
            }
        }
        else if (level_led_r == 0 && level_led_g == count_top && level_led_b != count_top)
        {
            level_led_b = fade_2_sinout;
            pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, level_led_b);
            if (level_led_b == count_top)
            {
                fade_2_degree = 90 * 20;
                going_up = true;
            }
        }
        else if (level_led_r == 0 && level_led_g != 0 && level_led_b == count_top)
        {
            level_led_g = fade_2_sinout;
            pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, level_led_g);
            if (level_led_g == 0)
            {
                fade_2_degree = 270 * 20;
                going_up = false;
                finish_fade_2 = false;
            }
        }
    }
    else
    {
        if (level_led_r != count_top && level_led_g != count_top && level_led_b == count_top)
        {
            level_led_g = fade_2_sinout;
            level_led_r = fade_2_sinout;
            pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, level_led_r);
            pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, level_led_g);
            if (level_led_r == count_top)
            {
                fade_2_degree = 90 * 20;
                going_up = true;
                finish_fade_2 = true;
            }
        }
    }
}

void fade_3()
{
    if (in_line_current_led_count == 0)
    {
        if (in_line_current_led == slice_num_led_r)
            in_line_current_led = slice_num_led_g;
        else if (in_line_current_led == slice_num_led_g)
            in_line_current_led = slice_num_led_b;
        else
            in_line_current_led = slice_num_led_r;
    }

    if (going_up)
    {
        ++in_line_current_led_count;
        if (in_line_current_led_count >= count_top)
        {
            in_line_current_led_count = count_top;
            going_up = false;
        }
    }
    else
    {
        --in_line_current_led_count;
        if (in_line_current_led_count <= 0)
        {
            in_line_current_led_count = 0;
            going_up = true;
        }
    }

    pwm_set_wrap(in_line_current_led, count_top);
    pwm_set_enabled(in_line_current_led, true);
    pwm_set_chan_level(in_line_current_led, PWM_CHAN_B, in_line_current_led_count);
}

void color_mode()
{
    pwm_set_wrap(slice_num_led_r, 255);
    pwm_set_enabled(slice_num_led_r, true);
    pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, color.r);

    pwm_set_wrap(slice_num_led_g, 255);
    pwm_set_enabled(slice_num_led_g, true);
    pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, color.g);

    pwm_set_wrap(slice_num_led_b, 255);
    pwm_set_enabled(slice_num_led_b, true);
    pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, color.b);
}

bool drive_mosfets(struct repeating_timer *t)
{
    pwm_set_wrap(slice_num_led_r, count_top);
    pwm_set_wrap(slice_num_led_g, count_top);
    pwm_set_wrap(slice_num_led_b, count_top);

    if (current_mode == "fade")
    {
        fade();
    }
    else if (current_mode == "fade_2")
    {
        fade_2();
    }
    else if (current_mode == "fade_3")
    {
        fade_3();
    }
    else if (current_mode == "color")
    {
        color_mode();
    }

    return true;
}

void turn_off_the_lights()
{
    pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, 0);
    pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, 0);
    pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, 0);
}

void change_mode(std::string value)
{
    turn_off_the_lights();

    going_up = true;

    if (value == "fade")
    {
        current_mode = value;
        count = 0;
    }
    else if (value == "fade_2")
    {
        level_led_r = count_top;
        pwm_set_wrap(slice_num_led_r, count_top);
        pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, level_led_r);

        level_led_g = count_top;
        pwm_set_wrap(slice_num_led_g, count_top);
        pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, level_led_g);

        level_led_b = count_top;
        pwm_set_wrap(slice_num_led_b, count_top);
        pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, level_led_b);

        current_mode = value;
    }
    else if (value == "fade_3")
    {
        current_mode = value;
        in_line_current_led_count = 0;
    }
    else if (value == "color")
    {
        current_mode = value;
    }
}

void run_command(std::string command)
{
    std::string value = command.substr(command.find(DELIMITER) + 1, command.length());
    command = command.substr(0, command.find(DELIMITER));

    if (command == "mode")
    {
        change_mode(value);
    }
    else if (command == "color")
    {
        try
        {
            int hexColor = stoi(value, 0, 16);
            color = color_converter(hexColor);
        }
        catch (std::invalid_argument &e)
        {
            error(e.what());
        }
        catch (std::out_of_range &e)
        {
            error(e.what());
        }
    }
    else if (command == "speed")
    {
        try
        {
            count_top = stoi(value);
        }
        catch (std::invalid_argument &e)
        {
            error(e.what());
        }
        catch (std::out_of_range &e)
        {
            error(e.what());
        }
    }
}

int main()
{
    stdio_init_all();

    add_repeating_timer_ms(ms, drive_mosfets, NULL, &current_timer);

    config = pwm_get_default_config();

    gpio_set_function(LED_R, GPIO_FUNC_PWM);
    slice_num_led_r = pwm_gpio_to_slice_num(LED_R);
    pwm_set_enabled(slice_num_led_r, true);

    gpio_set_function(LED_G, GPIO_FUNC_PWM);
    slice_num_led_g = pwm_gpio_to_slice_num(LED_G);
    pwm_set_enabled(slice_num_led_g, true);

    gpio_set_function(LED_B, GPIO_FUNC_PWM);
    slice_num_led_b = pwm_gpio_to_slice_num(LED_B);
    pwm_set_enabled(slice_num_led_b, true);

    run_command("mode=fade_2");

    while (true)
    {
        userInput = getchar_timeout_us(0);

        if ((userInput == SUFFIX || userInput == 255) && command != "")
        {
            run_command(command);
            command = "";
        }
        else if (userInput != 255)
            command.push_back(userInput);
    }
    return 0;
}