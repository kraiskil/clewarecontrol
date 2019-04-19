// SVN: $Revision: 61 $
#include <errno.h>
#include <iostream>
#include <libintl.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "error.h"
#include "USBaccess.h"

#define _ gettext

typedef enum { O_BRIEF, O_FULL, O_SS } output_t;
output_t ot = O_FULL;

int retry_count = 10;
double slp = 0.5;
CUSBaccess CWusb;
int USBcount = -1;
bool initted = false;
bool abrt = false;
const char *ts = NULL;
FILE *fh = NULL;

const char *limit_exec_min = NULL, *limit_exec_max = NULL;
bool limit_min_triggered = false, limit_max_triggered = false;
double limit_min = 0.0, limit_max = 0.0;

void sigh(int sig)
{
	(void) sig;
	abrt = true;

	fprintf(stderr, _("Aborting program\n"));
}

std::string format(const char *fmt, ...)
{
	char *buffer = NULL;
	va_list ap;
	int rv;
 
        va_start(ap, fmt);
	rv = vasprintf(&buffer, fmt, ap);
        va_end(ap);
	(void)rv;

	std::string result = buffer;
	free(buffer);

	return result;
}

std::string time_to_str(double t_in)
{
        if (t_in <= 0)
                return _("n/a");

        time_t t = (time_t)t_in;
        struct tm *tm = localtime(&t);
        if (!tm)
                error_exit(true, _("localtime(%ld) failed"), (long int)t);

        char time_buffer[128];
        snprintf(time_buffer, sizeof time_buffer, "%04d-%02d-%02dT%02d:%02d:%02d.%03d",
                tm -> tm_year + 1900,
                tm -> tm_mon + 1,
                tm -> tm_mday,
                tm -> tm_hour,
                tm -> tm_min,
                tm -> tm_sec,
                int((t_in - double(t)) * 1000.0) % 1000);

        return std::string(time_buffer);
}

void init()
{
	if (!initted)
	{
		USBcount = CWusb.OpenCleware();

		initted = true;
	}
}

const char * device_id_to_string(int id)
{
	switch(id)
	{
	case CUSBaccess::POWER_DEVICE:
		return "Power";
	case CUSBaccess::WATCHDOGXP_DEVICE:
		return "WatchdogXP";
	case CUSBaccess::LED_DEVICE:
		return "LED";
	case CUSBaccess::WATCHDOG_DEVICE:
		return "Watchdog";
	case CUSBaccess::AUTORESET_DEVICE:
		return "Autoreset device";
	case CUSBaccess::SWITCH1_DEVICE:
		return "Switch1";
	case CUSBaccess::SWITCH2_DEVICE:
		return "Switch2";
	case CUSBaccess::SWITCH3_DEVICE:
		return "Switch3";
	case CUSBaccess::SWITCH4_DEVICE:
		return "Switch4";
	case CUSBaccess::SWITCH5_DEVICE:
		return "Switch5";
	case CUSBaccess::SWITCH6_DEVICE:
		return "Switch6";
	case CUSBaccess::SWITCH7_DEVICE:
		return "Switch7";
	case CUSBaccess::SWITCH8_DEVICE:
		return "Switch8";
	case CUSBaccess::SWITCHX_DEVICE:
		return "SwitchX";
	case CUSBaccess::TEMPERATURE_DEVICE:
		return "Temperature sensor";
	case CUSBaccess::TEMPERATURE2_DEVICE:
		return "Temperature 2 sensor";
	case CUSBaccess::TEMPERATURE5_DEVICE:
		return "Temperature 5 sensor";
	case CUSBaccess::HUMIDITY1_DEVICE:
		return "Humidity sensor";
	case CUSBaccess::CONTACT00_DEVICE:
		return "Contact 00 device";
	case CUSBaccess::CONTACT01_DEVICE:
		return "Contact 01 device";
	case CUSBaccess::CONTACT02_DEVICE:
		return "Contact 02 device";
	case CUSBaccess::CONTACT03_DEVICE:
		return "Contact 03 device";
	case CUSBaccess::CONTACT04_DEVICE:
		return "Contact 04 device";
	case CUSBaccess::CONTACT05_DEVICE:
		return "Contact 05 device";
	case CUSBaccess::CONTACT06_DEVICE:
		return "Contact 06 device";
	case CUSBaccess::CONTACT07_DEVICE:
		return "Contact 07 device";
	case CUSBaccess::CONTACT08_DEVICE:
		return "Contact 08 device";
	case CUSBaccess::CONTACT09_DEVICE:
		return "Contact 09 device";
	case CUSBaccess::CONTACT10_DEVICE:
		return "Contact 10 device";
	case CUSBaccess::CONTACT11_DEVICE:
		return "Contact 11 device";
	case CUSBaccess::CONTACT12_DEVICE:
		return "Contact 12 device";
	case CUSBaccess::CONTACT13_DEVICE:
		return "Contact 13 device";
	case CUSBaccess::CONTACT14_DEVICE:
		return "Contact 14 device";
	case CUSBaccess::CONTACT15_DEVICE:
		return "Contact 15 device";
	case CUSBaccess::ENCODER01_DEVICE:
		return "Encoder 01 device";
	case CUSBaccess::F4_DEVICE:
		return "F4 device";
	case CUSBaccess::KEYC01_DEVICE:
		return "Keyc01 device";
	case CUSBaccess::KEYC16_DEVICE:
		return "Keyc16 device";
	case CUSBaccess::ADC0800_DEVICE:
		return "AC0800 device";
	case CUSBaccess::ADC0801_DEVICE:
		return "AC0801 device";
	case CUSBaccess::ADC0802_DEVICE:
		return "AC0802 device";
	case CUSBaccess::ADC0803_DEVICE:
		return "AC0803 device";
	case CUSBaccess::COUNTER00_DEVICE:
		return "Counter device";
	case CUSBaccess::BUTTON_NODEVICE:
		return "Button no device";
	}

	return _("Device ID not recognised!");
}

