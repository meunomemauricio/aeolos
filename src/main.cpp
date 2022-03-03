/** PID Fan Controller */

#include <PID_v1.h>

/** Defines */
#define BAUD_RATE 9600
#define SAMPLE_TIME 33 // ms

// Pins
#define BTN_MINUS 5
#define BTN_PLUS 6
#define FAN_CONTROL 3
#define FAN_TACH 8
#define FAN_POWER 10

// TODO: Documment value choice
#define OCRA_VALUE 88

#define MIN_RPM 750
#define MAX_RPM 3800

#define MIN_ACTUATOR 0
#define MAX_ACTUATOR OCRA_VALUE

/** Global Variables */
// Main Loop Timing
unsigned long previous_millis = 0;

// Power
bool power_status = false;

// Sensor - Declared as volatile since they can change in interrupts
volatile double rpm_value = 0.0;

// Controller
double setpoint = MIN_ACTUATOR;
double output = 0.0;
double kp = 0.01343, ki = 0.02526, kd = 0.00397;

// TODO: Figure out how to solve the volatile warning
PID pid_cntlr(&rpm_value, &output, &setpoint, kp, ki, kd, DIRECT);

/** Setup Fan and Tachometer pins. */
void setup_pins()
{
  pinMode(FAN_POWER, OUTPUT);
  digitalWrite(FAN_POWER, LOW); // Make sure the FAN is off when booting

  pinMode(FAN_CONTROL, OUTPUT);
  pinMode(FAN_TACH, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(BTN_MINUS, INPUT_PULLUP);
  pinMode(BTN_PLUS, INPUT_PULLUP);
}

/** Convert to RPM.
 *
 * Convert the values from the Input Compare to an RPM value.
 * TODO: Write formula;
 * TODO: Optimize?
 * TODO: Is this even accurate?
 */
double convert_to_rpm(unsigned int duration)
{
  return 7500000.0 / (double)duration;
}

/** Setup the PWM to control the fans.

   TOP is set as OCRA in order to achieve 22.5 KHz:
             Clk              16MHz
    OCRA = -------- - 1 = ------------- - 1 = 87,88... ~= 88
            Ps * f         8 * 22.5KHz
 */
void setup_pwm()
{
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
void setup_input_capture()
{
  TCCR1A = 0;
  // Input Capture Noise Canceler + Rising Edge + Clk/64
  TCCR1B = bit(ICNC1) | bit(ICES1) | bit(CS11) | bit(CS10);
  TCCR1C = 0;

  // Clear ready and enable interrupt
  TIFR1 = bit(ICF1);
  TIMSK1 = bit(ICIE1);
}

/** Setup the PID Controller. */
void setup_controller()
{
  pid_cntlr.SetMode(AUTOMATIC);
  pid_cntlr.SetControllerDirection(DIRECT);
  pid_cntlr.SetOutputLimits(MIN_ACTUATOR, MAX_ACTUATOR);
  pid_cntlr.SetSampleTime(SAMPLE_TIME);
  setpoint = MIN_RPM;
}

/** Turn the Fan Power OFF. */
void fan_off()
{
  digitalWrite(FAN_POWER, LOW);
  bitClear(TIMSK1, ICIE1);   // Disable sensor interrupt
  pid_cntlr.SetMode(MANUAL); // Disable controller while powered off
  setpoint = MIN_RPM;
  OCR2B = 0;
  rpm_value = 0;
  power_status = false;
}

/** Turn the Fan Power ON.
 *
 * The Setpoint is reset to MIN_RPM when turning it on.
 */
void fan_on()
{
  bitSet(TIMSK1, ICIE1); // Reenable sensor interrupt
  digitalWrite(FAN_POWER, HIGH);
  pid_cntlr.SetMode(AUTOMATIC); // Reenable the controller
  power_status = true;
}

/** Setup function */
void setup()
{
  Serial.begin(BAUD_RATE);
  setup_pins();
  setup_pwm();
  setup_input_capture();
  setup_controller();
  sei();
}

/** Interrupt function for Capture Events. */
ISR(TIMER1_CAPT_vect)
{
  rpm_value = convert_to_rpm(ICR1);
  // Reset the Counter and clear the interrupt
  TCNT1 = 0;
  TIFR1 |= bit(ICF1);
}

/** Main Loop */
void loop() {
  unsigned long current_millis = millis();
  if (current_millis - previous_millis >= SAMPLE_TIME) {
    previous_millis = current_millis;

    if (power_status) {
      pid_cntlr.Compute();
      OCR2B = output;
    }
  }
}
