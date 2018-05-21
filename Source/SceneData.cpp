//
// SceneData.cpp
//
// Clark Kromenaker
//
#include "SceneData.h"
#include "IniParser.h"
#include <iostream>
#include "StringUtil.h"

SceneData::SceneData(std::string name, char* data, int dataLength) : Asset(name)
{
    ParseFromData(data, dataLength);
}

std::string SceneData::GetBSPName()
{
    // If an override is specified, use it!
    if(!mBSPNameOverride.empty())
    {
        return mBSPNameOverride;
    }
    
    // Otherwise, we default to our asset name, with no extension.
    return mName.substr(0, mName.length() - 4);
}

void SceneData::ParseFromData(char *data, int dataLength)
{
    IniParser parser(data, dataLength);
    parser.ParseAll();
    
    std::vector<IniSection> topSections = parser.GetSections("");
    for(auto& section : topSections)
    {
        for(auto& entry : section.entries)
        {
            if(StringUtil::EqualsIgnoreCase(entry->key, "bsp"))
            {
                mBSPNameOverride = entry->value;
            }
        }
    }
}