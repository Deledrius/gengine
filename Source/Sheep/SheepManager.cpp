//
// SheepManager.cpp
//
// Clark Kromenaker
//
#include "SheepManager.h"

#include "Services.h"
#include "StringUtil.h"

SheepScript* SheepManager::Compile(const char* filename)
{
    return mCompiler.Compile(filename);
}

SheepScript* SheepManager::Compile(const std::string& sheep)
{
    return mCompiler.Compile(sheep);
}

SheepScript* SheepManager::Compile(std::istream& stream)
{
    return mCompiler.Compile(stream);
}

SheepScript* SheepManager::CompileEval(const std::string& sheep)
{
	std::string fullSheep = StringUtil::Format(mEvalHusk, sheep.c_str());
	return mCompiler.Compile(fullSheep);
}

void SheepManager::Execute(const std::string& sheepName, const std::string& functionName, std::function<void()> finishCallback)
{
	SheepScript* script = Services::GetAssets()->LoadSheep(sheepName);
	if(script != nullptr)
	{
		Execute(script, functionName, finishCallback);
	}
}

void SheepManager::Execute(SheepScript* script, std::function<void()> finishCallback)
{
	mVirtualMachine.Execute(script, finishCallback);
}

void SheepManager::Execute(SheepScript* script, const std::string& functionName, std::function<void()> finishCallback)
{
	mVirtualMachine.Execute(script, functionName, finishCallback);
}

bool SheepManager::Evaluate(SheepScript* script)
{
	return Evaluate(script, 0, 0);
}

bool SheepManager::Evaluate(SheepScript* script, int n, int v)
{
	// No script automatically equates to "true".
	if(script == nullptr) { return true; }
	
	// Otherwise, do it "for real."
	return mVirtualMachine.Evaluate(script, n, v);
}
