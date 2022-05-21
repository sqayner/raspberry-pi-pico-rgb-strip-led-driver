#include <stdio.h>

#include <string.h>
#include <iostream>
#include <stdexcept>
#include <regex>

#include "pico/stdlib.h"

#include "hardware/pwm.h"
#include "hardware/clocks.h"

#include "pico/time.h"

#include "pico/multicore.h"
#include "hardware/irq.h"

struct RGB
{
    int r;
    int g;
    int b;
};

const uint LED_R = 7;
const uint LED_G = 9;
const uint LED_B = 11;

const char SUFFIX = 0x0a; // LF
const char DELIMITER = '=';

uint count_top = 100;
uint count = 0;

uint ms = 1;

uint slice_num_led_r;
uint slice_num_led_g;
uint slice_num_led_b;

char userInput;
std::string command = "";

pwm_config config;

bool going_up = true;

uint in_line_current_led;
uint in_line_current_led_count;

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

bool repeating_timer_callback(struct repeating_timer *t)
{
    if (current_mode == "fade")
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

        pwm_clear_irq(slice_num_led_r);
        pwm_set_irq_enabled(slice_num_led_r, true);
        pwm_set_wrap(slice_num_led_r, count_top);
        pwm_set_enabled(slice_num_led_r, true);
        pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, count);

        pwm_clear_irq(slice_num_led_g);
        pwm_set_irq_enabled(slice_num_led_g, true);
        pwm_set_wrap(slice_num_led_g, count_top);
        pwm_set_enabled(slice_num_led_g, true);
        pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, count);

        pwm_clear_irq(slice_num_led_b);
        pwm_set_irq_enabled(slice_num_led_b, true);
        pwm_set_wrap(slice_num_led_b, count_top);
        pwm_set_enabled(slice_num_led_b, true);
        pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, count);
    }
    else if (current_mode == "in_line_fade")
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
    else if (current_mode == "color")
    {
        pwm_clear_irq(slice_num_led_r);
        pwm_set_irq_enabled(slice_num_led_r, true);
        pwm_set_wrap(slice_num_led_r, 255);
        pwm_set_enabled(slice_num_led_r, true);
        pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, color.r);

        pwm_clear_irq(slice_num_led_g);
        pwm_set_irq_enabled(slice_num_led_g, true);
        pwm_set_wrap(slice_num_led_g, 255);
        pwm_set_enabled(slice_num_led_g, true);
        pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, color.g);

        pwm_clear_irq(slice_num_led_b);
        pwm_set_irq_enabled(slice_num_led_b, true);
        pwm_set_wrap(slice_num_led_b, 255);
        pwm_set_enabled(slice_num_led_b, true);
        pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, color.b);
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
    else if (value == "in_line_fade")
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

    add_repeating_timer_ms(ms, repeating_timer_callback, NULL, &current_timer);

    config = pwm_get_default_config();

    gpio_set_function(LED_R, GPIO_FUNC_PWM);
    slice_num_led_r = pwm_gpio_to_slice_num(LED_R);

    gpio_set_function(LED_G, GPIO_FUNC_PWM);
    slice_num_led_g = pwm_gpio_to_slice_num(LED_G);

    gpio_set_function(LED_B, GPIO_FUNC_PWM);
    slice_num_led_b = pwm_gpio_to_slice_num(LED_B);

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