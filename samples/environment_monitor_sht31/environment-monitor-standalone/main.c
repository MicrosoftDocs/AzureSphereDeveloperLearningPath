﻿/*
 *   AzureSphereDevX
 *   ===============
 *   These labs are built on version 1 of the Azure Sphere Learning Path library.
 *   Version 2 of the Learning Path library is called AzureSphereDevX.
 *
 *   The AzureSphereDevX documentation and examples are located at https://github.com/Azure-Sphere-DevX/AzureSphereDevX.Examples.
 *   AzureSphereDevX builds on the Azure Sphere Learning Path library incorporating more customer experiences.
 *   Everything you learn completing these labs is relevant to AzureSphereDevX.
 * 	 
 * 
 *   LAB UPDATES
 *   ===========
 *   This lab now use the LP_TIMER_HANDLER macro to define timer handlers
 *   This lab now use the LP_DEVICE_TWIN_HANDLER macro to define device twin handlers
 *   All application declarations are located in main.h
 *    
 *
 *   DISCLAIMER
 *   ==========
 *   The functions provided in the LearningPathLibrary folder:
 *
 *	   1. are prefixed with lp_, typedefs are prefixed with LP_
 *	   2. are built from the Azure Sphere SDK Samples at https://github.com/Azure/azure-sphere-samples
 *	   3. are not intended as a substitute for understanding the Azure Sphere SDK Samples.
 *	   4. aim to follow best practices as demonstrated by the Azure Sphere SDK Samples.
 *	   5. are provided as is and as a convenience to aid the Azure Sphere Developer Learning experience.
 *
 *
 *   DEVELOPER BOARD SELECTION
 *   =========================
 *   The following developer boards are supported.
 *
 *	   1. AVNET Azure Sphere Starter Kit.
 *     2. AVNET Azure Sphere Starter Kit Revision 2.
 *	   3. Seeed Studio Azure Sphere MT3620 Development Kit aka Reference Design Board or rdb.
 *	   4. Seeed Studio Seeed Studio MT3620 Mini Dev Board.
 *
 *   ENABLE YOUR DEVELOPER BOARD
 *   ===========================
 *   Each Azure Sphere developer board manufacturer maps pins differently. You need to select the configuration that matches your board.
 *
 *   Follow these steps:
 *
 *	   1. Open CMakeLists.txt.
 *	   2. Uncomment the set command that matches your developer board.
 *	   3. Click File, then Save to save the CMakeLists.txt file which will auto generate the CMake Cache.
 */

#include "main.h"

/// <summary>
/// Read sensor and send to Azure IoT
/// </summary>
static LP_TIMER_HANDLER(MeasureSensorHandler)
{
	static int msgId = 0;
	int32_t int32_temperature, int32_humidity;

	/* Measure temperature and relative humidity and store into variables
	 * temperature, humidity (each output multiplied by 1000).
	*/
	int16_t ret = sht3x_measure_blocking_read(&int32_temperature, &int32_humidity);

	temperature = (float)int32_temperature / 1000.0f;
	humidity = (float)int32_humidity / 1000.0f;

	if (ret == STATUS_OK && snprintf(msgBuffer, JSON_MESSAGE_BYTES, MsgTemplate, temperature, humidity, ++msgId) > 0)
	{
		Log_Debug("%s\n", msgBuffer);
	}
}
LP_TIMER_HANDLER_END

static bool InitializeSht31(void)
{
	uint16_t interval_in_seconds = 2;
	int retry = 0;

	sensirion_i2c_init();

	while (sht3x_probe() != STATUS_OK && ++retry < 5)
	{
		Log_Debug("SHT sensor probing failed\n");
		sensirion_sleep_usec(1000000u);
	}

	if (retry < 5)
	{
		Log_Debug("SHT sensor probing successful\n");
	}
	else
	{
		Log_Debug("SHT sensor probing failed\n");
	}

	sensirion_sleep_usec(interval_in_seconds * 1000000u); // sleep for good luck

	return true;
}

/// <summary>
///  Initialize PeripheralGpios, device twins, direct methods, timers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static void InitPeripheralGpiosAndHandlers(void)
{
	InitializeSht31();
	lp_timerSetStart(timerSet, NELEMS(timerSet));
}

/// <summary>
///     Close PeripheralGpios and handlers.
/// </summary>
static void ClosePeripheralGpiosAndHandlers(void)
{
	Log_Debug("Closing file descriptors\n");
	lp_timerSetStop(timerSet, NELEMS(timerSet));
	lp_timerEventLoopStop();
}

int main(int argc, char *argv[])
{
	lp_registerTerminationHandler();

	InitPeripheralGpiosAndHandlers();

	// Main loop
	while (!lp_isTerminationRequired())
	{
		int result = EventLoop_Run(lp_timerGetEventLoop(), -1, true);
		// Continue if interrupted by signal, e.g. due to breakpoint being set.
		if (result == -1 && errno != EINTR)
		{
			lp_terminate(ExitCode_Main_EventLoopFail);
		}
	}

	ClosePeripheralGpiosAndHandlers();

	Log_Debug("Application exiting.\n");
	return lp_getTerminationExitCode();
}