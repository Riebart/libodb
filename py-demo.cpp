#include <stdlib.h>
#include <stdio.h>
#include <boost/python.hpp>

#define BUF_SIZE 50000

// #include "Python.h"

using namespace boost::python;

char * handle_indent(char * string)
{
    char * tmp;
    
    printf("... ");
    
    //TODO: don't overflow me buffers
    fgets(string, BUF_SIZE-1, stdin);
    
    while (string[0] == '\t')
    {
                
        if ( (tmp=strchr(string, ':')) != NULL )
        {
            string=handle_indent(tmp+1);
        }     

        printf("... ");
        string=strchr(string, '\n');
        string++;
        
        fgets(tmp, BUF_SIZE-1-(tmp-string), stdin);
   
    }
                    
    
}


int main(int argc, char ** argv)
{
    Py_Initialize();
    Py_SetProgramName(argv[0]);
    Py_InitializeEx(0);
    
    char buffer [BUF_SIZE];
    char * tmp;

    object main_module = import("__main__");
    object main_namespace = main_module.attr("__dict__");

            
    while (true)
    {
        try
        {
               
            printf(">>> ");
            
            fgets(buffer, BUF_SIZE-1, stdin);
            
            if (buffer[0] == '\0')
            {
                printf("\n");
                return EXIT_SUCCESS;
            }
            
            if ( (tmp=strchr(buffer, ':')) != NULL )
            {
                handle_indent(tmp+1);                
            }
            
            printf(buffer);
            printf("\n");            
            object ignored = exec(buffer, main_namespace, main_namespace);

            memset(buffer, 0, BUF_SIZE);
            

        }   
        catch (error_already_set const &)
        {
            printf(buffer);
            printf("\n");
            PyErr_Print();
            
        }
    }

    return EXIT_SUCCESS;
}
