//
// IniParser.cpp
//
// Clark Kromenaker
//
#include "IniParser.h"
#include <iostream>
#include <cstdlib>
#include "imstream.h"
#include "StringUtil.h"

Vector2 IniKeyValue::GetValueAsVector2()
{
    // We assume the string form of {4.23, 5.23} as an example.
    // First, let's get rid of the braces.
    std::string noBraces = value.substr(1, value.length() - 2);
    
    // Find the comma index. If not present,
    // this isn't the right form, so we fail.
    std::size_t commaIndex = noBraces.find(',');
    if(commaIndex == std::string::npos)
    {
        return Vector2::Zero;
    }
    
    // Split at the comma.
    std::string firstNum = noBraces.substr(0, commaIndex);
    std::string secondNum = noBraces.substr(commaIndex + 1, std::string::npos);
    
    // Convert to numbers and return.
    return Vector2(atof(firstNum.c_str()), atof(secondNum.c_str()));
}

Vector3 IniKeyValue::GetValueAsVector3()
{
    // We assume the string form of {4.23, 5.23, 10.04} as an example.
    // First, let's get rid of the braces.
    std::string noBraces = value.substr(1, value.length() - 2);
    
    // Find the two commas.
    std::size_t firstCommaIndex = noBraces.find(',');
    if(firstCommaIndex == std::string::npos)
    {
        return Vector3::Zero;
    }
    std::size_t secondCommaIndex = noBraces.find(',', firstCommaIndex + 1);
    if(secondCommaIndex == std::string::npos)
    {
        return Vector3::Zero;
    }
    
    // Split at commas.
    std::string firstNum = noBraces.substr(0, firstCommaIndex);
    std::string secondNum = noBraces.substr(firstCommaIndex + 1, secondCommaIndex - firstCommaIndex);
    std::string thirdNum = noBraces.substr(secondCommaIndex + 1, std::string::npos);
    
    // Convert to numbers and return.
    return Vector3(atof(firstNum.c_str()), atof(secondNum.c_str()), atof(thirdNum.c_str()));
}

IniParser::IniParser(const char* filePath)
{
    // Create stream to read from file.
    mStream = new std::ifstream(filePath, std::ios::in);
    if(!mStream->good())
    {
        std::cout << "IniParser can't read from file " << filePath << "!" << std::endl;
    }
    mCurrentSection = "";
}

IniParser::IniParser(const char* memory, unsigned int memoryLength)
{
    // Create stream to read from memory.
    mStream = new imstream(memory, memoryLength);
    mCurrentSection = "";
}

IniParser::~IniParser()
{
    delete mStream;
}

void IniParser::ParseAll()
{
    // Make sure everything is in a state to read in data.
    mStream->seekg(0);
    mSections.clear();
    
    // We will use this one section to populate the entire section list.
    // We just push a copy of this onto the list each time.
    IniSection section;
    
    // Just read the whole file one line at a time...
    std::string line;
    while(std::getline(*mStream, line))
    {
        // "getline" reads up to the '\n' character in a file, and "eats" the '\n' too.
        // But Windows line breaks might include the '\r' character too, like "\r\n".
        // To deal with this semi-elegantly, we'll search for and remove the '\r' here.
        if(!line.empty() && line[line.length() - 1] == '\r')
        {
            line.resize(line.length() - 1);
        }
        
        // Ignore empty lines. Need to do this after \r check because some lines might just be '\r'.
        if(line.empty()) { continue; }
        
        // Ignore comment lines.
        if(line.length() > 2 && line[0] == '/' && line[1] == '/') { continue; }
        
        // Detect headers and react to them, but don't stop parsing.
        if(line.length() > 2 && line[0] == '[' && line[line.length() - 1] == ']')
        {
            if(section.entries.size() > 0)
            {
                mSections.push_back(section);
            }
            section.condition.clear();
            section.entries.clear();
            
            // Subtract the brackets to get the section name.
            section.name = line.substr(1, line.length() - 2);
            
            // If there's an equals sign, it means this section is conditional.
            std::size_t equalsIndex = section.name.find('=');
            if(equalsIndex != std::string::npos)
            {
                section.condition = section.name.substr(equalsIndex + 1, std::string::npos);
                section.name = section.name.substr(0, equalsIndex);
            }
            continue;
        }
        
        // From here: just a normal line with key/value pair(s) on it.
        // So, we need to split it into individual key/value pairs.
        IniKeyValue* lastOnLine = nullptr;
        while(!line.empty())
        {
            // First, determine the token we want to work with on the current line.
            // We want the first item, if there are multiple comma-separated values.
            // Otherwise, we just want the whole remaining line.
            std::string currentKeyValuePair;
            
            // We can't just use string::find because we want to ignore commas that are inside braces.
            // Ex: pos={10, 20, 30} should NOT be considered multiple key/value pairs.
            std::size_t found = std::string::npos;
            int braceDepth = 0;
            for(int i = 0; i < line.length(); i++)
            {
                if(line[i] == '{') { braceDepth++; }
                if(line[i] == '}') { braceDepth--; }
                
                if(line[i] == ',' && braceDepth == 0)
                {
                    found = i;
                    break;
                }
            }
            
            // If we found a valid comma separator, then we only want to deal with the parts in front of the comma.
            // If no comma, then the rest of the line is our focus.
            if(found != std::string::npos)
            {
                currentKeyValuePair = line.substr(0, found);
                line = line.substr(found + 1, std::string::npos);
            }
            else
            {
                currentKeyValuePair = line;
                line.clear();
            }
            
            IniKeyValue* keyValue = new IniKeyValue();
            if(lastOnLine == nullptr)
            {
                section.entries.push_back(keyValue);
            }
            else
            {
                lastOnLine->next = keyValue;
            }
            lastOnLine = keyValue;
            
            // Trim any whitespace.
            StringUtil::Trim(currentKeyValuePair);
            
            // OK, so now we have a string representing a key/value pair, "model=blahblah" or similar.
            // But it might also just be a keyword (no value) like "hidden".
            found = currentKeyValuePair.find('=');
            if(found != std::string::npos)
            {
                keyValue->key = currentKeyValuePair.substr(0, found);
                keyValue->value = currentKeyValuePair.substr(found + 1, std::string::npos);
            }
            else
            {
                keyValue->key = currentKeyValuePair;
            }
        }
    }
    
    // After we've run out of all lines to read, push the final section on the list, if we have one.
    if(section.entries.size() > 0)
    {
        mSections.push_back(section);
    }
}