void list_devices()
{
	init();

	printf(_("Cleware library version: %d\n"), CWusb.GetDLLVersion());
        printf(_("Number of Cleware devices found: %d\n"), USBcount);

        for (int devID=0; devID < USBcount; devID++)
	{
                int devType = CWusb.GetUSBType(devID);

		printf(_("Device: %d, type: %s (%d), version: %d, serial number: %d\n"),
			devID,
			device_id_to_string(devType), devType,
			CWusb.GetVersion(devID),
			CWusb.GetSerialNumber(devID)
			);
	}
}

#define NO_COMMAND	0
#define READ_TEMP	1
#define READ_SWITCH	2
#define RESET_DEVICE	3
#define SET_SWITCH	4
#define READ_HUMIDITY	5
#define START_DEVICE	6
#define SET_LED		7
#define READ_EXTERNAL_SWITCH	8
#define READ_AUTO_RESET	9
#define READ_MULTISWITCH	10
#define SET_MULTISWITCH	11
#define MULTISWITCH_DIRECTION	12
#define CONFIGURE_SWITCH	13
#define READ_COUNTER	14
#define SET_COUNTER	15
#define READ_ADC	16
#define SET_ADC_CHANNEL	17

int find_usb_id(int device_id)
{
	for(int devID=0; devID<USBcount; devID++)
	{
		if (device_id == -1 || CWusb.GetSerialNumber(devID) == device_id)
			return devID;
	}

	return -1;
}

int start_tapping_the_watchdog(int device_id)
{
	init();

	int usb_id = find_usb_id(device_id);

	if (usb_id == -1)
		error_exit(false, _("Device %d not found!"), device_id);

	int err_cnt = 0;

	if (daemon(0, 0) == -1)
		error_exit(true, _("Failed to become daemon process\n"));

	for(;;) {
		int time1 = 1, time2 = 5;

		if (!CWusb.CalmWatchdog(usb_id, time1, time2)) {
			err_cnt++;

			if (err_cnt == 10)
				error_exit(false, _("Failed to tap the watchdog 10 times"));
		}

		sleep(2);
	}

	return 0;
}

void emit(std::string what)
{
	if (fh == NULL)
		std::cout << what;
	else
		fprintf(fh, "%s", what.c_str());
}

void emit_ts()
{
	if (ts)
	{
		time_t t = time(NULL);
		struct tm *tm = localtime(&t);
		char buffer[256];

		strftime(buffer, sizeof buffer, ts, tm);

		emit(format("%s ", buffer));
	}

}

void init_output()
{
	if (ot == O_SS) {
		emit(format("<?xml version=\"1.0\"?>\n"));
		emit(format("<?mso-application progid=\"Excel.Sheet\"?>\n"));
		emit(format("<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\n"));
		emit(format(" xmlns:x=\"urn:schemas-microsoft-com:office:excel\"\n"));
		emit(format(" xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\"\n"));
		emit(format(" xmlns:html=\"http://www.w3.org/TR/REC-html40\">\n"));
		emit(format("<DocumentProperties xmlns=\"urn:schemas-microsoft-com:office:office\">\n"));
		emit(format("<Author>ClewareControl</Author>\n"));
		emit(format("<Company>folkert@vanheusden.com</Company>\n"));
		emit(format("<Version>" VERSION "</Version>\n"));
		emit(format("</DocumentProperties>\n"));
		emit(format("<Styles>\n"));
		emit(format("<Style ss:ID=\"sMD\">\n"));
		emit(format("<NumberFormat ss:Format=\"Long Time\"/>\n"));
		emit(format("</Style>\n"));
		emit(format("</Styles>\n"));
	}
}

