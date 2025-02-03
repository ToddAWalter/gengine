#include "SidneyFiles.h"

#include <algorithm>

#include "AssetManager.h"
#include "GameProgress.h"
#include "Sidney.h"
#include "SidneyUtil.h"
#include "Texture.h"
#include "UIButton.h"
#include "UICanvas.h"
#include "UIImage.h"
#include "UILabel.h"

void SidneyFiles::Init(Sidney* parent)
{
    // Build directory structure.
    mData.emplace_back();
    mData.back().name = "ImageDir";
    mData.back().type = SidneyFileType::Image;
    mData.emplace_back();
    mData.back().name = "AudioDir";
    mData.back().type = SidneyFileType::Audio;
    mData.emplace_back();
    mData.back().name = "TextDir";
    mData.back().type = SidneyFileType::Text;
    mData.emplace_back();
    mData.back().name = "FingerDir";
    mData.back().type = SidneyFileType::Fingerprint;
    mData.emplace_back();
    mData.back().name = "LicenseDir";
    mData.back().type = SidneyFileType::License;
    mData.emplace_back();
    mData.back().name = "ShapeDir";
    mData.back().type = SidneyFileType::Shape;

    // Build list of known files. (TODO: Would be cool if this was data-driven?)
    // Fingerprints (0-18)
    mAllFiles.emplace_back( 0, SidneyFileType::Fingerprint, "fileAbbePrint",        "ABBE_FINGERPRINT",      "e_sidney_add_fingerprint_abbe");
    mAllFiles.emplace_back( 1, SidneyFileType::Fingerprint, "fileBuchelliPrint",    "BUCHELLIS_FINGERPRINT", "e_sidney_add_fingerprint_buchelli");
    mAllFiles.emplace_back( 2, SidneyFileType::Fingerprint, "fileButhanePrint",     "BUTHANES_FINGERPRINT",  "e_sidney_add_fingerprint_buthane");
    mAllFiles.emplace_back( 3, SidneyFileType::Fingerprint, "fileEstellePrint",     "ESTELLES_FINGERPRINT",  "e_sidney_add_fingerprint_estelle");
    mAllFiles.emplace_back( 4, SidneyFileType::Fingerprint, "fileLadyHowardPrint",  "HOWARDS_FINGERPRINT",   "e_sidney_add_fingerprint_howard");
    mAllFiles.emplace_back( 5, SidneyFileType::Fingerprint, "fileMontreauxPrint",   "MONTREAUX_FINGERPRINT", "e_sidney_add_fingerprint_montreaux");
    mAllFiles.emplace_back( 6, SidneyFileType::Fingerprint, "fileWilkesPrint",      "WILKES_FINGERPRINT",    "e_sidney_add_fingerprint_wilkes");
    mAllFiles.emplace_back( 7, SidneyFileType::Fingerprint, "fileMoselyPrint",      "MOSELYS_FINGERPRINT");  //TODO: Hmm, no score event for this one?
    mAllFiles.emplace_back( 8, SidneyFileType::Fingerprint, "fileLarryPrint",       "LARRYS_FINGERPRINT",    "e_sidney_add_fingerprint_larry");
    mAllFiles.emplace_back( 9, SidneyFileType::Fingerprint, "fileWilkesPrint2",     "WILKES_FINGERPRINT_LABELED_BUCHELLI");
    mAllFiles.emplace_back(10, SidneyFileType::Fingerprint, "fileBuchelliPrint2",   "BUCHELLIS_FINGERPRINT_LABELED_WILKES");
    mAllFiles.emplace_back(11, SidneyFileType::Fingerprint, "fileUnknownPrint1",    "UNKNOWN_PRINT_1",       "e_sidney_add_manuscript_prints_buthane");
    mAllFiles.emplace_back(12, SidneyFileType::Fingerprint, "fileUnknownPrint2",    "UNKNOWN_PRINT_2",       "e_sidney_add_manuscript_prints_mosley");
    mAllFiles.emplace_back(13, SidneyFileType::Fingerprint, "fileUnknownPrint3",    "UNKNOWN_PRINT_3",       "e_sidney_add_manuscript_prints_buchelli");
    mAllFiles.emplace_back(14, SidneyFileType::Fingerprint, "fileUnknownPrint4",    "UNKNOWN_PRINT_4");
    mAllFiles.emplace_back(15, SidneyFileType::Fingerprint, "fileUnknownPrint5",    "UNKNOWN_PRINT_5");
    mAllFiles.emplace_back(16, SidneyFileType::Fingerprint, "fileUnknownPrint6",    "UNKNOWN_PRINT_6");
    mAllFiles.emplace_back(17, SidneyFileType::Fingerprint, "fileLSR1Print",        "UNKNOWN_PRINT_7");      // Unused?
    mAllFiles.emplace_back(18, SidneyFileType::Fingerprint, "fileEstellesLSRPrint", "UNKNOWN_PRINT_8");

    // Images (19-26)
    mAllFiles.emplace_back(19, SidneyFileType::Image, "fileMap",              "MAP",                      "e_sidney_add_map");
    mAllFiles.emplace_back(20, SidneyFileType::Image, "fileParchment1",       "PARCHMENT_1",              "e_sidney_add_parch1");
    mAllFiles.emplace_back(21, SidneyFileType::Image, "fileParchment2",       "PARCHMENT_2",              "e_sidney_add_parch2");
    mAllFiles.emplace_back(22, SidneyFileType::Image, "filePoussinPostcard",  "POUSSIN_POSTCARD",         "e_sidney_add_postcard_1");
    mAllFiles.emplace_back(23, SidneyFileType::Image, "fileTeniersPostcard1", "TENIERS_POSTCARD_TEMP",    "e_sidney_add_postcard_2");
    mAllFiles.emplace_back(24, SidneyFileType::Image, "fileTeniersPostcard2", "TENIERS_POSTCARD_NO_TEMP", "e_sidney_add_postcard_3");
    mAllFiles.emplace_back(25, SidneyFileType::Image, "fileHermeticSymbols",  "HERM_SYMBOLS",             "e_sidney_add_hermetical_symbols_from_serres");
    mAllFiles.emplace_back(26, SidneyFileType::Image, "fileSUMNote",          "I_AM_WORDS",               "e_sidney_add_sum_note");

    // Audio (27-28)
    mAllFiles.emplace_back(27, SidneyFileType::Audio, "fileAbbeTape",     "", "e_sidney_add_tape_abbe");
    mAllFiles.emplace_back(28, SidneyFileType::Audio, "fileBuchelliTape", "", "e_sidney_add_tape_buchelli");

    // Text (29-30)
    mAllFiles.emplace_back(29, SidneyFileType::Text, "fileArcadiaText", "");
    mAllFiles.emplace_back(30, SidneyFileType::Text, "fileTempleOfSolomonText", "");     //TODO: Is this a text type?

    mAllFiles.emplace_back(31, SidneyFileType::Image, "fileHermeticSymbols", "");         //TODO: Seems doubled up and unused?

    // Licenses (32-36)
    mAllFiles.emplace_back(32, SidneyFileType::License, "fileBuchelliLicense",   "BUCHELLIS_LICENSE", "e_sidney_add_license_buchelli");
    mAllFiles.emplace_back(33, SidneyFileType::License, "fileEmilioLicense",     "EMILIOS_LICENSE",   "e_sidney_add_license_emilio");
    mAllFiles.emplace_back(34, SidneyFileType::License, "fileLadyHowardLicense", "HOWARDS_LICENSE",   "e_sidney_add_license_howard");
    mAllFiles.emplace_back(35, SidneyFileType::License, "fileMoselyLicense",     "MOSELYS_LICENSE",   "e_sidney_add_license_mosely");
    mAllFiles.emplace_back(36, SidneyFileType::License, "fileWilkesLicense",     "WILKES_LICENSE",    "e_sidney_add_license_wilkes");

    // Shapes (37-39)
    mAllFiles.emplace_back(37, SidneyFileType::Shape, "triangle");
    mAllFiles.emplace_back(38, SidneyFileType::Shape, "circle");
    mAllFiles.emplace_back(39, SidneyFileType::Shape, "rectangle");
    mAllFiles.emplace_back(40, SidneyFileType::Shape, "hexagram");

    // The Files UI is a bit unique. It's more of a "floating dialog" that can appear over other screens.
    // Though files UI doesn't have a background image, it's helpful to create a "dummy" rect to help with positioning things.
    {
        mRoot = new Actor("Files", TransformType::RectTransform);
        mRoot->GetTransform()->SetParent(parent->GetTransform());

        RectTransform* rt = mRoot->GetComponent<RectTransform>();
        Texture* backgroundTexture = gAssetManager.LoadTexture("S_BKGND.BMP");
        rt->SetSizeDelta(static_cast<float>(backgroundTexture->GetWidth()),
                         static_cast<float>(backgroundTexture->GetHeight()));
    }

    mFileList.Init(mRoot, false);
    mShapeList.Init(mRoot, true);
    mCustomList.Init(mRoot, false);
}

