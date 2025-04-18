#include "SceneAsset.h"

#include <iostream>

#include "AssetManager.h"
#include "IniParser.h"
#include "Skybox.h"
#include "StringUtil.h"

SceneAsset::~SceneAsset()
{
	delete mSkybox;
}

void SceneAsset::Load(uint8_t* data, uint32_t dataLength)
{
    ParseFromData(data, dataLength);
}

void SceneAsset::ParseFromData(uint8_t* data, uint32_t dataLength)
{
    IniParser parser(data, dataLength);
    parser.SetMultipleKeyValuePairsPerLine(false);

    IniSection section;
    while(parser.ReadNextSection(section))
    {
        if(section.name.empty())
        {
            for(auto& line : section.lines)
            {
                IniKeyValue& entry = line.entries.front();
                if(StringUtil::EqualsIgnoreCase(entry.key, "bsp"))
                {
                    mBspName = entry.value;
                }
            }
        }
        else if(StringUtil::EqualsIgnoreCase(section.name, "Skybox"))
        {
            if(section.lines.size() > 0)
            {
                mSkybox = new Skybox();
            }
            for(auto& line : section.lines)
            {
                IniKeyValue& entry = line.entries.front();
                if(StringUtil::EqualsIgnoreCase(entry.key, "left"))
                {
                    Texture* texture = gAssetManager.LoadSceneTexture(entry.value, GetScope());
                    mSkybox->SetLeftTexture(texture);
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "right"))
                {
                    Texture* texture = gAssetManager.LoadSceneTexture(entry.value, GetScope());
                    mSkybox->SetRightTexture(texture);
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "front"))
                {
                    Texture* texture = gAssetManager.LoadSceneTexture(entry.value, GetScope());
                    mSkybox->SetFrontTexture(texture);
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "back"))
                {
                    Texture* texture = gAssetManager.LoadSceneTexture(entry.value, GetScope());
                    mSkybox->SetBackTexture(texture);
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "up"))
                {
                    Texture* texture = gAssetManager.LoadSceneTexture(entry.value, GetScope());
                    mSkybox->SetUpTexture(texture);
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "down"))
                {
                    Texture* texture = gAssetManager.LoadSceneTexture(entry.value, GetScope());
                    mSkybox->SetDownTexture(texture);
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "azimuth"))
                {
                    mSkybox->SetAzimuth(Math::ToRadians(entry.GetValueAsFloat()));
                }
            }

            // Make sure any skybox mask textures are also loaded.
            mSkybox->LoadMaskTextures();
        }
        else if(StringUtil::EqualsIgnoreCase(section.name, "Lights") || StringUtil::EqualsIgnoreCase(section.name, "Models"))
        {
            // We are not doing anything with these sections right now - except ignoring them.
        }
        else
        {
            /*
            // Any other section MUST BE (as far as I know) a light definition.
            mLights.emplace_back();
            mLights.back().name = section.name;

            for(auto& line : section.lines)
            {
                IniKeyValue& entry = line.entries.front();
                if(StringUtil::EqualsIgnoreCase(entry.key, "Type"))
                {
                    mLights.back().type = entry.GetValueAsInt();
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "Position"))
                {
                    mLights.back().position = entry.GetValueAsVector3();
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "Direction"))
                {
                    mLights.back().direction = entry.GetValueAsVector3();
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "Color"))
                {
                    // Colors in SCN are floating point.
                    // Since we don't yet have support for that, just read in as floats and convert to Color32.
                    Vector3 color = entry.GetValueAsVector3();
                    mLights.back().color = Color32(static_cast<int>(color.x * 255),
                                                   static_cast<int>(color.y * 255),
                                                   static_cast<int>(color.z * 255));
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "Hotspot"))
                {
                    mLights.back().hotspot = entry.GetValueAsFloat();
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "Falloff"))
                {
                    mLights.back().falloff = entry.GetValueAsFloat();
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "AttenStart"))
                {
                    mLights.back().attenuationStart = entry.GetValueAsFloat();
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "AttenEnd"))
                {
                    mLights.back().attenuationEnd = entry.GetValueAsFloat();
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "UseAtten"))
                {
                    mLights.back().useAttenuation = entry.GetValueAsInt() != 0;
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "CastShadows"))
                {
                    mLights.back().castShadows = entry.GetValueAsInt() != 0;
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "Overshoot"))
                {
                    mLights.back().overshoot = entry.GetValueAsInt() != 0;
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "Intensity"))
                {
                    mLights.back().intensity = entry.GetValueAsFloat();
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "Radius"))
                {
                    mLights.back().radius = entry.GetValueAsFloat();
                }
                else if(StringUtil::EqualsIgnoreCase(entry.key, "DecayType"))
                {
                    mLights.back().decayType = entry.GetValueAsInt();
                }
            }
            */
        }
    }

    // If no BSP name was specified in the SCN file, default to the asset name with no extension.
    if(mBspName.empty())
    {
        mBspName = GetNameNoExtension();
    }
}
