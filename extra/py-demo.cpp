/* MPL2.0 HEADER START
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MPL2.0 HEADER END
 *
 * Copyright 2010-2012 Michael Himbeault and Travis Friesen
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <boost/python.hpp>

using namespace libodb;

#define BUF_SIZE 50000

// #include "Python.h"

using namespace boost::python;

char * handle_indent(char * string)
{
    char * tmp, * tmp2;

    printf("... ");

#warning "TODO: don't overflow me buffers"
    tmp2 = fgets(string, BUF_SIZE-1, stdin);

    while (string[0] == '\t')
    {

        if ( (tmp=strchr(string, ':')) != NULL )
        {
            string=handle_indent(tmp+1);
        }

        printf("... ");
        string=strchr(string, '\n');
        string++;

        tmp2 = fgets(tmp, BUF_SIZE-1-(tmp-string), stdin);

    }

    return string;


}


int main(int argc, char ** argv)
{
    Py_Initialize();
    Py_SetProgramName(argv[0]);
    Py_InitializeEx(0);

    char buffer [BUF_SIZE];
    char * tmp, * tmp2;

    object main_module = import("__main__");
    object main_namespace = main_module.attr("__dict__");


    while (true)
    {
        try
        {

            printf(">>> ");

            tmp2 = fgets(buffer, BUF_SIZE-1, stdin);

            if (buffer[0] == '\0')
            {
                printf("\n");
                return EXIT_SUCCESS;
            }

            if ( (tmp=strchr(buffer, ':')) != NULL )
            {
                handle_indent(tmp+1);
            }

            printf("%500s\n", buffer);
            object ignored = exec(buffer, main_namespace, main_namespace);

            memset(buffer, 0, BUF_SIZE);


        }
        catch (error_already_set const &)
        {
            printf("%500s\n", buffer);
            PyErr_Print();

        }
    }

    return EXIT_SUCCESS;
}
