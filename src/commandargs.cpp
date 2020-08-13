#include <string>
#include <iostream>

#include "commandargs.h"

static ProgramConfiguration* Config = nullptr;
static Setting* CurrentState = nullptr;

// Returns true if a flag is matched
bool MatchesFlag(String token, Setting** matchedSetting)
{
	// If there is no text (shouldn't happen) OR this is not a flag, discard
    if(token.length() < 1 || token.at(0) != '-')
    {
        return false;
    }

    for(Setting& s: *Config)
    {
        if(token == s.Flag)
        {
            CurrentState = &s;
            *matchedSetting = &s;
            return true;
        }
    }
    
    // If an unknown which resembles a flag is passed, inform the user
    if(token.at(0) == '-')
    {
        Usage(std::vector<SettingOption>());
    }
    
    return false;
}

void ParseCommandArgs(int argc, char* argv[], ProgramConfiguration* config)
{
    Config = config;
    CurrentState = nullptr;
    bool inFlag = false;

    std::vector<SettingOption> CurrentSettingOptions;
    CurrentSettingOptions.reserve(8);

    for(int i=1; i<argc; i++)
    {
        String arg = argv[i];
        
        Setting* newSetting = nullptr;
        if(MatchesFlag(arg, &newSetting))
        {
            if(newSetting->Action(CurrentSettingOptions))
            {
                inFlag = true;
            }
        }
        else if(inFlag)
        {
            // Flags consume at most one argument
            SettingOption option = arg;
            CurrentSettingOptions.push_back(option);
            CurrentState->Action(CurrentSettingOptions);
            CurrentSettingOptions.clear();
            inFlag = false;
        }
        else
        {        	
            // Set program name to run, this is the 0'th Setting
            Config->at(0).Flag = arg;
            break;
        }
    }

    // flush
    if(CurrentState != nullptr)
    {
        CurrentState->Action(CurrentSettingOptions);
    }
}
