 // Hardware definition
#include "hw/azure_sphere_learning_path.h"

// Learning Path Libraries
#include "azure_iot.h"
#include "config.h"
#include "exit_codes.h"
#include "peripheral_gpio.h"
#include "terminate.h"
#include "timer.h"

// System Libraries
#include "applibs_versions.h"
#include <applibs/gpio.h>
#include <applibs/log.h>
#include <applibs/powermanagement.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

// Hardware specific
#ifdef OEM_AVNET
#include "board.h"
#include "imu_temp_pressure.h"
#include "light_sensor.h"
#endif // OEM_AVNET

// Hardware specific
#ifdef OEM_SEEED_STUDIO
#include "board.h"
#endif // SEEED_STUDIO

#define LP_LOGGING_ENABLED FALSE
#define JSON_MESSAGE_BYTES 256 // Number of bytes to allocate for the JSON telemetry message for IoT Central
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:azuresphere:labmonitor;1"	// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play

// Forward signatures
static LP_DECLARE_DIRECT_METHOD_HANDLER(RestartDeviceHandler);
static LP_DECLARE_TIMER_HANDLER(AzureIoTConnectionStatusHandler);
static LP_DECLARE_TIMER_HANDLER(DelayRestartDeviceTimerHandler);
static LP_DECLARE_TIMER_HANDLER(MeasureSensorHandler);

LP_USER_CONFIG lp_config;

static char msgBuffer[JSON_MESSAGE_BYTES] = { 0 };

// GPIO
static LP_GPIO azureIotConnectedLed = {
    .pin = NETWORK_CONNECTED_LED,
    .direction = LP_OUTPUT,
    .initialState = GPIO_Value_Low,
    .invertPin = true,
    .name = "networkConnectedLed" };

// Timers
static LP_TIMER azureIotConnectionStatusTimer = {
    .period = {5, 0},
    .name = "azureIotConnectionStatusTimer",
    .handler = AzureIoTConnectionStatusHandler };

static LP_TIMER restartDeviceOneShotTimer = {
    .period = {0, 0},
    .name = "restartDeviceOneShotTimer",
    .handler = DelayRestartDeviceTimerHandler };

static LP_TIMER measureSensorTimer = {
	.period = { 6, 0 },
	.name = "measureSensorTimer",
	.handler = MeasureSensorHandler };

// Azure IoT Device Twins
static LP_DEVICE_TWIN_BINDING dt_reportedRestartUtc = {
    .twinProperty = "ReportedRestartUTC",
    .twinType = LP_TYPE_STRING };

// Azure IoT Direct Methods
static LP_DIRECT_METHOD_BINDING dm_restartDevice = {
    .methodName = "RestartDevice",
    .handler = RestartDeviceHandler };

// Initialize Sets
LP_GPIO* gpioSet[] = { &azureIotConnectedLed };
LP_TIMER* timerSet[] = { &measureSensorTimer, &azureIotConnectionStatusTimer, &restartDeviceOneShotTimer };
LP_DEVICE_TWIN_BINDING* deviceTwinBindingSet[] = { &dt_reportedRestartUtc };
LP_DIRECT_METHOD_BINDING* directMethodBindingSet[] = { &dm_restartDevice };

// Message templates and property sets

static const char* msgTemplate = "{ \"Temperature\":%3.2f, \"Humidity\":%3.1f, \"Pressure\":%3.1f, \"MsgId\":%d }";

static LP_MESSAGE_PROPERTY* telemetryMessageProperties[] = {
	&(LP_MESSAGE_PROPERTY) { .key = "appid", .value = "hvac" },
	&(LP_MESSAGE_PROPERTY) {.key = "format", .value = "json" },
	&(LP_MESSAGE_PROPERTY) {.key = "type", .value = "telemetry" },
	&(LP_MESSAGE_PROPERTY) {.key = "version", .value = "1" }
};