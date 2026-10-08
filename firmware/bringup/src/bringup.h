#pragma once

// Each peripheral module configures its hardware here and registers its
// own shell commands. Return 0 or a negative errno.
int leds_init();
int motor_init();
int servo_init();
int ultrasonic_init();
int analog_init();
int encoder_init();
int ble_init();

void leds_self_test();