void SidneyFiles::Show(std::function<void(SidneyFile*)> selectFileCallback)
{
    mFileList.Show(mAllFiles, mData, selectFileCallback);
}

void SidneyFiles::ShowShapes(std::function<void(SidneyFile*)> selectFileCallback)
{
    mShapeList.Show(mAllFiles, mData, selectFileCallback);
}

void SidneyFiles::ShowCustom(const std::string& title, const std::vector<std::string>& choices, std::function<void(size_t)> selectCallback)
{
    mCustomList.Show(title, choices, selectCallback);
}

void SidneyFiles::AddFile(size_t fileIndex)
{
    if(fileIndex >= mAllFiles.size()) { return; }

    // Find appropriate directory.
    for(auto& dir : mData)
    {
        // Add file if not already in this directory.
        if(dir.type == mAllFiles[fileIndex].type && !dir.HasFile(fileIndex))
        {
            dir.fileIds.emplace_back(fileIndex);

            // Sort files by ID to ensure a consistent ordering when displayed in the UI.
            std::sort(dir.fileIds.begin(), dir.fileIds.end(), [](int a, int b){
                return a < b;
            });

            // Add to score if there's a score event.
            if(!mAllFiles[fileIndex].scoreName.empty())
            {
                gGameProgress.ChangeScore(mAllFiles[fileIndex].scoreName);
            }
        }
    }
}

