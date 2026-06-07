
#include <arduino.h>
#include "HeaterControl.h"

void RGBLed(int r, int g, int b);

HeaterControl::HeaterControl() {
  _heater_state = OFF;
  _floor_temp = -999;
  _delay_on = 0;
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
}


bool HeaterControl::thermostatOn() {
  Serial.print("ThermostatOn: ");
  Serial.print(_thermostat_on);
  Serial.print(" min: ");
  Serial.println(_thermostat_min);
  return _thermostat_on && (_thermostat_temp < _thermostat_min);
}

void HeaterControl::iterate(float temperature, float thermostat_temp) {
  //Serial.println(temperature);
  int now = millis() / 1000;
  _thermostat_temp = thermostat_temp;

  if (_prev_time < 1) {
    _prev_time = now;
    _floor_temp = temperature;
    _old_temp = temperature;
    _temp_derivative = 0;
    return;
  }

  _floor_temp = 0.9 * _floor_temp + 0.1 *temperature;
  float delta_temp = 0;
  if (_old_temp != -999) {
    delta_temp = _floor_temp - _old_temp;
    delta_temp *= 60 / (now - _prev_time);
  }
  _temp_derivative = 0.9 * _temp_derivative + 0.1 * delta_temp; /* Low Pass filter*/
  _temp_derivative = delta_temp;
  _old_temp = _floor_temp;
  _prev_time = now;

  switch (_heater_state) {
    case OFF:
      digitalWrite(HEATER_PIN, LOW);
      analogWrite(FAN_PIN, 0);
      if (_temp_derivative > 1.5 || thermostatOn()) {
        if (_restarts > 0) {
          if (++_cycle_mode > 2)
            _cycle_mode = 2;
        } else {
          if (--_cycle_mode < 0)
            _cycle_mode = 0;
        }
        _heat_on = now + _cycle_length[_cycle_mode];
        _heater_state = HEATER_START;
        RGBLed(0, 255, 0);
      }

      if ((_floor_temp - _thermostat_temp) > 10 ) {
        _temp_diff_cnt++;
      } else  {
        _temp_diff_cnt = 0;
      }
      if (_temp_diff_cnt > 900) {
         _heat_on = now + _cycle_length[_cycle_mode];
        _heater_state = HEATER_START;
        _temp_diff_cnt = 0;
        RGBLed(0, 255, 0);

      }
      break;

    case HEATER_START:
      pinMode(HEATER_PIN, OUTPUT);
      digitalWrite(HEATER_PIN, HIGH);
      analogWrite(FAN_PIN, 255);
      _heat_on = now + _cycle_length[_cycle_mode];
      _heater_state = HEATON;
      RGBLed(16, 16, 0);
      break;

    case HEATER_STOP:
      digitalWrite(HEATER_PIN, 0);
      _restarts = 0;
      _delay_on = now + 60;
      _heater_state = IGNORE;
      RGBLed(0, 0, 0);
      break;

    case HEATON:
      pinMode(HEATER_PIN, OUTPUT);
      digitalWrite(HEATER_PIN, HIGH);
      _temp_diff_cnt = 0;
      #if 0
      if ((now % 30) == 0) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(500);
        digitalWrite(BUZZER_PIN, LOW);
      }
      #endif
      if (now > _heat_on) {
        _heat_on = 0;
        _heater_state = HEATER_STOP;
      }
      if (_thermostat_on && (_thermostat_temp > _thermostat_max)) {
        _heat_on = 0;
        _heater_state = HEATER_STOP;
      }
      RGBLed(255, 0, 0);
      break;

    case IGNORE:
      if (now > _delay_on) {
        _delay_on = now + 240;
        _heater_state = DELAY;
      }
      RGBLed(64,64,64);
      break;

/*
  The heater is off during delay state, so any positive temp derivative is due heating by gas. If we sense extra hearting during the delay, we assume
  that more electric heating is needed, so we increase length of the heater on cycle. We have 10/20/30 minute cycle lengths to choose from.
*/

    case DELAY:
      if (now > _delay_on) {
        _delay_on = 0;
        _heater_state = OFF;
        _off_count = 0;
      }

      if ((_temp_derivative > 1.5) || thermostatOn()) {
        _restarts++;
        _heater_state = OFF;
      }
      RGBLed(0, 0, 255);
      break;
  }
}

void HeaterControl::exportState(float &floor_temp, float &temp_derivative, HEATER_STATES &heater_state, float &time_left, int &temp_diff_cnt)
{
  float now = millis() / 1000;
  floor_temp = _floor_temp;
  temp_derivative = _temp_derivative;
  temp_diff_cnt = _temp_diff_cnt;
  heater_state = _heater_state;
  time_left = 0;
  if (_heat_on > 0) {
     time_left = _heat_on - now;
  } else if(_delay_on > 0) {
    time_left = _delay_on - now;
  }
}

void HeaterControl::thermostatCmd(const char* cmd)
{
  char buffer[64];
  char temp1[32];
  char temp2[32];

  float old_min = _thermostat_min;
  float old_max = _thermostat_max;
  char *tmp = (char*) (cmd + strlen("CMD THERMOSTAT "));

  Serial.println(tmp);
  int tmin, tmax;
  sscanf(tmp, "%s", buffer);
  if (strncmp("ON", buffer, 2) == 0) {
    _thermostat_on = true;
    Serial.println(tmp +3);
    sscanf(tmp + 2, "%s %s", temp1, temp2);
    _thermostat_min = atof(temp1);
    _thermostat_max = atof(temp2);
  }
  if (strncmp("OFF", buffer, 3) == 0) {
    _thermostat_on = false;
  }

  
  if (strncmp("DOWN", buffer, 4) == 0) {
    _thermostat_min -= 1;
    _thermostat_max -= 1;
  }

    if (strncmp("UP", buffer, 2) == 0) {
    _thermostat_min += 1;
  }

  if ((_thermostat_max - _thermostat_min) < 2)
    _thermostat_max = _thermostat_min + 2;
  
  Serial.print("Thermostat: ");
  Serial.println(_thermostat_on);
  if ((_thermostat_min != old_min) || (_thermostat_max != old_max)) {
    beep(3);
  }
}

void HeaterControl::beep(int beeps)
{
  for (int i = 0; i < beeps; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
  }
}