std::vector<IniSection> IniParser::GetSections(std::string name)
{
    std::vector<IniSection> toReturn;
    for(auto& section : mSections)
    {
        if(section.name == name)
        {
            toReturn.push_back(section);
        }
    }
    return toReturn;
}

bool IniParser::ReadLine()
{
    if(mStream->eof()) { return false; }
    
    std::string line;
    while(std::getline(*mStream, line))
    {
        // "getline" reads up to the '\n' character in a file, and "eats" the '\n' too.
        // But Windows line breaks might include the '\r' character too, like "\r\n".
        // To deal with this semi-elegantly, we'll search for and remove the '\r' here.
        if(!line.empty() && line[line.length() - 1] == '\r')
        {
            line.resize(line.length() - 1);
        }
        
        // Ignore empty lines. Need to do this after \r check because some lines might just be '\r'.
        if(line.empty()) { continue; }
        
        // Ignore comment lines.
        if(line.length() > 2 && line[0] == '/' && line[1] == '/') { continue; }
        
        // Detect headers and react to them, but don't stop parsing.
        if(line.length() > 2 && line[0] == '[' && line[line.length() - 1] == ']')
        {
            mCurrentSection = line.substr(1, line.length() - 2);
            continue;
        }
        
        // Save the current line.
        mCurrentLine = line;
        mCurrentLineWorking = line;
        return true;
    }
    
    // If we get here, I guess it means we ran out of stuff to read.
    return false;
}

bool IniParser::SkipToNextSection()
{
    // If at EOF, we will definitely fail to read to next section.
    if(mStream->eof()) { return false; }
    
    // Our logic here is, see what the current section is, and then read lines
    // until we either reach the end of the file, or we get to a new section.
    std::string currentSection = mCurrentSection;
    while(ReadLine() && mCurrentSection == currentSection) { }
    
    // If at EOF, again, we failed to read to the next section.
    if(mStream->eof()) { return false; }
    
    // Otherwise, we will usually have succeeded...unless there was only one section or something?
    return currentSection != mCurrentSection;
}

bool IniParser::ReadKeyValuePair()
{
    // If nothing left in current line, we're done.
    if(mCurrentLineWorking.empty()) { return false; }
    
    // First, determine the token we want to work with on the current line.
    // We want the first item, if there are multiple comma-separated values.
    // Otherwise, we just want the whole remaining line.
    std::string currentKeyValuePair;
    
    // We can't just use string::find because we want to ignore commans that are inside braces.
    // Ex: pos={10, 20, 30} should not be considered multiple key/value pairs.
    std::size_t found = std::string::npos;
    int braceDepth = 0;
    for(int i = 0; i < mCurrentLineWorking.length(); i++)
    {
        if(mCurrentLineWorking[i] == '{') { braceDepth++; }
        if(mCurrentLineWorking[i] == '}') { braceDepth--; }
        
        if(mCurrentLineWorking[i] == ',' && braceDepth == 0)
        {
            found = i;
            break;
        }
    }
    
    // If we found a valid comma separator, then we only want to deal with the parts in front of the comma.
    // If no comma, then the rest of the line is our focus.
    if(found != std::string::npos)
    {
        currentKeyValuePair = mCurrentLineWorking.substr(0, found);
        mCurrentLineWorking = mCurrentLineWorking.substr(found + 1, std::string::npos);
    }
    else
    {
        currentKeyValuePair = mCurrentLineWorking;
        mCurrentLineWorking.clear();
    }
    
    // Trim any whitespace.
    StringUtil::Trim(currentKeyValuePair);
    
    // OK, so now we have a string representing a key/value pair, "model=blahblah" or similar.
    // But it might also just be a keyword (no value) like "hidden".
    found = currentKeyValuePair.find('=');
    if(found != std::string::npos)
    {
        mCurrentKeyValue.key = currentKeyValuePair.substr(0, found);
        mCurrentKeyValue.value = currentKeyValuePair.substr(found + 1, std::string::npos);
    }
    else
    {
        mCurrentKeyValue.key = currentKeyValuePair;
    }
    return true;
}