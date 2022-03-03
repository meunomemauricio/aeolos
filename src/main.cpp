/** PID Fan Controller */

#include <AceButton.h>
#include <Arduino.h>
#include <PID_v1.h>

using namespace ace_button;

/** Defines */
#define BAUD_RATE 115200
#define SAMPLE_TIME 22  // ms

// Pins
#define BTN_DECREASE 5
#define BTN_INCREASE 6

#define FAN_CONTROL 3
#define FAN_TACH 8
#define FAN_POWER 10

// TODO: Documment value choice
#define OCRA_VALUE 88

#define MIN_RPM 550
#define MAX_RPM 1750

#define RPN_STEP_N 9  // Number of desired steps, minus 1

#define MIN_ACTUATOR 0
#define MAX_ACTUATOR OCRA_VALUE

/** Global Variables */
// Main Loop Timing
unsigned long previous_millis = 0;

// Fan Variables
bool power_status = false;

const double rpm_step = (MAX_RPM - MIN_RPM) / (double)RPN_STEP_N;
// How many speed steps are active. -1 is turned off;
int8_t fan_steps = -1;

// Buttons
ButtonConfig btn_cfg;
AceButton btn_inc(&btn_cfg, BTN_INCREASE);
AceButton btn_dec(&btn_cfg, BTN_DECREASE);

void handle_btn_event(AceButton*, uint8_t, uint8_t);  // Fwr Ref

// Sensor - Declared as volatile since they can change in interrupts
volatile double rpm_value = 0.0;

// Controller
double setpoint = 0.0;
double output = 0.0;
double kp = 0.15, ki = 0.22, kd = 0.005;

// TODO: Figure out how to solve the volatile warning
PID pid_cntlr(&rpm_value, &output, &setpoint, kp, ki, kd, DIRECT);

// Setup Functions

/** Setup Fan and Tachometer pins. */
void setup_pins() {
  pinMode(BTN_DECREASE, INPUT_PULLUP);
  pinMode(BTN_INCREASE, INPUT_PULLUP);

  pinMode(FAN_POWER, OUTPUT);
  digitalWrite(FAN_POWER, LOW);  // Make sure the FAN is off when booting

  pinMode(FAN_CONTROL, OUTPUT);
  pinMode(FAN_TACH, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

/** Setup the PWM to control the fans.

   TOP is set as OCRA in order to achieve 22.5 KHz:
             Clk              16MHz
    OCRA = -------- - 1 = ------------- - 1 = 87,88... ~= 88
            Ps * f         8 * 22.5KHz
 */
void setup_pwm() {
  // Timer 2 - Fast PWM - Non Inverting -
  TCCR2A = bit(COM2B1) | bit(WGM21) | bit(WGM20);
  // TOP OCRA - Clk/8
  TCCR2B = bit(WGM22) | bit(CS21);
  OCR2A = OCRA_VALUE;
  OCR2B = 0;
}

/** Setup Timer 1 for Input Capture.
 *
 * Used to capture transitions from the Fan Speed Sensor.
 */
void setup_input_capture() {
  TCCR1A = 0;
  // Input Capture Noise Canceler + Rising Edge + Clk/64
  TCCR1B = bit(ICNC1) | bit(ICES1) | bit(CS11) | bit(CS10);
  TCCR1C = 0;

  // Clear ready and enable interrupt
  TIFR1 = bit(ICF1);
  TIMSK1 = bit(ICIE1);
}

/** Setup the PID Controller. */
void setup_controller() {
  pid_cntlr.SetMode(AUTOMATIC);
  pid_cntlr.SetControllerDirection(DIRECT);
  pid_cntlr.SetOutputLimits(MIN_ACTUATOR, MAX_ACTUATOR);
  pid_cntlr.SetSampleTime(SAMPLE_TIME);
}

/** Main Setup function */
void setup() {
  Serial.begin(BAUD_RATE);
  setup_pins();
  setup_pwm();
  setup_input_capture();
  setup_controller();
  sei();

  btn_cfg.setEventHandler(handle_btn_event);
}

// Interrupt Routines

/** Convert to RPM.
 *
 * Convert the values from the Input Compare to an RPM value.
 * TODO: Write formula;
 * TODO: Optimize?
 * TODO: Is this even accurate?
 */
double convert_to_rpm(unsigned int duration) {
  return 7500000.0 / (double)duration;
}

/** Interrupt function for Capture Events. */
ISR(TIMER1_CAPT_vect) {
  rpm_value = convert_to_rpm(ICR1);
  // Reset the Counter and clear the interrupt
  TCNT1 = 0;
  TIFR1 |= bit(ICF1);
}

// FAN Control Functions

/** Turn the Fan Power OFF. */
void fan_off() {
  digitalWrite(FAN_POWER, LOW);
  bitClear(TIMSK1, ICIE1);    // Disable sensor interrupt
  pid_cntlr.SetMode(MANUAL);  // Disable controller while powered off
  setpoint = 0;
  OCR2B = 0;
  rpm_value = 0;
  power_status = false;
}

/** Turn the Fan Power ON.
 *
 * The Setpoint is reset to MIN_RPM when turning it on.
 */
void fan_on() {
  bitSet(TIMSK1, ICIE1);  // Reenable sensor interrupt
  digitalWrite(FAN_POWER, HIGH);
  setpoint = MIN_RPM;
  pid_cntlr.SetMode(AUTOMATIC);  // Reenable the controller
  power_status = true;
}

/** Button Event Handler.  */
void handle_btn_event(AceButton* btn, uint8_t event_type, uint8_t) {
  if (event_type != AceButton::kEventReleased) {
    return;
  }

  if (btn->getPin() == BTN_DECREASE) {
    fan_steps--;
  } else {
    fan_steps++;
  }

  fan_steps = constrain(fan_steps, -1, RPN_STEP_N);

  if (power_status && fan_steps < 0) {
    fan_off();
  } else if (!power_status && fan_steps >= 0) {
    fan_on();
  } else if (fan_steps >= 0) {
    setpoint = MIN_RPM + (fan_steps * rpm_step);
  }
}

// Main Loop Functions

/** Check the buttons, triggering events if necessary. */
void check_buttons(void) {
  btn_dec.check();
  btn_inc.check();
}

/** Print current speed and setpoint to the serial. */
void print_speed(void) {
  Serial.print(rpm_value);
  Serial.print(",");
  Serial.println(setpoint);
}

/** Compute the PID Controller Logic.
 *
 * Function is rate limited by the SAMPLE_TIME;
 */
void process_controller(void) {
  unsigned long current_millis = millis();
  if (current_millis - previous_millis >= SAMPLE_TIME) {
    previous_millis = current_millis;
    print_speed();

    if (power_status) {
      pid_cntlr.Compute();
      OCR2B = output;
    }
  }
}

/** Main Loop */
void loop() {
  check_buttons();
  process_controller();
}