void init_command_output(int command, int dev_serial)
{
	if (ot == O_SS)
	{
		const char *name = "Cleware";

		switch(command)
		{
			case READ_TEMP:
				name = "temperature";
				break;
			case READ_SWITCH:
				name = "read switch";
				break;
			case RESET_DEVICE:
				name = "reset";
				break;
			case SET_SWITCH:
				name = "set switch";
				break;
			case READ_HUMIDITY:
				name = "humitidy";
				break;
			case START_DEVICE:
				name = "start";
				break;
			case SET_LED:
				name = "set led";
				break;
			case READ_EXTERNAL_SWITCH:
				name = "read ext switch";
				break;
			case READ_AUTO_RESET:
				name = "read auto reset";
				break;
			case READ_MULTISWITCH:
				name = "read multisw";
				break;
			case SET_MULTISWITCH:
				name = "set multisw";
				break;
			case MULTISWITCH_DIRECTION:
				name = "multisw dir";
				break;
			case CONFIGURE_SWITCH:
				name = "cfg switch";
				break;
			case READ_COUNTER:
				name = "read counter";
				break;
			case SET_COUNTER:
				name = "set counter";
				break;
			case READ_ADC:
				name = "read adc";
				break;
			case SET_ADC_CHANNEL:
				name = "set adc";
				break;
			default:
				error_exit(false, "Internal error (unknown command)");
		}

		emit(format("<Worksheet ss:Name=\"%s\">\n", name));
		emit(format("<Table>\n"));
		emit(format("<Row>\n"));
		emit(format("<Cell><Data ss:Type=\"String\">Device serial number:</Data></Cell>\n"));
		emit(format("<Cell><Data ss:Type=\"String\">%d</Data></Cell>\n", dev_serial));
		emit(format("</Row>\n"));
		emit(format("<Row>\n"));
		emit(format("<Cell><Data ss:Type=\"String\">timestamp</Data></Cell>\n"));
		emit(format("<Cell><Data ss:Type=\"String\">value</Data></Cell>\n"));
		emit(format("</Row>\n"));
	}
}

void finish_command_output()
{
	if (ot == O_SS)
	{
                emit(format("</Table>\n"));
                emit(format("</Worksheet>\n"));
	}
}

void finish_output()
{
	if (ot == O_SS)
	{
	        emit(format("</Workbook>\n"));
	}
}

void output(std::string descr, double value)
{
	if (ot == O_FULL)
	{
		emit_ts();
		emit(format("%s: %f\n", descr.c_str(), value));
	}
	else if (ot == O_BRIEF)
	{
		emit_ts();
		emit(format("%f\n", value));
	}
	else if (ot == O_SS)
	{
                emit(format("<Row>\n"));

                std::string date_str = time_to_str(double(time(NULL)));
                emit(format("<Cell ss:StyleID=\"sMD\"><Data ss:Type=\"DateTime\">%s</Data></Cell>\n", date_str.c_str()));

		emit(format("<Cell><Data ss:Type=\"Number\">%f</Data></Cell>\n", value));

                emit(format("</Row>\n"));
	}
}

void output(std::string descr, int value)
{
	if (ot == O_FULL)
	{
		emit_ts();
		emit(format("%s: %d\n", descr.c_str(), value));
	}
	else if (ot == O_BRIEF)
	{
		emit_ts();
		emit(format("%d\n", value));
	}
	else if (ot == O_SS)
	{
                emit(format("<Row>\n"));

                std::string date_str = time_to_str(double(time(NULL)));
                emit(format("<Cell ss:StyleID=\"sMD\"><Data ss:Type=\"DateTime\">%s</Data></Cell>\n", date_str.c_str()));

		emit(format("<Cell><Data ss:Type=\"Number\">%d</Data></Cell>\n", value));

                emit(format("</Row>\n"));
	}
}

void output(std::string descr, std::string value)
{
	if (ot == O_FULL)
	{
		emit_ts();
		emit(format("%s: %s\n", descr.c_str(), value.c_str()));
	}
	else if (ot == O_BRIEF)
	{
		emit_ts();
		emit(format("%s\n", value.c_str()));
	}
	else if (ot == O_SS)
	{
                emit(format("<Row>\n"));

                std::string date_str = time_to_str(double(time(NULL)));
                emit(format("<Cell ss:StyleID=\"sMD\"><Data ss:Type=\"DateTime\">%s</Data></Cell>\n", date_str.c_str()));

		emit(format("<Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", value.c_str()));

                emit(format("</Row>\n"));
	}
}

void output(std::string value)
{
	if (ot == O_FULL)
	{
		emit_ts();
		emit(format("%s\n", value.c_str()));
	}
	else if (ot == O_BRIEF)
	{
	}
	else if (ot == O_SS)
	{
                emit(format("<Row>\n"));

                std::string date_str = time_to_str(double(time(NULL)));
                emit(format("<Cell ss:StyleID=\"sMD\"><Data ss:Type=\"DateTime\">%s</Data></Cell>\n", date_str.c_str()));

		emit(format("<Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", value.c_str()));

                emit(format("</Row>\n"));
	}
}

