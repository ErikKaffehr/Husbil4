#ifndef HEATERCONTROL
#define HEATERCONTROL


/* Theater can go into at few discrete states*/
typedef enum heater_states{ OFF, HEATER_START, HEATON, HEATER_STOP, IGNORE, DELAY } HEATER_STATES;

#define HIGH 1   	/* Heater ON */
#define LOW 0			/* Heater OFF*/
#define HEATER_PIN 3		/* OUTPUT PIN controlling HEATER */
#define BUZZER_PIN 19 /* should be 19 */
#define FAN_PIN 4

/*
This class controls the heater, based on floor temperatur. 
It has two methods:
   iterate(fllor_temp) is called by 'loop' in the main programs and controls the heater based on floor temperature
	 exportState(...) reports data for plotting to main program
*/

class HeaterControl {
public:
	HeaterControl();  /* This is called contructor and it creates a new instance of this class */
	void iterate(float floor_temp, float thermostat_temp); /* iterate causes update to the class*/
  void exportState(float &floor_temp, float &temp_derivative, HEATER_STATES &HeaterState, float &time_left, int &temp_diff_cnt); /* Exports data for plotting */
	void thermostatCmd(const char *cmd);
	bool thermostatOn();
	void beep(int beeps);

private: /* Thiese data are variables for this class and hidden from the outside */
  HEATER_STATES _heater_state;
	float _old_temp;
	float _floor_temp;
	float _temp_derivative;
	int _delay_on;
	int _off_count;
	int _cycle_mode;
	int _restarts;
	int _heat_on;
	float _prev_time = 0.0;
	int _cycle_length[3] = { 600, 1200, 1800};
	bool _thermostat_on = false;
	float _thermostat_min = 22;
	float _thermostat_max = 24;
	float _thermostat_temp = 0;
	int _temp_diff_cnt = 0;
};




#endif // HEATERCONTROL