bool SidneyFiles::HasFile(size_t fileId) const
{
    for(auto& dir : mData)
    {
        if(dir.HasFile(fileId))
        {
            return true;
        }
    }
    return false;
}

SidneyFile* SidneyFiles::GetFile(size_t fileId)
{
    for(SidneyFile& file : mAllFiles)
    {
        if(file.id == fileId)
        {
            return &file;
        }
    }
    return nullptr;
}

bool SidneyFiles::HasFile(const std::string& fileName) const
{
    for(auto& file : mAllFiles)
    {
        if(StringUtil::EqualsIgnoreCase(file.name, fileName))
        {
            return HasFile(file.id);
        }
    }
    return false;
}

bool SidneyFiles::HasFileOfType(SidneyFileType type) const
{
    for(auto& dir : mData)
    {
        if(dir.type == type)
        {
            return !dir.fileIds.empty();
        }
    }
    return false;
}

void SidneyFiles::OnPersist(PersistState& ps)
{
    //TODO: This is a bit of a HACK to deal with the fact that save/loading the all files vector overwrites any new entries on load.
    //TODO: Maybe we should only be saving the "has been analyzed" bool...
    if(ps.IsSaving())
    {
        ps.Xfer(PERSIST_VAR(mAllFiles));
    }
    else if(ps.IsLoading())
    {
        std::vector<SidneyFile> loadedFiles;
        ps.Xfer("mAllFiles", loadedFiles);

        for(SidneyFile& loadedFile : loadedFiles)
        {
            for(SidneyFile& file : mAllFiles)
            {
                if(file.id == loadedFile.id)
                {
                    file.hasBeenAnalyzed = loadedFile.hasBeenAnalyzed;
                    break;
                }
            }
        }
    }

    ps.Xfer(PERSIST_VAR(mData));
}

