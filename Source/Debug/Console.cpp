//
// Console.cpp
//
// Clark Kromenaker
//
#include "Console.h"

#include "Services.h"
#include "StringUtil.h"

void Console::AddToScrollback(std::string str)
{
	std::vector<std::string> lines = StringUtil::Split(str, '\n');
	for(auto& line : lines)
	{
		mScrollback.push_back(line);
	}
	
	if(mScrollback.size() > kMaxScrollbackLength)
	{
		int extraCount = kMaxScrollbackLength - (int)mScrollback.size();
		mScrollback.erase(mScrollback.begin(), mScrollback.begin() + extraCount);
	}
}

void Console::ExecuteCommand(std::string command)
{
	// Passing an empty command outputs 40 dashes.
	// An acknowledgement that you did something meaningless.
	if(command.empty())
	{
		AddToScrollback("----------------------------------------");
		return;
	}
	Services::GetReports()->Log("Console", StringUtil::Format("console command: '%s'", command.c_str()));
	
	// Modify command to have required syntax.
	//TODO: Update compiler to accept without braces?
	std::string modCommand = command;
	if(modCommand[0] != '{')
	{
		modCommand.insert(modCommand.begin(), '{');
	}
	if(modCommand[modCommand.size() - 1] != '}')
	{
		modCommand.push_back('}');
	}
	
	// Compile the sheep from text.
	std::string scriptName = StringUtil::Format("`Console`:%i", mCommandCounter);
	SheepScript* sheepScript = Services::GetSheep()->Compile(scriptName, modCommand);
	
	// If compiled successfully, execute it!
	if(sheepScript != nullptr)
	{
		Services::GetSheep()->Execute(sheepScript, [sheepScript]() -> void {
			delete sheepScript;
		});
	}
	
	// Add to history.
	mCommandHistory.push_back(command);
	
	// Increment command counter.
	mCommandCounter++;
}

std::string Console::GetCommandFromHistory(int index) const
{
	if(index < 0 || index >= static_cast<int>(mCommandHistory.size()))
	{
		return "";
	}
	return mCommandHistory[index];
}
