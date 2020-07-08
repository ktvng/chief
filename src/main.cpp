#include <iostream>

#include "main.h"
#include "program.h"
#include "token.h"
#include "diagnostics.h"
#include "operation.h"
#include "reference.h"
#include "parse.h"



// Logging
LogSeverityType LogAtLevel = LogSeverityType::Sev0_Debug;
bool g_outputOn = true;

int main()
{
    bool ShouldPrintInitialCompileResult = true; 
    bool ShouldPrintProgramExecutionFinalResult = true;

    PurgeLog();                         // cleans log between each run
    CompileGrammar();                   // compile grammar from grammar.txt


    // compile program
    LogIt(LogSeverityType::Sev1_Notify, "main", "program compile begins");

    ParseProgram("./program.pebl");
    if(FatalCompileError)
        return 1;

    LogIt(LogSeverityType::Sev1_Notify, "main", "program compile finished");

    if(ShouldPrintInitialCompileResult)
    {
        LogDiagnostics(PROGRAM->Main, "initial program parse structure", "main");
        for(ObjectReferenceMap* map: PROGRAM->ObjectsIndex)
        {
            LogDiagnostics(map, "initial object reference state", "main");
        }
    }

    // run program
    SetConsoleColor(ConsoleColor::LightBlue);
    std::cout << "################################################################################\n";
    SetConsoleColor(ConsoleColor::White);
    LogIt(LogSeverityType::Sev1_Notify, "main", "program execution begins");

    DoProgram(*PROGRAM);
    
    LogIt(LogSeverityType::Sev1_Notify, "main", "program execution finished");
    SetConsoleColor(ConsoleColor::LightBlue);
    std::cout << "################################################################################\n";
    SetConsoleColor(ConsoleColor::White);

    if(ShouldPrintProgramExecutionFinalResult)
    {
        for(ObjectReferenceMap* map: PROGRAM->ObjectsIndex)
        {
            LogDiagnostics(map, "final object reference state", "main");
        }
    }

    LogItDebug("end reached.", "main");
    return 0;
}
