/*
 * Author: Mark Gottscho <mgottscho@ucla.edu>
 */

/**
 * @file
 *
 * @brief Extensions to third-party optionparser-related code.
 */

#ifndef MY_ARG_H
#define MY_ARG_H

//Headers
#include <ExampleArg.h>

//Libraries
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <stdio.h>

namespace xmem {

    class MyArg : public ExampleArg
    {
    public:
        /**
         * @brief Checks an option that it is an integer.
         */
        static ArgStatus Integer(const Option& option, bool msg) {
            char* endptr = 0;
            if (option.arg != 0 && strtol(option.arg, &endptr, 10)) {};
            if (endptr != option.arg && *endptr == 0)
                return ARG_OK;

            if (msg)
                printError("Option '", option, "' requires an integer argument\n");
            return ARG_ILLEGAL;
        }

        /**
         * @brief Checks an option that it is a nonnegative integer.
         */
        static ArgStatus NonnegativeInteger(const Option& option, bool msg) {
            char* endptr = 0;
            int32_t tmp = -1;
            if (option.arg != 0)
                tmp = strtol(option.arg, &endptr, 10);
            if (endptr != option.arg && *endptr == 0 && tmp >= 0)
                return ARG_OK;

            if (msg)
                printError("Option '", option, "' requires a non-negative integer argument\n");
            return ARG_ILLEGAL;
        }

        /**
         * @brief Checks an option that it is a positive integer.
         */
        static ArgStatus PositiveInteger(const Option& option, bool msg) {
            char* endptr = 0;
            int32_t tmp = -1;
            if (option.arg != 0)
                tmp = strtol(option.arg, &endptr, 10);
            if (endptr != option.arg && *endptr == 0 && tmp > 0)
                return ARG_OK;

            if (msg)
                printError("Option '", option, "' requires a positive integer argument\n");
            return ARG_ILLEGAL;
        }

        /**
         * @brief Checks an option that it is a hexadecimal address.
         */
        static ArgStatus HexAddress(const Option& option, bool msg) {
            char* endptr = 0;
            unsigned long long tmp = -1;
            if (option.arg != 0)
                tmp = strtoull(option.arg, &endptr, 16);
            if (endptr != option.arg && *endptr == 0 && tmp > 0 && tmp != ULLONG_MAX)
                return ARG_OK;

            if (msg)
                printError("Option '", option, "' requires a hexadecimal address argument\n");
            return ARG_ILLEGAL;
        }

        /**
         * @brief Checks an option that it is a list of hexadecimal address.
         */
        static ArgStatus HexAddresses(const Option& option, bool msg) {
            char* endptr = 0;
            unsigned long long tmp = -1;
            if (option.arg != 0) {
                std::string t;
                std::stringstream ss(option.arg);

                while(getline(ss, t, ',')) {
                    endptr = 0;
                    tmp = strtoull(t.c_str(), &endptr, 16);
                    if (tmp <= 0 || tmp == ULLONG_MAX || *endptr != 0 || endptr == option.arg) {
                        if (msg)
                            printError("Option '", option, "' requires a hexadecimal address argument\n");
                        return ARG_ILLEGAL;
                    }
                }
            }

            return ARG_OK;
        }
    };
};

#endif
