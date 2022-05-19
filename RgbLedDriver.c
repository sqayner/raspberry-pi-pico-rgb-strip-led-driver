#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "hardware/pwm.h"
#include "hardware/clocks.h"

#include "pico/time.h"

#include "pico/multicore.h"
#include "hardware/irq.h"

const uint LED_R = 7;
const uint LED_G = 9;
const uint LED_B = 11;

uint count_top = 500;
uint count = 0;

uint slice_num_led_r;
uint slice_num_led_g;
uint slice_num_led_b;

pwm_config config;

bool going_up = true;

volatile bool timer_fired = false;

bool repeating_timer_callback(struct repeating_timer *t)
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

    timer_fired = true;
    return true;
}

uint in_line_current_led;
uint in_line_current_led_count;
bool rgb_in_line_repeating_timer_callback(struct repeating_timer *t)
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

    pwm_set_chan_level(in_line_current_led, PWM_CHAN_B, in_line_current_led_count);

    timer_fired = true;
    return true;
}

int main()
{
    stdio_init_all();

    //-- led config
    config = pwm_get_default_config();

    gpio_set_function(LED_R, GPIO_FUNC_PWM);
    gpio_set_function(LED_G, GPIO_FUNC_PWM);
    gpio_set_function(LED_B, GPIO_FUNC_PWM);

    //---------------------

    slice_num_led_r = pwm_gpio_to_slice_num(LED_R);
    pwm_clear_irq(slice_num_led_r);
    pwm_set_irq_enabled(slice_num_led_r, true);

    pwm_set_wrap(slice_num_led_r, count_top);
    pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, 50);
    pwm_set_enabled(slice_num_led_r, true);

    //---------------------

    slice_num_led_g = pwm_gpio_to_slice_num(LED_G);
    pwm_clear_irq(slice_num_led_g);
    pwm_set_irq_enabled(slice_num_led_g, true);

    pwm_set_wrap(slice_num_led_g, count_top);
    pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, 50);
    pwm_set_enabled(slice_num_led_g, true);

    //---------------------

    slice_num_led_b = pwm_gpio_to_slice_num(LED_B);
    pwm_clear_irq(slice_num_led_b);
    pwm_set_irq_enabled(slice_num_led_b, true);

    pwm_set_wrap(slice_num_led_b, count_top);
    pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, 50);
    pwm_set_enabled(slice_num_led_b, true);
    //-- led config

    char userInput;

    struct repeating_timer timer;

    while (true)
    {
        userInput = getchar_timeout_us(0);

        if (userInput == '1')
        {
            if (timer_fired)
            {
                cancel_repeating_timer(&timer);
                timer_fired = false;
            }
            add_repeating_timer_ms(1, repeating_timer_callback, NULL, &timer);
        }
        else if (userInput == '2')
        {
            pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, 0);
            pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, 0);
            pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, 0);

            if (timer_fired)
            {
                cancel_repeating_timer(&timer);
                timer_fired = false;
            }
            add_repeating_timer_ms(1, rgb_in_line_repeating_timer_callback, NULL, &timer);
        }
        else if (userInput == 'r')
        {
            if (timer_fired)
            {
                cancel_repeating_timer(&timer);
                timer_fired = false;
            }
            pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, count_top);
            pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, 0);
            pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, 0);
        }
        else if (userInput == 'g')
        {
            if (timer_fired)
            {
                cancel_repeating_timer(&timer);
                timer_fired = false;
            }

            pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, 0);
            pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, count_top);
            pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, 0);
        }
        else if (userInput == 'b')
        {
            if (timer_fired)
            {
                cancel_repeating_timer(&timer);
                timer_fired = false;
            }

            pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, 0);
            pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, 0);
            pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, count_top);
        }
        else if (userInput == 'w')
        {
            if (timer_fired)
            {
                cancel_repeating_timer(&timer);
                timer_fired = false;
            }

            pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, count_top);
            pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, count_top);
            pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, count_top);
        }
        else if (userInput == 'c')
        {
            if (timer_fired)
            {
                cancel_repeating_timer(&timer);
                timer_fired = false;
            }

            pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, 0);
            pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, count_top);
            pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, count_top);
        }
        else if (userInput == 'p')
        {
            if (timer_fired)
            {
                cancel_repeating_timer(&timer);
                timer_fired = false;
            }

            pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, count_top);
            pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, 0);
            pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, count_top);
        }
        else if (userInput == 'y')
        {
            if (timer_fired)
            {
                cancel_repeating_timer(&timer);
                timer_fired = false;
            }

            pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, count_top);
            pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, count_top);
            pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, 0);
        }
        else if (userInput == '0')
        {
            if (timer_fired)
            {
                cancel_repeating_timer(&timer);
                timer_fired = false;
            }

            pwm_set_chan_level(slice_num_led_r, PWM_CHAN_B, 0);
            pwm_set_chan_level(slice_num_led_g, PWM_CHAN_B, 0);
            pwm_set_chan_level(slice_num_led_b, PWM_CHAN_B, 0);
        }
    }
    return 0;
}