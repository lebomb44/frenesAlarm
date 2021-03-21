#include <Cmd.h>
#include <GPRS_Shield_Arduino.h>

/* *****************************
 *  Pin allocation
 * *****************************
 */
#define LED_pin 13

#define GSM_TX_pin    14
#define GSM_RX_pin    15
#define GSM_POWER_pin 9

#define MOVE_EXT_AV_pin A9
#define MOVE_ENTRY_pin A13
#define MOVE_CORRIDOR_pin A14
#define MOVE_DINING_pin A15
#define MOVE_PORTE_GARAGE_pin A12
#define MOVE_EXT_PERG_pin A11
#define MOVE_EXT_SAM_pin A10

#define BUZZER_pin 17
#define CMD_pin 16

/* *****************************
 *  Global variables
 * *****************************
 */
GPRS gprs(&Serial3, 19200);
uint32_t gprs_checkPowerUp_task = 0;
uint32_t gprs_checkPowerUp_counter = 0;

#define CMD_NB_MAX 500
uint16_t cmd_nb = 0;
bool cmd_current = false;
bool cmd_previous = false;
#define DETECTOR_NB_MAX 1000
uint16_t detector_nb = 0;
bool detector_bool = false;
bool alarm_started = false;
bool alarm_triggered = false;

uint32_t alarm_time = 0;

/* *****************************
 *  Debug Macros
 * *****************************
 */

bool alarm_printIsEnabled  = true;
#define ALARM_PRINT(m)  if(true == alarm_printIsEnabled) { m }
#define MSG_ALERT "Alerte ! Alarme declanchee: "

/* *****************************
 *  Command lines funstions
 * *****************************
 */

/* GPRS */
void gprsSendSMS(int arg_cnt, char **args) {
  /* > gprsSendSMS 0612345678 sms_text_to_send_without_space */
  if(3 == arg_cnt) {
    Serial.print("Sending SMS...");
    /* Send the SMS: phoneNumber, text */
    if(true == gprs.sendSMS(args[1], args[2])) { Serial.println("OK"); }
    else { Serial.println("ERROR"); }
  }
  else { Serial.println("Incorrect number of arguments"); }
}
void gprsGetSignalStrength(int arg_cnt, char **args) {
  Serial.print("GPRS signal strength...");
  int signalStrength = 0;
  /* Get signal strength: [0..100] % */
  if(true == gprs.getSignalStrength(&signalStrength)) { Serial.print("OK = "); Serial.print(signalStrength); Serial.println(); } else { Serial.println("ERROR"); }
}
void gprsPowerUpDown(int arg_cnt, char **args) { gprs.powerUpDown(GSM_POWER_pin); Serial.println("GPRS power Up-Down done"); }
void gprsCheckPowerUp(int arg_cnt, char **args) { Serial.print("GPRS check power up..."); if(true == gprs.checkPowerUp()) { Serial.println("OK"); } else { Serial.println("ERROR"); } }
void gprsInit(int arg_cnt, char **args) { Serial.print("GPRS init..."); if(true == gprs.init()) { Serial.println("OK"); } else { Serial.println("ERROR"); } }
void gprsEnablePrint(int arg_cnt, char **args) { alarm_printIsEnabled = true; Serial.println("GPRS print enabled"); }
void gprsDisablePrint(int arg_cnt, char **args) { alarm_printIsEnabled = false; Serial.println("GPRS print disabled"); }

