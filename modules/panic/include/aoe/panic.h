//
// Created by yarten on 24-1-4.
//

#pragma once

#include <string_view>
#include <source_location>
#include <iostream>


namespace aoe
{
    struct
    {
        /**
         * \brief Print an error message before aborting the program.
         */
        [[noreturn]]
        void operator()(const std::string_view msg, const std::source_location loc = std::source_location::current()) const
            noexcept(true)
        {
            printAndAbort("panic", msg, loc);
        }

        /**
         * \brief Print an error message about some implementation missing before aborting the program.
         */
        [[noreturn]]
        void todo(const std::string_view msg, const std::source_location loc = std::source_location::current()) const
            noexcept(true)
        {
            printAndAbort("panic.todo", msg, loc);
        }

        /**
         * \brief Print an error message about some impossible things happening before aborting the program.
         */
        [[noreturn]]
        void wtf(const std::string_view msg, const std::source_location loc = std::source_location::current()) const
            noexcept(true)
        {
            printAndAbort("panic.wtf", msg, loc);
        }

    private:
        [[noreturn]]
        static void printAndAbort(const std::string_view tag, const std::string_view msg, const std::source_location loc)
            noexcept(true)
        {
            std::cerr
                << "[" << tag << "]! " << msg << std::endl
                << "  in: " << loc.function_name() << std::endl
                << "  at: line " << loc.line() << ", column " << loc.column() << std::endl
                << "  of: " << loc.file_name() << std::endl;

            std::abort();
        }
    } inline constexpr panic; // use this to break the control flow
}
