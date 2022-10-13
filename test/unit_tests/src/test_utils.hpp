/**-----------------------------------------------------------------------------
; @file test_utils.cpp
;
; @brief
;   The file contains auxiliary functionality (e.g., interaction with simulator)
;   for creating and executing tests of ExternalProcess class.
;
; @author ep1h
;-----------------------------------------------------------------------------*/
#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#define QQ(x) #x
#define Q(x) QQ(x)

#ifndef EXTERNAL_PROCESS_SIMULATOR_NAME
#define EXTERNAL_PROCESS_SIMULATOR_NAME external_process_simulator.exe
#endif /* EXTERNAL_PROCESS_SIMULATOR_NAME */

/* Remote process name. Testing functionality of ExternalProcess class will be
   performed based on interaction with this process. */
extern const char test_application[];

#endif /* TEST_UTILS_HPP */