void setup() {
  Serial.begin(115200);
  Serial.println("Mika Alarm Starting...");

  /* ****************************
   *  Pin configuration
   * ****************************
   */
  pinMode(LED_pin, OUTPUT); 

  pinMode(GSM_TX_pin, OUTPUT);
  digitalWrite(GSM_TX_pin, HIGH);
  pinMode(GSM_RX_pin, INPUT_PULLUP);
  pinMode(GSM_POWER_pin, OUTPUT);
  digitalWrite(GSM_POWER_pin, LOW);

  pinMode(MOVE_EXT_AV_pin, INPUT);
  pinMode(MOVE_ENTRY_pin, INPUT_PULLUP);
  pinMode(MOVE_CORRIDOR_pin, INPUT_PULLUP);
  pinMode(MOVE_DINING_pin, INPUT_PULLUP);
  pinMode(MOVE_PORTE_GARAGE_pin, INPUT_PULLUP);
  pinMode(MOVE_EXT_SAM_pin, INPUT_PULLUP);
  pinMode(MOVE_EXT_PERG_pin, INPUT_PULLUP);

  pinMode(BUZZER_pin, OUTPUT);
  pinMode(CMD_pin, INPUT_PULLUP);

  /* ****************************
   *  Modules initialization
   * ****************************
   */

  gprs.init();

  cmdInit();

  /* ***********************************
   *  Register commands for command line
   * ***********************************
   */
  cmdAdd("gprsSendSMS", "Send SMS", gprsSendSMS);
  cmdAdd("gprsGetSignal", "Get GPRS signal strength", gprsGetSignalStrength);
  cmdAdd("gprsPowerUpDown", "GPRS power up-down", gprsPowerUpDown);
  cmdAdd("gprsCheckPowerUp", "Check GPRS power up", gprsCheckPowerUp);
  cmdAdd("gprsInit", "Initialize GPRS", gprsInit);
  cmdAdd("gprsEnablePrint", "Enable print in GPRS lib", gprsEnablePrint);
  cmdAdd("gprsDisablePrint", "Disable print in GPRS lib", gprsDisablePrint);
  cmdAdd("help", "List commands", cmdList);

  Serial.println("Mika Alarm Init done");
}

