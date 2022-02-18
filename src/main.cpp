/** PID Fan Controller */

#include <CmdMessenger.h>
#include <PID_v1.h>

/** Defines */
#define BAUD_RATE 9600
#define SAMPLE_TIME 33 // ms

// Pins
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

// Cmd Messenger Definition
enum commands
{
  SYN,
  ID,
  ACK,
  ERROR,
  // Module specific.
  READ_ACTUATOR_REQ,
  READ_ACTUATOR_RESP,
  READ_PARAMETERS_REQ,
  READ_PARAMETERS_RESP,
  READ_SENSOR_REQ,
  READ_SENSOR_RESP,
  READ_SETPOINT_REQ,
  READ_SETPOINT_RESP,
  READ_STATE_REQ,
  READ_STATE_RESP,
  TOGGLE_FAN,
  UPDATE_PARAMETERS,
  UPDATE_SETPOINT,
};

CmdMessenger messenger = CmdMessenger(Serial);

/** Setup Fan and Tachometer pins. */
void setup_pins()
{
  pinMode(FAN_POWER, OUTPUT);
  digitalWrite(FAN_POWER, LOW); // Make sure the FAN is off when booting

  pinMode(FAN_CONTROL, OUTPUT);
  pinMode(FAN_TACH, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

/** Convert to RPM.
 *
 * Convert the values from the Input Compare to an RPM value.
 * TODO: Write formula;
 * TODO: Optimize?
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

/** Setup Timer 1 for Input Capture. */
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

/** Reply the SYN command with the Module ID. */
void handle_handshake()
{
  messenger.sendBinCmd(ID, (byte)MODULE_ID);
}

/** Send the actuator value through the serial port.*/
void read_actuator()
{
  messenger.sendBinCmd(READ_ACTUATOR_RESP, (byte)OCR2B);
}

/** Send the controller parameters through the serial port.*/
void read_parameters()
{
  messenger.sendCmdStart(READ_PARAMETERS_RESP);
  messenger.sendCmdBinArg<double>(kp);
  messenger.sendCmdBinArg<double>(ki);
  messenger.sendCmdBinArg<double>(kd);
  messenger.sendCmdEnd();
}

/** Send the Sensor value (in RPMs) through the serial port. */
void read_sensor()
{
  messenger.sendBinCmd(READ_SENSOR_RESP, (double)rpm_value);
}

/** Send the setpoint value through the serial port. */
void read_setpoint()
{
  messenger.sendBinCmd(READ_SETPOINT_RESP, (double)setpoint);
}

/** Send the status of the Fan Power. */
void read_status()
{
  messenger.sendBinCmd(READ_STATE_RESP, power_status);
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

/** Togle the Fan Power based on the command argument. */
void toggle_fan()
{
  bool turn_on = messenger.readBinArg<bool>();
  digitalWrite(LED_BUILTIN, turn_on ? HIGH : LOW);
  (turn_on ? fan_on : fan_off)();
  messenger.sendCmd(ACK);
}

/** Apply the command arguments as the controller Parameters. */
void update_parameters()
{
  kp = messenger.readBinArg<double>();
  ki = messenger.readBinArg<double>();
  kd = messenger.readBinArg<double>();
  pid_cntlr.SetTunings(kp, ki, kd);
  messenger.sendCmd(ACK);
}

/** Apply the command argument as SetPoint. */
void update_setpoint()
{
  setpoint = messenger.readBinArg<double>();
  messenger.sendCmd(ACK);
}

/** Attach the Messenger Callbacks. */
void setup_messenger_callbacks()
{
  messenger.attach(SYN, handle_handshake);
  messenger.attach(READ_ACTUATOR_REQ, read_actuator);
  messenger.attach(READ_PARAMETERS_REQ, read_parameters);
  messenger.attach(READ_SENSOR_REQ, read_sensor);
  messenger.attach(READ_SETPOINT_REQ, read_setpoint);
  messenger.attach(READ_STATE_REQ, read_status);
  messenger.attach(TOGGLE_FAN, toggle_fan);
  messenger.attach(UPDATE_PARAMETERS, update_parameters);
  messenger.attach(UPDATE_SETPOINT, update_setpoint);
}

/** Setup function */
void setup()
{
  Serial.begin(BAUD_RATE);
  setup_pins();
  setup_pwm();
  setup_input_capture();
  setup_controller();
  setup_messenger_callbacks();
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

/** Serial Event Loop. */
void serialEvent()
{
  messenger.feedinSerialData();
}

/** Main Loop */
void loop()
{
  unsigned long current_millis = millis();
  if (current_millis - previous_millis >= SAMPLE_TIME)
  {
    previous_millis = current_millis;
    if (power_status)
    {
      pid_cntlr.Compute();
      OCR2B = output;
    }
  }
}