void spawn_script(const char *proc, const char *par)
{
	if (ot == O_FULL)
		fprintf(stderr, _("Starting childprocess: %s\n"), proc);

	pid_t pid = fork();

	if (pid == -1)
		fprintf(stderr, _("Failed to fork!\n"));
	else if (pid == 0)
	{
		if (-1 == execlp(proc, proc, par, (void *)NULL))
			fprintf(stderr, _("Failed to execlp(%s)\n"), proc);

		exit(1);
	}
}

void eval_val(double value)
{
	if (limit_exec_min)
	{
		if (value < limit_min)
		{
			if (!limit_min_triggered)
			{
				limit_min_triggered = true;

				spawn_script(limit_exec_min, format("%f", value).c_str());
			}
		}
		else
		{
			limit_min_triggered = false;
		}
	}

	if (limit_exec_max)
	{
		if (value > limit_max)
		{
			if (!limit_max_triggered)
			{
				limit_max_triggered = true;

				spawn_script(limit_exec_max, format("%f", value).c_str());
			}
		}
		else
		{
			limit_max_triggered = false;
		}
	}
}

int do_command(int device_id, int command, int par, int par2, double offset)
{
	init();

	int ok = 0;
	int usb_id = -1, retry;

	if (device_id == -1)
		fprintf(stderr, _("You did not select a device, using first device found.\n"));

	usb_id = find_usb_id(device_id);

	if (device_id == -1)
	{
		if (usb_id == -1)
		{
			fprintf(stderr, _("No device found\n"));

			if (getuid())
				fprintf(stderr, _("Not running as root: does your current user have enough rights to access the device?\n"));

			exit(1);
		}

		device_id = CWusb.GetSerialNumber(usb_id);

		if (ot == O_FULL) printf(_("Using device with serial number: %d\n"), device_id);
	}
	else if (usb_id == -1) {
		error_exit(false, _("Device %d not found"), device_id);
	}

	init_command_output(command, device_id);

	for(retry=0; (retry<retry_count || retry_count == 0) && abrt == false; retry++)
	{
		if (isatty(1) == 0 || fh != NULL)
			fprintf(stderr, "%d\r", retry);

		if (command == READ_TEMP)
		{
			double temperature;
			static int prev_time = -1;
			int time;

			if (CWusb.GetTemperature(usb_id, &temperature, &time) && time != prev_time)
			{
				output(_("Temperature"), temperature + offset);

				eval_val(temperature + offset);

				ok++;
			}
		}
		else if (command == READ_SWITCH)
		{
			int status;

			// if ((status = CWusb.GetSwitch(usb_id, (CUSBaccess::SWITCH_IDs)par)) != -1)
			if ((status = CWusb.GetSeqSwitch(usb_id, (CUSBaccess::SWITCH_IDs)par, 0)) != -1)
			{
				output(_("Status"), format("%s (%d)", status?"On":"Off", status));

				ok++;
			}
		}
		else if (command == RESET_DEVICE)
		{
			if (CWusb.ResetDevice(usb_id))
			{
				output(_("Device resetted"));

				ok++;
			}
		}
		else if (command == SET_SWITCH)
		{
			if (CWusb.SetSwitch(usb_id, (CUSBaccess::SWITCH_IDs)par, par2))
			{
				output(format(_("Switch %d"), par - 16), format(_("set to %s"), par2 ? _("On"):_("Off")));

				ok++;
			}
		}
		else if (command == READ_HUMIDITY)
		{
			double humidity;
			static int prev_time = -1;
			int time;

			if (CWusb.GetHumidity(usb_id, &humidity, &time) && prev_time != time)
			{
				output(_("Humidity"), humidity + offset);

				eval_val(humidity + offset);

				ok++;
			}
		}
		else if (command == START_DEVICE)
		{
			if (CWusb.StartDevice(usb_id))
			{
				output(_("Device started"));

				ok++;
			}
		}
		else if (command == SET_LED)
		{
			if (CWusb.SetLED(usb_id, (CUSBaccess::LED_IDs)par, par2))
			{
				output(format(_("LED %d"), par), format(_("set to %d"), par2));
				ok++;
			}
		}
		else if (command == READ_EXTERNAL_SWITCH)
		{
			output(_("Switched on count"), CWusb.GetManualOnCount(usb_id));
			output(_("Switched on duration "), CWusb.GetManualOnTime(usb_id));

			ok++;
		}
		else if (command == READ_AUTO_RESET)
		{
			output(_("Auto reset count"), CWusb.GetOnlineOnCount(usb_id));
			output(_("Auto reset duration"), CWusb.GetOnlineOnTime(usb_id));

			ok++;
		}
		else if (command == READ_MULTISWITCH)
		{
			unsigned long int value = 0;

			if (CWusb.GetMultiSwitch(usb_id, NULL, &value, 0))
			{
				std::string bits;
				for(int loop=15; loop>=0; loop--)
				{
					if (value & (1 << loop))
						bits += "1";
					else
						bits += "0";
				}

				output(_("Statusses of lines"), bits);

				ok++;
			}
		}
		else if (command == SET_MULTISWITCH)
		{
			if (CWusb.SetMultiSwitch(usb_id, par) == 0)
			{
				output(_("Bit-pattern set"));

				ok++;
			}
		}
		else if (command == MULTISWITCH_DIRECTION)
		{
			if (CWusb.SetMultiConfig(usb_id, par) == 0)
			{
				output(_("Directions set"));

				ok++;
			}
		}
		else if (command == CONFIGURE_SWITCH)
		{
			int type = CWusb.GetUSBType(usb_id);

			if (type == CUSBaccess::WATCHDOG_DEVICE  ||
					type == CUSBaccess::AUTORESET_DEVICE ||
					type == CUSBaccess::SWITCH1_DEVICE)
			{
				if (par == 0 ||
						par == 1 ||
						par == 2 ||
						par == 3)
				{
					unsigned char buf[3];

					buf[0] = CUSBaccess::EEwrite;
					buf[1] = 2;
					buf[2] = par;  // =watchdog, 1=autoreset, 2=switch, 3=switch ATXX
					CWusb.SetLED(usb_id, CUSBaccess::LED_3, 7);
					CWusb.SetValue(usb_id, buf, 3);
					CWusb.SetLED(usb_id, CUSBaccess::LED_3, 0);

					fprintf(stderr, _("Now first unplug (and replug) the device before use!\n"));

					ok++;
				}
				else
				{
					fprintf(stderr, _("%d is not a valid setting. Valid parameters: 0 (watchdog), 1 (autoreset), 2 (switch) and 3 (switch ATXX)\n"), par);
				}
			}
			else
			{
				fprintf(stderr, _("You can only perform this action a watchdog-, an autoreset- or a switch device!\n"));
			}
		}
		else if (command == READ_COUNTER)
		{
			int value = CWusb.GetCounter(usb_id, (CUSBaccess::COUNTER_IDs)par);

			output(_("Counter"), value);

			eval_val(value);

			ok++;
		}
		else if (command == SET_ADC_CHANNEL)
		{
			if (CWusb.ResetDevice(usb_id) == 0)
				fprintf(stderr, _("Failed resetting device, channel switch failed\n"));
			else
			{
				unsigned char buf[3] = { 0 };

				if (par == 0)
					buf[0] = 6;
				else if (par == 1)
				{
					buf[0] = 5;
					buf[1] = 1;
				}
				else
				{
					fprintf(stderr, _("Only channel 0 or 1 are supported\n"));
				}

				if (buf[0])
				{
					if (CWusb.SetValue(usb_id, buf, 3) == 0)
						fprintf(stderr, _("Problem setting ADC channel\n"));
					else
					{
						output(format(_("Channel %d selected"), par));
						ok++;
					}
				}
			}
		}
		else if (command == READ_ADC)
		{
			double scale = -1.0;
			if (par == 0)
				scale = 5.181;
			else if (par == 1)
				scale = 13.621;
			else if (par == 2)
				scale = 24.704;
			else
				fprintf(stderr, _("Second parameter must be either 0, 1 or 2. See help (-h).\n"));

			if (scale > 0)
			{
				unsigned char buf[4];
				if (CWusb.GetValue(usb_id, buf, sizeof buf) == 0)
					fprintf(stderr, _("Problem retrieving measurement\n"));
				else
				{
					unsigned char nibble1 = buf[2 + 0] & 0x0f;
					unsigned char nibble2 = buf[2 + 1] >> 4;
					unsigned char nibble3 = buf[2 + 1] & 0x0f;

					double value = (nibble1 << 8) + (nibble2) + (nibble3 << 4);
					value *= scale / double(0xfff);

					eval_val(value);

					output(_("Voltage"), value);

					ok++;
				}
			}
		}
		else if (command == SET_COUNTER)
		{
			if (CWusb.SetCounter(usb_id, par2) < 0)
				fprintf(stderr, _("Failed to set counter %d\n"), par);

			output(format(_("Counter %d set"), par));

			ok++;
		}
		else
		{
			error_exit(false, _("Internal error! (unknown command %d)"), command);
		}

		if ((retry < retry_count - 1 || retry_count == 0) && slp > 0.0)
			usleep(useconds_t(slp * 1000000.0));
	}

	if (ok == 0)
		fprintf(stderr, _("Failed to access device %d\n"), CWusb.GetSerialNumber(usb_id));
	else if (ok != retry_count)
		fprintf(stderr, _("Occasionally (%d/%d) failed to access device %d\n"), ok, retry_count, CWusb.GetSerialNumber(usb_id));

	finish_command_output();

	return ok ?  0 : -1;
}