void loop() {
  if(LOW == digitalRead(CMD_pin)) {
    if(cmd_nb < CMD_NB_MAX) { cmd_nb++; }
  }
  else {
    if(0 < cmd_nb) { cmd_nb--; }
  }
  if(cmd_nb > CMD_NB_MAX / 2) {
    cmd_current = true;
  }
  else {
    cmd_current = false;
  }
  /*
  ALARM_PRINT( Serial.print("-ExtAV    : "); Serial.println(digitalRead(MOVE_EXT_AV_pin)); )
  ALARM_PRINT( Serial.print("-Entry    : "); Serial.println(digitalRead(MOVE_ENTRY_pin)); )
  ALARM_PRINT( Serial.print("-Corridor : "); Serial.println(digitalRead(MOVE_CORRIDOR_pin)); )
  ALARM_PRINT( Serial.print("-Dining   : "); Serial.println(digitalRead(MOVE_DINING_pin)); )
  ALARM_PRINT( Serial.print("-PorteGar : "); Serial.println(digitalRead(MOVE_PORTE_GARAGE_pin)); )
  ALARM_PRINT( Serial.print("-ExtSaM   : "); Serial.println(!digitalRead(MOVE_EXT_SAM_pin)); )
  ALARM_PRINT( Serial.print("-ExtPerg  : "); Serial.println(!digitalRead(MOVE_EXT_PERG_pin)); )
  */

  if((HIGH == digitalRead(MOVE_EXT_AV_pin)) \
  || (HIGH == digitalRead(MOVE_ENTRY_pin)) \
  || (HIGH == digitalRead(MOVE_CORRIDOR_pin)) \
  || (HIGH == digitalRead(MOVE_DINING_pin)) \
  || (HIGH == digitalRead(MOVE_PORTE_GARAGE_pin)) \
  || (LOW == digitalRead(MOVE_EXT_SAM_pin)) \
  || (LOW == digitalRead(MOVE_EXT_PERG_pin))) {
    if(detector_nb < DETECTOR_NB_MAX) { detector_nb++; }
  }
  else {
    if(0 < detector_nb) { detector_nb--; }
  }
  if(detector_nb > DETECTOR_NB_MAX / 2) {
    detector_bool = true;
  }
  else {
    detector_bool = false;
  }

  if(true == cmd_current) {
    if(false == cmd_previous) {
      ALARM_PRINT( Serial.println("Alarm ON"); )
      //digitalWrite(BUZZER_pin, HIGH);
      delay(200);
      digitalWrite(BUZZER_pin, LOW);
      alarm_triggered = false;
    }
    cmd_previous = true;

    if(true == detector_bool) {
      if(true == alarm_started) {
        if(false == alarm_triggered) {
          ALARM_PRINT( Serial.print("ExtAV    : "); Serial.println(digitalRead(MOVE_EXT_AV_pin)); )
          ALARM_PRINT( Serial.print("Entry    : "); Serial.println(digitalRead(MOVE_ENTRY_pin)); )
          ALARM_PRINT( Serial.print("Corridor : "); Serial.println(digitalRead(MOVE_CORRIDOR_pin)); )
          ALARM_PRINT( Serial.print("Dining   : "); Serial.println(digitalRead(MOVE_DINING_pin)); )
          ALARM_PRINT( Serial.print("PorteGar : "); Serial.println(digitalRead(MOVE_PORTE_GARAGE_pin)); )
          ALARM_PRINT( Serial.print("ExtSaM   : "); Serial.println(!digitalRead(MOVE_EXT_SAM_pin)); )
          ALARM_PRINT( Serial.print("ExtPerg  : "); Serial.println(!digitalRead(MOVE_EXT_PERG_pin)); )

          char msg[sizeof(MSG_ALERT)+10] = {0};
          memset(msg, 0, sizeof(msg));
          strcpy(msg, MSG_ALERT);
          if(HIGH == digitalRead(MOVE_EXT_AV_pin)) { strcat(msg, "ExtAV"); }
          else if(HIGH == digitalRead(MOVE_ENTRY_pin)) { strcat(msg, "Entree"); }
          else if(HIGH == digitalRead(MOVE_CORRIDOR_pin)) { strcat(msg, "Corridor"); }
          else if(HIGH == digitalRead(MOVE_DINING_pin)) { strcat(msg, "Salon"); }
          else if(HIGH == digitalRead(MOVE_PORTE_GARAGE_pin)) { strcat(msg, "PorteGar"); }
          else if(LOW == digitalRead(MOVE_EXT_SAM_pin)) { strcat(msg, "ExtSaM"); }
          else if(LOW == digitalRead(MOVE_EXT_PERG_pin)) { strcat(msg, "ExtPerg"); }
          ALARM_PRINT( Serial.println(msg); )

          digitalWrite(BUZZER_pin, HIGH);
          ALARM_PRINT( Serial.print("Sending SMS..."); )
          if(true == gprs.sendSMS("0602732751", msg)) {
            ALARM_PRINT( Serial.println("OK"); )
          }
          else {
            ALARM_PRINT( Serial.println("ERROR"); )
          }
        }
        alarm_triggered = true;
      }
    }
    else {
      if(false == alarm_started) {
        ALARM_PRINT( Serial.println("Alarm detection started"); )
      }
      alarm_started = true;
    }
  }
  else {
    if(true == cmd_previous) {
      ALARM_PRINT( Serial.println("Alarm OFF"); )
    }
    digitalWrite(BUZZER_pin, LOW);
    cmd_previous = false;
    alarm_started = false;
    alarm_triggered = false;
  }
  if(true == alarm_triggered) {
    if(alarm_time < 500000) {
      digitalWrite(BUZZER_pin, HIGH);
      if(alarm_time == 0) {
        ALARM_PRINT( Serial.println("Buzzer ON"); )
      }
      alarm_time++;
    }
    else {
      digitalWrite(BUZZER_pin, LOW);
      ALARM_PRINT( Serial.println("Buzzer OFF"); )
      alarm_time = 0;
      alarm_triggered = false;
    }
  }
  else {
    digitalWrite(BUZZER_pin, LOW);
    alarm_time = 0;
  }

  gprs_checkPowerUp_task++;
  /* Check GPRS power every 10 seconds */
  if(100000 < gprs_checkPowerUp_task) {
    /* Initialiaze counter for the new cycle */
    gprs_checkPowerUp_task=0;
    /* Power Up cycle */
    gprs_checkPowerUp_counter++;
    /* Get the signal strength and set it in the message */
    /* But also reset the power Up timeout */
    ALARM_PRINT( Serial.print("Checking GPRS signal strength..."); )
    int signalStrengthValue = 0;
    if(true == gprs.getSignalStrength(&signalStrengthValue)) {
      gprs_checkPowerUp_counter = 0;
      ALARM_PRINT( Serial.println("OK"); )
    }
    else {
      ALARM_PRINT( Serial.println("ERROR"); )
    }
    /* Timeout ! */
    /* We have to turn ON the GPRS shield after 6 attempts */
    if(6 < gprs_checkPowerUp_counter) {
      /* Initialiaze counter for the new cycle */
      gprs_checkPowerUp_counter = 0;
      ALARM_PRINT( Serial.print("Powering up GPRS..."); )
      /* Toggle the power og the GPRS shield */
      gprs.powerUpDown(GSM_POWER_pin);
      ALARM_PRINT( Serial.println("done"); )
      ALARM_PRINT( Serial.print("GPRS init..."); )
      /* Try to initialize thr shield */
      if(true == gprs.init()) { ALARM_PRINT( Serial.println("OK"); ) } else { ALARM_PRINT( Serial.println("ERROR"); ) }
    }
  }

  /* Poll for new command line */
  cmdPoll();
  /* Wait a minimum for cyclic task */
  delay(1);
}
