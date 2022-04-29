/**
 * @file rev1_direction_module.ino
 * 
 * @author Joseph Telaak
 * 
 * @brief Code for the simple motor controller module
 * 
 * @version 0.1
 * @date 2022-02-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "mcp2515.h"
#include "Encoder.h"

// Steering Motor Ctrl
#define STR_L_PWM 9
#define STR_R_PWM 6
#define STR_ENABLE 4

// Steering Linear Actuator Potentiometer
#define STR_POT A0

// Steering Wheel Input Encoder
#define STR_WHL_ENC 3
#define STR_WHL_ENC2 A2

// Brake Motor Ctrl
#define BRK_L_PWM 5
#define BRK_R_PWM 10
#define BRK_ENABLE 7

// Brake Actuator Potentiometer
#define BRK_POT A1

// CAN
#define CAN_CS 8
#define CAN_INT 2
#define CAN_DLC 8
#define CAN_ID 0x002

MCP2515 can(CAN_CS);
Encoder wheel_enc(STR_WHL_ENC, STR_WHL_ENC2);

void setup() {
    can.reset();
    can.setBitrate(CAN_125KBPS);
    can.setNormalMode();

    attachInterrupt(digitalPinToInterrupt(CAN_INT), can_irq, FALLING);

    noInterrupts();

    pinMode(STR_ENABLE, OUTPUT);
    pinMode(STR_L_PWM, OUTPUT);
    pinMode(STR_R_PWM, OUTPUT);
    pinMode(STR_POT, INPUT);

    digitalWrite(STR_ENABLE, LOW);
    digitalWrite(STR_L_PWM, LOW);
    digitalWrite(STR_R_PWM, LOW);

    pinMode(BRK_ENABLE, OUTPUT);
    pinMode(BRK_L_PWM, OUTPUT);
    pinMode(BRK_R_PWM, OUTPUT);
    pinMode(BRK_POT, INPUT);

    digitalWrite(BRK_ENABLE, LOW);
    digitalWrite(BRK_L_PWM, LOW);
    digitalWrite(BRK_R_PWM, LOW);

    interrupts();

}

void loop() {
    read_brk_pot();
    read_brk_stat();
    read_str_pot();
    read_str_stat();
    read_str_whl();

    delay(2000);

}

void can_irq() {
    struct can_frame can_msg_in;

    if (can.readMessage(&can_msg_in) == MCP2515::ERROR_OK) {
        if (can_msg_in.can_id == CAN_ID) {
            if (can_msg_in.data[0] == 0x0A) {
                if (can_msg_in.data[1] == 0x01) {
                    if (can_msg_in.data[2] == 0x0A) {
                        if (can_msg_in.data[3] == 0x01) {
                            digitalWrite(STR_ENABLE, LOW);
                            digitalWrite(STR_L_PWM, LOW);
                            digitalWrite(STR_R_PWM, LOW);

                        } else if (can_msg_in.data[3] == 0x02) {
                            digitalWrite(STR_ENABLE, HIGH);

                        }

                        read_str_stat();

                    } else if (can_msg_in.data[2] == 0x0C) {
                        if (can_msg_in.data[3] == 0x01) {
                            analogWrite(STR_L_PWM, can_msg_in.data[4]);
                            digitalWrite(STR_R_PWM, LOW);

                        } else if (can_msg_in.data[3] == 0x02) {
                            digitalWrite(STR_L_PWM, LOW);
                            analogWrite(STR_R_PWM, can_msg_in.data[4]);

                        }

                        read_str_pot();

                    }

                } else if (can_msg_in.data[1] == 0x02) {
                    if (can_msg_in.data[2] == 0x0A) {
                        if (can_msg_in.data[3] == 0x01) {
                            digitalWrite(BRK_ENABLE, LOW);
                            digitalWrite(BRK_L_PWM, LOW);
                            digitalWrite(BRK_R_PWM, LOW);

                        } else if (can_msg_in.data[3] == 0x02) {
                            digitalWrite(BRK_ENABLE, HIGH);

                        }

                        read_brk_stat();

                    } else if (can_msg_in.data[2] == 0x0C) {
                        if (can_msg_in.data[3] == 0x01) {
                            analogWrite(BRK_L_PWM, can_msg_in.data[4]);
                            digitalWrite(BRK_R_PWM, LOW);

                        } else if (can_msg_in.data[3] == 0x02) {
                            digitalWrite(BRK_L_PWM, LOW);
                            analogWrite(BRK_R_PWM, can_msg_in.data[4]);

                        }

                        read_brk_pot();

                    }
                }

            } else if (can_msg_in.data[0] == 0x0C) {
                if (can_msg_in.data[1] == 0x01) {
                    if (can_msg_in.data[2] == 0x0A)
                        read_str_stat();
                    else if (can_msg_in.data[3] == 0x0E) 
                        read_str_whl();
                    else if (can_msg_in.data[3] == 0x0F)
                        read_str_pot();

            
                } else if (can_msg_in.data[2] == 0x02) {
                    if (can_msg_in.data[2] == 0x0A) 
                        read_brk_stat();
                    else if (can_msg_in.data[2] == 0x0F) 
                        read_brk_pot();

                }
            }
        }
    }
}

void read_brk_pot() {
    int pot_value = analogRead(BRK_POT);

    struct can_frame can_msg_out;

    can_msg_out.can_id = CAN_ID;
    can_msg_out.can_dlc = CAN_DLC;
    can_msg_out.data[0] = 0x0C;
    can_msg_out.data[1] = 0x0C;
    can_msg_out.data[2] = 0x01;
    can_msg_out.data[3] = 0x0F;
    can_msg_out.data[4] = 0;
    can_msg_out.data[5] = 0;
    can_msg_out.data[6] = pot_value >> 8;
    can_msg_out.data[7] = pot_value & 0xFF;

    can.sendMessage(&can_msg_out);

}

void read_brk_stat() {
    struct can_frame can_msg_out;

    can_msg_out.can_id = CAN_ID;
    can_msg_out.can_dlc = CAN_DLC;
    can_msg_out.data[0] = 0x0C;
    can_msg_out.data[1] = 0x0C;
    can_msg_out.data[2] = 0x02;
    can_msg_out.data[3] = 0x0A;
    can_msg_out.data[4] = 0;
    can_msg_out.data[5] = 0;
    can_msg_out.data[6] = 0;
    can_msg_out.data[7] = digitalRead(STR_ENABLE) == HIGH ? 0x01 : 0x02;

    can.sendMessage(&can_msg_out);

}

void read_str_pot() {
    int pot_value = analogRead(STR_POT);

    struct can_frame can_msg_out;

    can_msg_out.can_id = CAN_ID;
    can_msg_out.can_dlc = CAN_DLC;
    can_msg_out.data[0] = 0x0C;
    can_msg_out.data[1] = 0x0C;
    can_msg_out.data[2] = 0x01;
    can_msg_out.data[3] = 0x0F;
    can_msg_out.data[4] = 0;
    can_msg_out.data[5] = 0;
    can_msg_out.data[6] = pot_value >> 8;
    can_msg_out.data[7] = pot_value & 0xFF;

    can.sendMessage(&can_msg_out);

}

void read_str_stat() {
    struct can_frame can_msg_out;

    can_msg_out.can_id = CAN_ID;
    can_msg_out.can_dlc = CAN_DLC;
    can_msg_out.data[0] = 0x0C;
    can_msg_out.data[1] = 0x0C;
    can_msg_out.data[2] = 0x01;
    can_msg_out.data[3] = 0x0A;
    can_msg_out.data[4] = 0;
    can_msg_out.data[5] = 0;
    can_msg_out.data[6] = 0;
    can_msg_out.data[7] = digitalRead(STR_ENABLE) == HIGH ? 0x01 : 0x02;

    can.sendMessage(&can_msg_out);

}

long old_pos = -999;

void read_str_whl() {
    long pos = wheel_enc.read();
    uint8_t change_id = 0x01;

    if (pos > old_pos)
        change_id = 0x02;
    else if (pos < old_pos)
        change_id = 0x03;

    struct can_frame can_msg_out;

    can_msg_out.can_id = CAN_ID;
    can_msg_out.can_dlc = CAN_DLC;
    can_msg_out.data[0] = 0x0C;
    can_msg_out.data[1] = 0x0C;
    can_msg_out.data[2] = 0x01;
    can_msg_out.data[3] = 0x0E;
    can_msg_out.data[4] = 0;
    can_msg_out.data[5] = 0;
    can_msg_out.data[6] = 0;
    can_msg_out.data[7] = change_id;

    can.sendMessage(&can_msg_out);

}