void SidneyFiles::FileListWindow::Init(Actor* parent, bool forShapes)
{
    mForShapes = forShapes;

    // Add dialog background.
    {
        // Create a root actor for the dialog.
        mWindowRoot = new Actor(TransformType::RectTransform);
        mWindowRoot->GetTransform()->SetParent(parent->GetTransform());
        mWindowRoot->AddComponent<UICanvas>(1);

        // Create a black background.
        UIImage* backgroundImage = mWindowRoot->AddComponent<UIImage>();
        backgroundImage->SetColor(Color32::Black);

        // Receive input to avoid sending inputs to main screen below this screen.
        backgroundImage->SetReceivesInput(true);

        // Set to correct size and position.
        RectTransform* rt = backgroundImage->GetRectTransform();
        rt->SetSizeDelta(153.0f, 350.0f);
        rt->SetAnchor(AnchorPreset::TopLeft);
        rt->SetAnchoredPosition(40.0f, -66.0f);
    }

    // Add close button.
    SidneyUtil::CreateCloseWindowButton(mWindowRoot, [this](){
        mWindowRoot->SetActive(false);
    });

    // Add title/header.
    {
        Actor* titleActor = new Actor(TransformType::RectTransform);
        titleActor->GetTransform()->SetParent(mWindowRoot->GetTransform());

        UILabel* titleLabel = titleActor->AddComponent<UILabel>();
        titleLabel->GetRectTransform()->SetPivot(0.5f, 1.0f); // Top-Center
        titleLabel->GetRectTransform()->SetAnchorMin(0.0f, 1.0f);
        titleLabel->GetRectTransform()->SetAnchorMax(1.0f, 1.0f); // Fill space horizontally, anchor to top.
        titleLabel->GetRectTransform()->SetSizeDeltaY(20.0f);

        titleLabel->SetFont(gAssetManager.LoadFont("SID_TEXT_18.FON"));
        titleLabel->SetText(forShapes ? SidneyUtil::GetAnalyzeLocalizer().GetText("ShapeList") :
                                        SidneyUtil::GetAnalyzeLocalizer().GetText("FileList"));
        titleLabel->SetHorizonalAlignment(HorizontalAlignment::Center);
        titleLabel->SetVerticalAlignment(VerticalAlignment::Top);
        mTitleLabel = titleLabel;
    }

    // Hide by default.
    mWindowRoot->SetActive(false);
}

