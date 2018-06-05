#ifndef EX5_LOG_H
#define EX5_LOG_H

//==============================================================================
//=============================== INCLUDES  ====================================
//==============================================================================
#define FAILURE -1
#define SUCCESS 0

// value to initialize timer funtion
#define INIT 0
// two digit number. (will be filled with 0 if its single digit number)
#define TWO_DIG 2
// If number is not 2-digit number, the gap filled with 0
#define FILL  '0'

//==============================================================================
//=============================== INCLUDES  ====================================
//==============================================================================
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <iomanip>

//==============================================================================
//================================== CLASS =====================================
//==============================================================================
class Log
{
public:
    Log(std::string path,  pthread_mutex_t& log_mutex)
            :log_path(path),
			 log_mutex(log_mutex){}

    /**
     * Destructor.
     */
    ~Log();

    /*
     * Open the file stream.
     */
    int open_log_file();

    /*
     * Open the file stream.
     */
    void close_log_file();

    /**
    * This method writes a given string to the log.
    */
    int write_to_log(std::string text);

    std::string log_path;
    std::fstream logFile;
    pthread_mutex_t& log_mutex;
    // Variables with which time stamp is measured.
    time_t now;
    tm *ltm;

};

#endif //EX5_LOG_H