bool ansi_terminal(void)
{
	const char *term = getenv("TERM");

	if (!isatty(1) || !isatty(2))
		return false;

	if (!term)
		return false;

	if (strcasestr(term, "ansi") == 0)
		return true;

	if (strcasestr(term, "console") == 0 || strcasestr(term, "con80x25") == 0 || strcasestr(term, "linux") == 0)
		return true;

	if (strcasestr(term, "screen") == 0)
		return true;

	if (strcasestr(term, "xterm") == 0)
		return true;

	if (strcasestr(term, "rxvt") == 0 || strcasestr(term, "konsole") == 0)
		return true;

	return false;
}

void set_bold(bool on)
{
	if (ansi_terminal()) {
		if (on)
			fprintf(stderr, "\x1b[1m");
		else
			fprintf(stderr, "\x1b[22m");
	}
}

void set_underline(bool on)
{
	if (ansi_terminal()) {
		if (on)
			fprintf(stderr, "\x1b[4m");
		else
			fprintf(stderr, "\x1b[24m");
	}
}

void reset_term()
{
	if (ansi_terminal())
		fprintf(stderr, "\x1b[0m\x1b[2K\r");
}

void help_header(const char *str)
{
	fprintf(stderr, "\n");

	set_bold(1);
	fprintf(stderr, " *** ");

	set_underline(1);
	fprintf(stderr, "%s", str);
	set_underline(0);

	fprintf(stderr, " ***\n");
	set_bold(0);
}