void SidneyFiles::FileListWindow::Show(std::vector<SidneyFile>& files, const std::vector<SidneyDirectory>& data, std::function<void(SidneyFile*)> selectCallback)
{
    // Show the window.
    mWindowRoot->SetActive(true);

    // Disable all existing labels.
    for(FileListButton& button : mButtons)
    {
        button.button->SetEnabled(false);
        button.label->SetEnabled(false);
    }
    mButtonIndex = 0;

    // Iterate and place each label.
    Vector2 topLeft(10.0f, -28.0f);
    for(auto& dir : data)
    {
        // Only show shapes if this is the shapes list.
        // Don't show shapes in the normal file list!
        if(mForShapes && dir.type != SidneyFileType::Shape)
        {
            continue;
        }
        if(!mForShapes && dir.type == SidneyFileType::Shape)
        {
            continue;
        }

        // Create the label for the directory itself.
        // This has a leading "-" and uses gray colored text.
        // Not used for shapes though!
        if(!mForShapes)
        {
            FileListButton& dirButton = GetFileListButton();
            dirButton.label->GetRectTransform()->SetAnchoredPosition(topLeft);
            dirButton.label->SetFont(gAssetManager.LoadFont("SID_TEXT_14_UL.FON"));
            dirButton.label->SetText("-" + SidneyUtil::GetMainScreenLocalizer().GetText(dir.name));
            dirButton.button->SetPressCallback(nullptr);
            topLeft.y -= 16.0f;
        }

        // Create labels for each file within the directory.
        // These are indented slightly and use gold colored text.
        for(int fileId : dir.fileIds)
        {
            // Get the file associated with this ID.
            SidneyFile* file = nullptr;
            for(SidneyFile& f : files)
            {
                if(f.id == fileId)
                {
                    file = &f;
                    break;
                }
            }

            // Invalid file, ignore!
            if(file == nullptr) { continue; }

            // Create button for this file.
            FileListButton& fileButton = GetFileListButton();
            fileButton.label->GetRectTransform()->SetAnchoredPosition(topLeft);
            fileButton.label->SetFont(gAssetManager.LoadFont("SID_TEXT_14.FON"));
            fileButton.button->SetPressCallback([this, selectCallback, file](UIButton* button){
                mWindowRoot->SetActive(false);
                if(selectCallback != nullptr) { selectCallback(file); }
            });

            // Shapes use a texture instead of a text label.
            if(mForShapes)
            {
                Texture* shapeTexture = gAssetManager.LoadTexture(file->name);
                fileButton.button->SetUpTexture(shapeTexture);
                topLeft.y -= shapeTexture->GetHeight();
            }
            else
            {
                fileButton.label->SetText("  " + file->GetDisplayName());
                topLeft.y -= 16.0f;
            }   
        }
    }
}

void SidneyFiles::FileListWindow::Show(const std::string& title, const std::vector<std::string>& choices, std::function<void(size_t)> selectCallback)
{
    // Show the window.
    mWindowRoot->SetActive(true);

    // Set the title.
    mTitleLabel->SetText(title);

    // Disable all existing labels.
    for(FileListButton& button : mButtons)
    {
        button.button->SetEnabled(false);
        button.label->SetEnabled(false);
    }
    mButtonIndex = 0;

    // Add desired choices.
    Vector2 topLeft(10.0f, -28.0f);
    for(size_t i = 0; i < choices.size(); ++i)
    {
        FileListButton& fileButton = GetFileListButton();
        fileButton.label->GetRectTransform()->SetAnchoredPosition(topLeft);
        fileButton.label->SetFont(gAssetManager.LoadFont("SID_TEXT_14.FON"));
        fileButton.label->SetText(choices[i]);

        // Set callback for pressing this choice.
        fileButton.button->SetPressCallback([this, selectCallback, i](UIButton* button){
            mWindowRoot->SetActive(false);
            if(selectCallback != nullptr)
            {
                selectCallback(i);
            }
        });

        // Move down to next label position.
        topLeft.y -= 16.0f;
    }
}

SidneyFiles::FileListButton& SidneyFiles::FileListWindow::GetFileListButton()
{
    // Need to create a new button maybe.
    if(mButtonIndex >= mButtons.size())
    {
        Actor* labelActor = new Actor(TransformType::RectTransform);
        labelActor->GetTransform()->SetParent(mWindowRoot->GetTransform());

        UILabel* label = labelActor->AddComponent<UILabel>();
        label->GetRectTransform()->SetPivot(0.0f, 1.0f); // Top-Left
        label->GetRectTransform()->SetAnchor(0.0f, 1.0f); // Top-Left
        label->GetRectTransform()->SetSizeDelta(128.0f, 16.0f);

        label->SetHorizonalAlignment(HorizontalAlignment::Left);
        label->SetVerticalAlignment(VerticalAlignment::Center);

        labelActor->AddComponent<UIButton>();

        mButtons.emplace_back();
        mButtons.back().button = labelActor->AddComponent<UIButton>();
        mButtons.back().label = label;
    }

    // We'll return this index.
    int idx = mButtonIndex;

    // Make sure it is enabled.
    mButtons[idx].button->SetEnabled(true);
    mButtons[idx].label->SetEnabled(true);

    // Increment index and return the button.
    ++mButtonIndex;
    return mButtons[idx];
}