//==============================================================================
//=============================== INCLUDES  ====================================
//==============================================================================

#include "Log.h"


//==============================================================================
//================================== CLASS =====================================
//==============================================================================

/**
 * Destructor.
 */
Log::~Log()
{
    close_log_file();
	pthread_mutex_destroy(&log_mutex);
}

/*
 * Open the file stream.
 */
int Log::open_log_file()
{
    logFile.open(log_path, std::fstream::out | std::fstream::in | \
                 std::fstream::trunc);

    if(!logFile.is_open())
    {
        return FAILURE;
    }

    return SUCCESS;
}

/*
 * Open the file stream.
 */
void Log::close_log_file()
{
    logFile.close();
}

/**
 * This method writes a given string to the log.
 */
int Log::write_to_log(std::string text) {
    if (!logFile) {
        return FAILURE;
    }

    pthread_mutex_lock(&log_mutex);

    now = time(INIT);
    ltm = localtime(&now);

    logFile << std::setw(TWO_DIG) << std::setfill(FILL) << ltm->tm_hour << ':'
            << std::setw(TWO_DIG) << std::setfill(FILL) << ltm->tm_min  << ':'
            << std::setw(TWO_DIG) << std::setfill(FILL) << ltm->tm_sec  << " \t"
            << text;

    pthread_mutex_unlock(&log_mutex);


    if (logFile.bad()) {
        return FAILURE;
    }

    return SUCCESS;
}