#define SWITCHES_COLUMN_WIDTH	10

int max_x = 80, max_y = 24;

void determine_terminal_size(void)
{
	struct winsize size;

	max_x = max_y = 0;

	if (!isatty(1))
	{
		max_y = 24;
		max_x = 80;
	}
#ifdef TIOCGWINSZ
	else if (ioctl(1, TIOCGWINSZ, &size) == 0)
	{
		max_y = size.ws_row;
		max_x = size.ws_col;
	}
#endif

	if (!max_x || !max_y)
	{
		char *dummy = getenv("COLUMNS");
		if (dummy)
			max_x = atoi(dummy);
		else
			max_x = 80;

		dummy = getenv("LINES");
		if (dummy)
			max_y = atoi(dummy);
		else
			max_y = 24;
	}
}

void str_add(char **to, const char *what, ...)
{
	int len_to = *to ? strlen(*to) : 0;
	char *buffer = NULL;
	int len_what = 0;

	va_list ap;

	va_start(ap, what);
	len_what = vasprintf(&buffer, what, ap);
	va_end(ap);

	*to = (char *)realloc(*to, len_to + len_what + 1);

	memcpy(&(*to)[len_to], buffer, len_what);

	(*to)[len_to + len_what] = 0x00;

	free(buffer);
}

void format_help(const char *short_str, const char *long_str, const char *descr)
{
	int par_width = SWITCHES_COLUMN_WIDTH, max_wrap_width = par_width / 2, cur_par_width = 0;
	int descr_width = max_x - (par_width + 1);
	char *line = NULL, *p = (char *)descr;
	char first = 1;

	if (long_str && short_str)
		str_add(&line, "%-4s / %s", short_str, long_str);
	else if (long_str)
		str_add(&line, "%s", long_str);
	else if (short_str)
		str_add(&line, "%s", short_str);
	else
		line = strdup("");

	cur_par_width = fprintf(stderr, "%-*s ", par_width, line);

	free(line);

	if (par_width + 1 >= max_x || cur_par_width >= max_x) {
		fprintf(stderr, "%s\n", descr);
		return;
	}

	for(;strlen(p);) {
		char *n =  NULL, *kn = NULL, *copy = NULL;
		int len_after_ww = 0, len_before_ww = 0;
		int str_len = 0, cur_descr_width = first ? max_x - cur_par_width : descr_width;

		while(*p == ' ')
			p++;

		str_len = strlen(p);
		if (!str_len)
			break;

		len_before_ww = std::min(str_len, cur_descr_width);

		n = &p[len_before_ww];
		kn = n;

		if (str_len > cur_descr_width) { 
			int n_len = 0;

			while (*n != ' ' && n_len < max_wrap_width) {
				n--;
				n_len++;
			}

			if (n_len >= max_wrap_width)
				n = kn;
		}

		len_after_ww = (int)(n - p);
		if (len_after_ww <= 0)
			break;

		copy = (char *)malloc(len_after_ww + 1);
		memcpy(copy, p, len_after_ww);
		copy[len_after_ww] = 0x00;

		if (first)
			first = 0;
		else
			fprintf(stderr, "%*s ", par_width, "");

		fprintf(stderr, "%s\n", copy);

		free(copy);

		p = n;
	}
}

void version(void)
{
	fprintf(stderr, "clewarecontrol " VERSION ", (C) 2005-2017 by folkert@vanheusden.com\n");
}

