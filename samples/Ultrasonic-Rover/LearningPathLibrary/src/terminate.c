#include "terminate.h"


static volatile sig_atomic_t terminationRequired = false;
static volatile sig_atomic_t _exitCode = 0;


void lp_registerTerminationHandler(void) {
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = lp_terminationHandler;
	sigaction(SIGTERM, &action, NULL);
}

void lp_terminationHandler(int signalNumber) {
	// Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
	terminationRequired = true;
	_exitCode = ExitCode_TermHandler_SigTerm;
}

void lp_terminate(int exitCode) {
	_exitCode = exitCode;
	terminationRequired = true;
}

bool lp_isTerminationRequired(void) {
	return terminationRequired;
}

int lp_getTerminationExitCode(void) {
	return _exitCode;
}

void lp_eventLoopRun(void)
{
    EventLoop *el = lp_timerGetEventLoop();
    // run the main event loop
    while (!terminationRequired) {
        int result = EventLoop_Run(el, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == -1 && errno != EINTR) {
            lp_terminate(ExitCode_Main_EventLoopFail);
        }
    }
}
