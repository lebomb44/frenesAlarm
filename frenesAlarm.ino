#include <Cmd.h>
#include <GPRS_Shield_Arduino.h>

/* *****************************
 *  Pin allocation
 * *****************************
 */
#define LED_pin 13

#define GSM_TX_pin    16
#define GSM_RX_pin    17
#define GSM_POWER_pin 9

#define CMD_pin 19

/* *****************************
 *  Global variables
 * *****************************
 */
GPRS gprs(&Serial2, 19200);
uint32_t gprs_checkPowerUp_task = 0;
uint32_t gprs_checkPowerUp_counter = 0;

#define CMD_NB_MAX 500
uint16_t cmd_nb = 0;
bool cmd_current = false;
bool cmd_previous = false;

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
  Serial.println("Frenes Alarm Starting...");

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

  Serial.println("Frenes Alarm Init done");
}

void loop() {
  if(HIGH == digitalRead(CMD_pin)) {
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
    cmd_previous = false;
  }

  if(true == cmd_current) {
    if(false == cmd_previous) {
      ALARM_PRINT( Serial.println("Alarm ON"); )
      ALARM_PRINT( Serial.print("Sending SMS..."); )
      if(true == gprs.sendSMS("+33689350159", "Alarme Frenes !")) {
        ALARM_PRINT( Serial.println("OK"); )
      }
      else {
        ALARM_PRINT( Serial.println("ERROR"); )
      }
    }
    cmd_previous = true;
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