void usage(void)
{
	version();

	fprintf(stderr, "\n");

	help_header(_("meta"));
	format_help("-l", NULL, _("list devices"));
	format_help("-d x", NULL, _("use device with serial number 'x' for the next operations"));
	format_help("-c x", NULL, _("number of times to repeat the command: 0 for keep running"));
	format_help("-i x", NULL, _("delay between each command invocation"));
	format_help("-t", NULL, _("add a timestamp before each line (in seconds since 1970), also see -T"));
	format_help("-T x", NULL, _("add a timestamp before each line, x defines the format. see \"man strftime\" for supported parameters"));
	format_help("-F", NULL, _("fork into the background (become daemon)"));

	help_header(_("retrieve"));
	format_help("-rt", NULL, _("read temperature"));
	format_help("-rh", NULL, _("read humidity"));
	format_help("-rs x", NULL, _("read switch 'x'"));
	format_help("-rp", NULL, _("read external switch"));
	format_help("-rr", NULL, _("shows how often the auto-reset kicked in"));
	format_help("-rm", NULL, _("read states of the USB-IO16 lines"));
	format_help("-rc x", NULL, _("read counter (x= 0 or 1)"));
	format_help("-ra x", NULL, _("read ADC, x=0 for 5.181V, 1 for 13.621V and 2 for 24.704"));

	help_header(_("do"));
	format_help("-ar", NULL, _("reset device"));
	format_help("-as x y", NULL, _("set switch x to y (0=off, 1=on)"));
	format_help("-ag", NULL, _("start device"));
	format_help("-al x y", NULL, _("set led x to y (0...15)"));
	format_help("-am x", NULL, _("set the states of the USB-IO16 lines: x must be a hexvalue"));
	format_help("-ad x", NULL, _("set the directions of the USB-IO16 lines (hexvalue)"));
	format_help("-ac x y", NULL, _("set counter x to y (x= 0 or 1)"));
	format_help("-ai x", NULL, _("set ADC channel, x is either 0 or 1"));
	format_help("-cfg x", NULL, _("configure the device to be a watchdog (0), autoreset (1), switch (2) or switch ATXX (3)"));
	format_help("-w", NULL, _("become daemon-process that pats the watchdog"));

	help_header(_("output"));
	format_help("-o x", NULL, _("offset to add to values"));
	format_help("-O x", NULL, _("output type (brief (former -b), readable (default), spreadsheet (xml spreadsheet, compatible with e.g. openoffice and microsoft excel)"));
	format_help("-f x", NULL, _("send output to file (only measured data, errors are emitted to your console/terminal)"));
	format_help("-mintrig x y", NULL, _("if the value read (temperature, humidity, counter, ADC) becomes less than x, then spawn process y"));
	format_help("-maxtrig x y", NULL, _("if the value read (temperature, humidity, counter, ADC) becomes bigger than x, then spawn process y"));

	fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
	int loop, device_id = -1;
	int par = -1, par2 = -1;
	double offset = 0.0;

	determine_terminal_size();

	setlocale(LC_ALL, "");
	bindtextdomain("clewarecontrol", "/usr/share/locale");
	textdomain("clewarecontrol");

	if (argc < 2)
	{
		usage();
		return 1;
	}

	signal(SIGCHLD, SIG_IGN);
	signal(SIGINT, sigh);

	for(loop=1; loop<argc; loop++)
	{
		if (strcmp(argv[loop], "-l") == 0)
		{
			list_devices();
			return 0;
		}
		else if (strcmp(argv[loop], "-F") == 0)
		{
			if (daemon(-1, 0) == -1)
				error_exit(true, _("Failed to become daemon process"));
		}
		else if (strcmp(argv[loop], "-f") == 0)
		{
			fh = fopen(argv[++loop], "wb");
			if (!fh)
				error_exit(true, _("Failed to create file %s"), argv[loop]);
		}
		else if (strcmp(argv[loop], "-o") == 0)
		{
			offset = atof(argv[++loop]);
		}
		else if (strcmp(argv[loop], "-i") == 0)
		{
			slp = atof(argv[++loop]);
		}
		else if (strcmp(argv[loop], "-t") == 0)
		{
			ts = "%s";
		}
		else if (strcmp(argv[loop], "-T") == 0)
		{
			ts = argv[++loop];
		}
		else if (strcmp(argv[loop], "-d") == 0)
		{
			device_id = atoi(argv[++loop]);
		}
		else if (strcmp(argv[loop], "-O") == 0)
		{
			const char *fmt = argv[++loop];

			if (strcasecmp(fmt, "brief") == 0)
				ot = O_BRIEF;
			else if (strcasecmp(fmt, "readable") == 0 || strcasecmp(fmt, "full") == 0)
				ot = O_FULL;
			else if (strcasecmp(fmt, "spreadsheet") == 0 || strcasecmp(fmt, "xml") == 0)
				ot = O_SS;
			else
			{
				fprintf(stderr, _("Output format \"%s\" is not recognized\n"), fmt);
				usage();
				return 1;
			}

			init_output();
		}
		else if (strcmp(argv[loop], "-c") == 0)
		{
			retry_count = atoi(argv[++loop]);
		}
		else if (strcmp(argv[loop], "-rt") == 0)
		{
			if (do_command(device_id, READ_TEMP, -1, -1, offset))
				return 1;
		}
		else if (strcmp(argv[loop], "-rs") == 0)
		{
			par = atoi(argv[++loop]);
			if (par < 16)
				par += 16;
			if (do_command(device_id, READ_SWITCH, par, -1, -1000.0))
				return 1;
		}
		else if (strcmp(argv[loop], "-ar") == 0)
		{
			if (do_command(device_id, RESET_DEVICE, -1, -1, -1000.0))
				return 1;
		}
		else if (strcmp(argv[loop], "-ra") == 0)
		{
			par = atoi(argv[++loop]);
			if (do_command(device_id, READ_ADC, par, -1, -1000.0))
				return 1;
		}
		else if (strcmp(argv[loop], "-ai") == 0)
		{
			par = atoi(argv[++loop]);
			if (do_command(device_id, SET_ADC_CHANNEL, par, -1, -1000.0))
				return 1;
		}
		else if (strcmp(argv[loop], "-as") == 0)
		{
			par = atoi(argv[++loop]);
			if (par < 16)
				par += 16;
			par2 = atoi(argv[++loop]);
			if (do_command(device_id, SET_SWITCH, par, par2, -1000.0))
				return 1;
		}
		else if (strcmp(argv[loop], "-al") == 0)
		{
			par = atoi(argv[++loop]);
			par2 = atoi(argv[++loop]);

			if (do_command(device_id, SET_LED, par, par2, -1000.0))
				return 1;
		}
		else if (strcmp(argv[loop], "-ac") == 0)
		{
			par = atoi(argv[++loop]);
			par2 = atoi(argv[++loop]);

			if (do_command(device_id, SET_COUNTER, par, par2, -1000.0))
				return 1;
		}
		else if (strcmp(argv[loop], "-mintrig") == 0)
		{
			double trigger_val = atof(argv[++loop]);
			const char *cmd = argv[++loop];

			limit_exec_min = cmd;
			limit_min = trigger_val;
		}
		else if (strcmp(argv[loop], "-maxtrig") == 0)
		{
			double trigger_val = atof(argv[++loop]);
			const char *cmd = argv[++loop];

			limit_exec_max = cmd;
			limit_max = trigger_val;
		}
		else if (strcmp(argv[loop], "-rh") == 0)
		{
			if (do_command(device_id, READ_HUMIDITY, -1, -1, offset))
				return 1;
		}
		else if (strcmp(argv[loop], "-ag") == 0)
		{
			if (do_command(device_id, START_DEVICE, -1, -1, -1000.0))
				return 1;
		}
		else if (strcmp(argv[loop], "-w") == 0)
		{
			if (start_tapping_the_watchdog(device_id))
				return 1;
		}
		else if (strcmp(argv[loop], "-rp") == 0)
		{
			if (do_command(device_id, READ_EXTERNAL_SWITCH, -1, -1, -1000.0))
				return 1;
		}
		else if (strcmp(argv[loop], "-rr") == 0)
		{
			if (do_command(device_id, READ_AUTO_RESET, -1, -1, -1000.0))
				return 1;
		}
		else if (strcmp(argv[loop], "-rm") == 0)
		{
			if (do_command(device_id, READ_MULTISWITCH, -1, -1, -1000.0))
				return 1;
		}
		else if (strcmp(argv[loop], "-rc") == 0)
		{
			par = strtol(argv[++loop], NULL, 10);
			if (do_command(device_id, READ_COUNTER, par, -1, -1000.0))
				return 1;
		}
		else if (strcmp(argv[loop], "-am") == 0)
		{
			par = strtol(argv[++loop], NULL, 16);
			if (do_command(device_id, SET_MULTISWITCH, par, -1, -1000.0))
				return 1;
		}
		else if (strcmp(argv[loop], "-ad") == 0)
		{
			par = strtol(argv[++loop], NULL, 16);
			if (do_command(device_id, MULTISWITCH_DIRECTION, par, -1, -1000.0))
				return 1;
		}
		else if (strcmp(argv[loop], "-cfg") == 0)
		{
			par = atoi(argv[++loop]);
			if (do_command(device_id, CONFIGURE_SWITCH, par, -1, -1000.0))
				return 1;
		}
		else if (strcmp(argv[loop], "-h") == 0)
		{
			usage();
			return 0;
		}
		else if (strcmp(argv[loop], "-V") == 0)
		{
			version();
			return 0;
		}
		else
		{
			fprintf(stderr, _("Switch '%s' is not recognized.\n"), argv[loop]);
			return 1;
		}
	}

	finish_output();

	if (fh)
	{
		fsync(fileno(fh));

		fclose(fh);
	}

	CWusb.CloseCleware();

	return 0;
}
